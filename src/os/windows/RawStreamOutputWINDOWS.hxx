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

#ifndef RAW_STREAM_OUTPUT_WINDOWS_HXX
#define RAW_STREAM_OUTPUT_WINDOWS_HXX

#include <atomic>

#include "Windows.hxx"
#include "RawStreamOutput.hxx"

/**
  Write the raw stream to a file, a pipe or stdout under Windows systems.
  A write that is waiting for the reader is stopped with
  CancelSynchronousIo().
*/
class RawStreamOutputWINDOWS : public RawStreamOutput
{
  public:
    RawStreamOutputWINDOWS() = default;
    ~RawStreamOutputWINDOWS() override;

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
      every later write().  Returns once no write() is in progress.
    */
    void cancel() override;

  private:
    HANDLE myHandle{INVALID_HANDLE_VALUE};
    bool myIsStdout{false};

    // The writing thread, opened by its first write() so that cancel()
    // can stop it
    std::atomic<HANDLE> myWriter{nullptr};

    std::atomic<bool> myWriting{false};
    std::atomic<bool> myCancelled{false};

  private:
    // Following constructors and assignment operators not supported
    RawStreamOutputWINDOWS(const RawStreamOutputWINDOWS&) = delete;
    RawStreamOutputWINDOWS(RawStreamOutputWINDOWS&&) = delete;
    RawStreamOutputWINDOWS& operator=(const RawStreamOutputWINDOWS&) = delete;
    RawStreamOutputWINDOWS& operator=(RawStreamOutputWINDOWS&&) = delete;
};

#endif  // RAW_STREAM_OUTPUT_WINDOWS_HXX
