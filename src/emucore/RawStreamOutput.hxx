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

#ifndef RAW_STREAM_OUTPUT_HXX
#define RAW_STREAM_OUTPUT_HXX

#include "bspf.hxx"

/**
  This class provides an interface for the file, pipe or stdout that the
  raw stream ('-rawstream') is written to.  Writing may wait for a slow
  reader, but another thread can always make it give up, so that Stella
  can close without waiting for a reader that has stopped reading.
*/
class RawStreamOutput
{
  public:
    RawStreamOutput() = default;
    virtual ~RawStreamOutput() = default;

    /**
      Open the given file or pipe for writing, or stdout for '-'.

      @param path  The name of the file or pipe
      @return  False on any errors, else true
    */
    virtual bool open(const string& path) { return false; }

    /**
      Write all of the given bytes, waiting for the reader as needed.
      Only one thread may write.

      @param data  The bytes to write
      @return  False on any errors or after cancel(), else true
    */
    virtual bool write(ByteSpan data) { return false; }

    /**
      Make a write() that is waiting for the reader give up, along with
      every later write().  Called from a thread other than the writer.
    */
    virtual void cancel() { }

  private:
    // Following constructors and assignment operators not supported
    RawStreamOutput(const RawStreamOutput&) = delete;
    RawStreamOutput(RawStreamOutput&&) = delete;
    RawStreamOutput& operator=(const RawStreamOutput&) = delete;
    RawStreamOutput& operator=(RawStreamOutput&&) = delete;
};

#endif  // RAW_STREAM_OUTPUT_HXX
