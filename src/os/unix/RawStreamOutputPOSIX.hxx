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

#ifndef RAW_STREAM_OUTPUT_POSIX_HXX
#define RAW_STREAM_OUTPUT_POSIX_HXX

#include <atomic>

#include "RawStreamOutput.hxx"

/**
  Write the raw stream to a file, a pipe or stdout under UNIX/POSIX.
  A pipe is only written once poll() reports room for it, so a write that
  is waiting for the reader checks for cancel() between short polls.
*/
class RawStreamOutputPOSIX : public RawStreamOutput
{
  public:
    RawStreamOutputPOSIX() = default;
    ~RawStreamOutputPOSIX() override;

    /**
      Open the given file or pipe for writing, or stdout for '-'.

      @param path  The name of the file or pipe
      @return  False on any errors, else true
    */
    bool open(const string& path) override;

    /**
      Write all of the given bytes, waiting for the reader as needed.

      @param data  The bytes to write
      @return  False on any errors or after cancel(), else true
    */
    bool write(ByteSpan data) override;

    /**
      Make a write() that is waiting for the reader give up, along with
      every later write().
    */
    void cancel() override { myCancelled = true; }

  private:
    static constexpr int INVALID_HANDLE_VALUE = -1;

    // How long a waiting write() goes between checks for cancel()
    static constexpr int POLL_TIMEOUT_MS = 100;

    // File descriptor for the output
    int myHandle{INVALID_HANDLE_VALUE};
    bool myIsStdout{false};

    // Regular files never wait for a reader, so only they are written
    // without polling
    bool myIsFile{false};

    std::atomic<bool> myCancelled{false};

  private:
    // Following constructors and assignment operators not supported
    RawStreamOutputPOSIX(const RawStreamOutputPOSIX&) = delete;
    RawStreamOutputPOSIX(RawStreamOutputPOSIX&&) = delete;
    RawStreamOutputPOSIX& operator=(const RawStreamOutputPOSIX&) = delete;
    RawStreamOutputPOSIX& operator=(RawStreamOutputPOSIX&&) = delete;
};

#endif  // RAW_STREAM_OUTPUT_POSIX_HXX
