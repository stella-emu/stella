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

#include "EmulationTiming.hxx"
#include "FSNode.hxx"
#include "Logger.hxx"
#include "TIAConstants.hxx"
#include "RawStream.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStream::RawStream(string_view path)
{
  // Only a regular file: opening a FIFO waits for a reader, and a pipe or a
  // device could keep the writer waiting
  const FSNode node(path);
  if(node.exists() && !node.isFile())
  {
    Logger::error(std::format("ERROR: rawstream '{}' isn't a regular file", path));
    return;
  }

  myFile = node.openOFStream();
  if(!myFile.is_open())
  {
    Logger::error(std::format("ERROR: rawstream couldn't open '{}'", path));
    return;
  }

  myOpen = true;
  myPendingAudio.reserve(1024);
  myWriter = std::thread([this] { writerLoop(); });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RawStream::~RawStream()
{
  if(!myWriter.joinable())
    return;

  writePendingAudio();
  beginChunk("END ");
  endChunk();
  send();

  {
    const std::scoped_lock lock(myQueueMutex);
    myStopping = true;
  }
  myQueueChanged.notify_all();
  myWriter.join();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RawStream::isBacklogged() const
{
  const std::scoped_lock lock(myQueueMutex);
  return myQueuedBytes >= BACKLOG;
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
  // The palette is often set again unchanged (leaving the debugger, say)
  if(rgb == myPalette)
    return;

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
  if(!isOpen())
  {
    // The stream has stopped; drop the samples made since the last frame
    myPendingAudio.clear();
    return;
  }

  {
    const std::scoped_lock lock(myInfoMutex);
    if(myHeaderDirty || timing != myTiming)
      writeHeader(timing);
    if(myPaletteDirty)
      writePalette();
  }
  writePendingAudio();

  beginChunk("RAM ");
  myBatch.putByteArray(ram);
  endChunk();

  constexpr uInt32 width = TIAConstants::H_PIXEL;
  beginChunk("FRAM");
  myBatch.putLong(cycles);
  myBatch.putInt(frameNumber);
  myBatch.putShort(U16(scanlines));
  myBatch.putShort(U16(startLine));
  myBatch.putShort(U16(width));
  myBatch.putShort(U16(height));
  myBatch.putByteArray(ByteSpan(pixels, size_t{width} * height));
  endChunk();

  send();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::jump(Jump reason)
{
  if(!isOpen())
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

  beginChunk("JUMP");
  myBatch.putByte(U8(reason));
  endChunk();

  send();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::beginChunk(string_view tag)
{
  // The payload length is filled in by endChunk()
  myChunkStart = myBatch.size();
  myBatch.putByteArray(ByteSpan(reinterpret_cast<const uInt8*>(tag.data()), 4));
  myBatch.putInt(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::endChunk()
{
  const size_t end = myBatch.size();
  myBatch.setPosition(myChunkStart + 4);
  myBatch.putInt(U32(end - myChunkStart - 8));
  myBatch.setPosition(end);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writeHeader(ConsoleTiming timing)
{
  // The stream is in emulated time, so it always runs at normal speed; there
  // are two audio samples per scanline, one every 38 CPU cycles
  const uInt32 cyclesPerSecond = EmulationTiming::nominalCyclesPerSecond(timing);

  beginChunk("HEAD");
  myBatch.putShort(VERSION);
  myBatch.putByte(U8(timing));
  myBatch.putByte(0);
  myBatch.putInt(cyclesPerSecond / 38);
  myBatch.putInt(cyclesPerSecond);
  myBatch.putShort(U16(TIAConstants::H_PIXEL));
  myBatch.putShort(0);
  for(const Int16 v: myMixMono)   myBatch.putShort(U16(v));
  for(const Int16 v: myMixStereo) myBatch.putShort(U16(v));
  myBatch.putString(myRomName);
  myBatch.putString(myRomMD5);
  endChunk();

  myTiming = timing;
  myHeaderDirty = false;
  myPaletteDirty = true;  // a reader starts a new palette with each header
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writePalette()
{
  beginChunk("PALT");
  for(const uInt32 rgb: myPalette)
  {
    myBatch.putByte(U8((rgb >> 16U) & 0xFFU));
    myBatch.putByte(U8((rgb >> 8U) & 0xFFU));
    myBatch.putByte(U8(rgb & 0xFFU));
  }
  endChunk();
  myPaletteDirty = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writePendingAudio()
{
  if(myPendingAudio.empty())
    return;

  beginChunk("AUDI");
  myBatch.putByteArray(myPendingAudio);
  endChunk();
  myPendingAudio.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::send()
{
  // Hand the batch over without copying it, and carry on in a buffer the
  // writer is done with; a new one (made without holding up the writer)
  // starts big enough for any batch so far
  Batch batch;
  {
    const std::scoped_lock lock(myQueueMutex);
    if(!mySpare.empty())
    {
      batch.bytes = std::move(mySpare.back());
      mySpare.pop_back();
    }
  }
  myLargestBatch = std::max(myLargestBatch, myBatch.size());
  if(batch.bytes.empty())
    batch.bytes.resize(myLargestBatch);
  batch.size = myBatch.swapBuffer(batch.bytes);

  std::unique_lock lock(myQueueMutex);
  // Wait for a slow disk rather than drop anything.  Normal emulation
  // stops at BACKLOG, so this only waits for the debugger, which runs
  // frames on the main thread
  myQueueChanged.wait(lock, [this] {
    return myQueuedBytes < MAX_QUEUED || !myOpen;
  });

  // The stream has stopped, so the batch is dropped
  if(!myOpen)
    return;

  myQueuedBytes += batch.size;
  myQueue.push_back(std::move(batch));
  lock.unlock();
  myQueueChanged.notify_all();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RawStream::writerLoop()
{
  bool ok = true;

  std::unique_lock lock(myQueueMutex);
  while(ok)
  {
    myQueueChanged.wait(lock, [this] { return !myQueue.empty() || myStopping; });
    if(myQueue.empty())
      break;  // stopping, and everything is written

    Batch batch = std::move(myQueue.front());
    myQueue.pop_front();
    lock.unlock();
    // Flushed at once, so a tool following the file sees each frame
    myFile.write(reinterpret_cast<const char*>(batch.bytes.data()),
                 static_cast<std::streamsize>(batch.size));
    ok = myFile.flush().good();
    lock.lock();

    myQueuedBytes -= batch.size;
    if(!ok)
    {
      // Drop everything from here on
      myOpen = false;
      myQueue.clear();
      myQueuedBytes = 0;
    }
    else if(mySpare.size() < MAX_SPARE)
      mySpare.push_back(std::move(batch.bytes));
    // send() may be waiting for queue space
    myQueueChanged.notify_all();
  }
  lock.unlock();

  if(!ok)
    Logger::error("ERROR: rawstream write failed, stream stopped");
}
