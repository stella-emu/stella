//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <csignal>

#if defined(BSPF_WINDOWS)
  #include <fcntl.h>
  #include <io.h>
#endif

#include "Logger.hxx"
#include "TIAConstants.hxx"
#include "RawStream.hxx"

namespace {
  // All values are little-endian
  void put8(ByteArray& b, uInt8 v)  { b.push_back(v); }
  void put16(ByteArray& b, uInt16 v)
  {
    b.push_back(U8(v & 0xFFU));
    b.push_back(U8(v >> 8U));
  }
  void put32(ByteArray& b, uInt32 v)
  {
    for(uInt32 shift = 0; shift < 32; shift += 8)
      b.push_back(U8((v >> shift) & 0xFFU));
  }
  void put64(ByteArray& b, uInt64 v)
  {
    for(uInt32 shift = 0; shift < 64; shift += 8)
      b.push_back(U8((v >> shift) & 0xFFU));
  }
  void putString(ByteArray& b, string_view s)
  {
    const auto len = U16(std::min<size_t>(s.size(), 0xFFFF));
    put16(b, len);
    b.insert(b.end(), s.begin(), s.begin() + len);
  }

  // Tag plus a length that send() fills in
  ByteArray newChunk(const char* tag, size_t payloadSize)
  {
    ByteArray b;
    b.reserve(8 + payloadSize);
    b.insert(b.end(), tag, tag + 4);
    put32(b, 0);
    return b;
  }

  // Two samples per scanline: (lines per frame * 76 cycles * frames per second) / 38
  uInt32 audioSampleRate(ConsoleTiming timing)
  {
    return timing == ConsoleTiming::ntsc ? 262 * 76 * 60 / 38 : 312 * 76 * 50 / 38;
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStream::RawStream(string_view path)
{
  if(path == "-")
  {
    claimStdout();
    myFile = stdout;
    myIsStdout = true;
  }
  else
  {
    const string name{path};
  #if defined(BSPF_WINDOWS)
    if(fopen_s(&myFile, name.c_str(), "wb") != 0)
      myFile = nullptr;
  #else
    myFile = std::fopen(name.c_str(), "wb");
  #endif
    if(!myFile)
    {
      Logger::error(std::format("ERROR: rawstream couldn't open '{}'", name));
      return;
    }
  }
#ifdef SIGPIPE
  // A reader that quits must not take Stella down with it
  std::signal(SIGPIPE, SIG_IGN);
#endif

  myPendingAudio.reserve(1024);
  myWriter = std::thread([this] { writerLoop(); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStream::~RawStream()
{
  if(!myFile)
    return;

  writePendingAudio();
  send(newChunk("END ", 0));
  {
    const std::scoped_lock lock(myQueueMutex);
    myStopping = true;
  }
  myQueueChanged.notify_all();
  myWriter.join();

  if(myIsStdout)
    std::fflush(stdout);
  else
    std::fclose(myFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::claimStdout()
{
  static bool claimed = false;
  if(claimed)
    return;
  claimed = true;

#if defined(BSPF_WINDOWS)
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  // stdout carries only the stream, so Stella's own messages go to stderr
  cout.rdbuf(cerr.rdbuf());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::setRom(string_view name, string_view md5)
{
  // A ROM (re)load breaks time just like a reset
  jump(Jump::romLoad);

  const std::scoped_lock lock(myInfoMutex);
  myRomName = name;
  myRomMD5 = md5;
  myHeaderDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::setPalette(const PaletteArray& rgb)
{
  const std::scoped_lock lock(myInfoMutex);
  myPalette = rgb;
  myPaletteDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::setMixingTables(const Int16* mono, const Int16* stereo)
{
  const std::scoped_lock lock(myInfoMutex);
  // The tables never change, so this only marks the header once
  if(std::equal(myMixMono.begin(), myMixMono.end(), mono) &&
     std::equal(myMixStereo.begin(), myMixStereo.end(), stereo))
    return;

  std::copy_n(mono, myMixMono.size(), myMixMono.begin());
  std::copy_n(stereo, myMixStereo.size(), myMixStereo.begin());
  myHeaderDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::frame(const uInt8* pixels, uInt32 height, uInt32 startLine,
                      uInt32 scanlines, uInt32 frameNumber, uInt64 cycles,
                      ConsoleTiming timing, ByteSpan ram)
{
  if(!myFile)
    return;

  {
    const std::scoped_lock lock(myInfoMutex);
    if(myHeaderDirty || timing != myTiming)
      writeHeader(timing);
    if(myPaletteDirty)
      writePalette();
  }
  writePendingAudio();

  ByteArray ramChunk = newChunk("RAM ", ram.size());
  ramChunk.insert(ramChunk.end(), ram.begin(), ram.end());
  send(std::move(ramChunk));

  constexpr uInt32 width = TIAConstants::H_PIXEL;
  ByteArray chunk = newChunk("FRAM", 20 + size_t{width} * height);
  put64(chunk, cycles);
  put32(chunk, frameNumber);
  put16(chunk, U16(scanlines));
  put16(chunk, U16(startLine));
  put16(chunk, U16(width));
  put16(chunk, U16(height));
  chunk.insert(chunk.end(), pixels, pixels + size_t{width} * height);
  send(std::move(chunk));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::jump(Jump reason)
{
  if(!myFile)
    return;

  {
    // Nothing has been sent yet, so there is nothing to break from
    const std::scoped_lock lock(myInfoMutex);
    if(myTiming == ConsoleTiming::numTimings)
    {
      myPendingAudio.clear();
      return;
    }
  }
  writePendingAudio();

  ByteArray chunk = newChunk("JUMP", 1);
  put8(chunk, U8(reason));
  send(std::move(chunk));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writeHeader(ConsoleTiming timing)
{
  const uInt32 sampleRate = audioSampleRate(timing);

  ByteArray chunk = newChunk("HEAD", 128);
  put16(chunk, VERSION);
  put8(chunk, U8(timing));
  put8(chunk, 0);
  put32(chunk, sampleRate);
  put32(chunk, sampleRate * 38);  // CPU cycles per second
  put16(chunk, U16(TIAConstants::H_PIXEL));
  put16(chunk, 0);
  for(const Int16 v: myMixMono)   put16(chunk, U16(v));
  for(const Int16 v: myMixStereo) put16(chunk, U16(v));
  putString(chunk, myRomName);
  putString(chunk, myRomMD5);
  send(std::move(chunk));

  myTiming = timing;
  myHeaderDirty = false;
  myPaletteDirty = true;  // a reader starts a new palette with each header
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writePalette()
{
  ByteArray chunk = newChunk("PALT", myPalette.size() * 3);
  for(const uInt32 rgb: myPalette)
  {
    put8(chunk, U8((rgb >> 16U) & 0xFFU));
    put8(chunk, U8((rgb >> 8U) & 0xFFU));
    put8(chunk, U8(rgb & 0xFFU));
  }
  send(std::move(chunk));
  myPaletteDirty = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writePendingAudio()
{
  if(myPendingAudio.empty())
    return;

  ByteArray chunk = newChunk("AUDI", myPendingAudio.size());
  chunk.insert(chunk.end(), myPendingAudio.begin(), myPendingAudio.end());
  send(std::move(chunk));
  myPendingAudio.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::send(ByteArray&& chunk)
{
  const auto length = U32(chunk.size() - 8);
  for(uInt32 i = 0; i < 4; ++i)
    chunk[4 + i] = U8((length >> (8 * i)) & 0xFFU);

  std::unique_lock lock(myQueueMutex);
  // Wait for a slow reader rather than drop anything
  myQueueChanged.wait(lock, [this] { return myQueuedBytes < MAX_QUEUED || myBroken; });
  if(myBroken)
    return;

  myQueuedBytes += chunk.size();
  myQueue.push_back(std::move(chunk));
  lock.unlock();
  myQueueChanged.notify_all();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writerLoop()
{
  std::unique_lock lock(myQueueMutex);
  for(;;)
  {
    myQueueChanged.wait(lock, [this] { return !myQueue.empty() || myStopping; });
    if(myQueue.empty())
      break;  // stopping, and everything is written

    std::deque<ByteArray> batch;
    batch.swap(myQueue);
    lock.unlock();

    bool ok = true;
    size_t written = 0;
    for(const ByteArray& chunk: batch)
    {
      if(ok)
        ok = std::fwrite(chunk.data(), 1, chunk.size(), myFile) == chunk.size();
      written += chunk.size();
    }
    if(ok)
      ok = std::fflush(myFile) == 0;

    lock.lock();
    myQueuedBytes -= written;
    if(!ok)
    {
      // The reader has gone away; drop everything from here on
      myBroken = true;
      myQueue.clear();
      myQueuedBytes = 0;
    }
    myQueueChanged.notify_all();
    if(myBroken)
    {
      lock.unlock();
      Logger::error("ERROR: rawstream write failed, stream stopped");
      return;
    }
  }
}
