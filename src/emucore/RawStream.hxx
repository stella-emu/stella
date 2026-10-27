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

#ifndef RAW_STREAM_HXX
#define RAW_STREAM_HXX

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

class RawStreamOutput;

#include "bspf.hxx"
#include "ConsoleTiming.hxx"
#include "FrameBufferConstants.hxx"

/**
  Writes the emulated TIA picture and sound, untouched, to a file or to
  stdout ('-'), so that outside tools can record, stream or analyse them.
  Enabled with '-rawstream <path>'.

  The stream is a sequence of chunks: a 4-character tag, a little-endian
  uInt32 payload length, then the payload.  Readers skip tags they don't
  know.  The chunk layouts are described in docs/rawstream.md.

  audioSample() and frame() run on the emulation thread.  The setters and
  jump() run on the main thread while emulation is paused.  A dedicated
  thread does the writing; when the reader falls MAX_QUEUED bytes behind,
  emulation waits for it, so the stream never skips data.  If the reader
  goes away, the stream stops and emulation carries on.  When the stream
  closes, it waits for the reader to take the rest, unless the reader has
  stopped reading for STALL_TIMEOUT.

  @author  Newbroo Games
*/
class RawStream
{
  public:
    static constexpr uInt16 VERSION = 1;

    enum class Jump: uInt8 { reset = 0, stateLoad = 1, romLoad = 2 };

    explicit RawStream(string_view path);
    ~RawStream();

    bool isOpen() const { return myOutput != nullptr; }

    /**
      Give stdout to the stream ('-rawstream -') for the rest of the run:
      Stella's own console messages go to stderr.  main() calls this before
      anything is logged; later calls do nothing.
    */
    static void claimStdout();

    void setRom(string_view name, string_view md5);
    void setPalette(const PaletteArray& rgb);
    void setMixingTables(const Int16* mono, const Int16* stereo);

    /**
      One audio sample (two per scanline): channel 0's volume in the low
      nibble, channel 1's in the high nibble.
    */
    void audioSample(uInt8 sample) { myPendingAudio.push_back(sample); }

    /**
      A finished frame of TIA color indices, 160 per row, and the console's
      128 bytes of RAM ($80-$FF) at that moment.
    */
    void frame(const uInt8* pixels, uInt32 height, uInt32 startLine,
               uInt32 scanlines, uInt32 frameNumber, uInt64 cycles,
               ConsoleTiming timing, ByteSpan ram);

    /**
      Emulation time did not run on from the last frame.
    */
    void jump(Jump reason);

  private:
    void writeHeader(ConsoleTiming timing);
    void writePalette();
    void writePendingAudio();
    void send(ByteArray&& chunk);
    void writerLoop();

  private:
    static constexpr size_t MAX_QUEUED = 64 * 1024 * 1024;

    // How long closing waits on a reader that has stopped reading
    static constexpr auto STALL_TIMEOUT = std::chrono::seconds(2);

    unique_ptr<RawStreamOutput> myOutput;

    // Set on the main thread, read on the emulation thread
    std::mutex myInfoMutex;
    string myRomName, myRomMD5;
    PaletteArray myPalette{};
    std::array<Int16, 31> myMixMono{};
    std::array<Int16, 16> myMixStereo{};
    bool myHeaderDirty{true}, myPaletteDirty{true};
    ConsoleTiming myTiming{ConsoleTiming::numTimings};

    ByteArray myPendingAudio;

    std::mutex myQueueMutex;
    std::condition_variable myQueueChanged;
    std::deque<ByteArray> myQueue;
    size_t myQueuedBytes{0};
    // Closing: send() no longer waits for queue space
    bool myClosing{false};
    // Stopping: everything is queued, so the writer finishes once it's written
    bool myStopping{false};
    bool myBroken{false};
    bool myWriterDone{false};
    std::thread myWriter;

  private:
    // Following constructors and assignment operators not supported
    RawStream() = delete;
    RawStream(const RawStream&) = delete;
    RawStream(RawStream&&) = delete;
    RawStream& operator=(const RawStream&) = delete;
    RawStream& operator=(RawStream&&) = delete;
};

#endif
