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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef TIA_6502TS_CORE_FRAME_MANAGER
#define TIA_6502TS_CORE_FRAME_MANAGER

#include <functional>
#include "bspf.hxx"

namespace TIA6502tsCore {

class FrameManager {

  public:

    enum TvMode {
      pal, ntsc
    };

    typedef std::function<void()> frameCompletionHandler;

  public:

    FrameManager();

  public:

    void setOnFrameCompleteHandler(frameCompletionHandler);

    void reset();

    void nextLine();

    void setVblank(bool vblank);

    void setVsync(bool vsync);

    bool isRendering() const;

    TvMode tvMode() const;

    bool vblank() const;

    uInt32 height() const;

    uInt32 currentLine() const;

  private:

    enum State {
      waitForVsyncStart,
      waitForVsyncEnd,
      waitForFrameStart,
      frame,
      overscan
    };

  private:

    void setTvMode(TvMode mode);

    void setState(State state);

    void finalizeFrame();

  private:

    frameCompletionHandler myOnFrameComplete;

    TvMode myMode;
    State myState;
    bool myWaitForVsync;
    uInt32 myLineInState;
    uInt32 myLinesWithoutVsync;
    uInt32 myCurrentFrameTotalLines;

    bool myVsync;
    bool myVblank;

    uInt32 myVblankLines;
    uInt32 myKernelLines;
    uInt32 myOverscanLines;
    uInt32 myFrameLines;
    uInt32 myMaxLinesWithoutVsync;

  private:

    FrameManager(const FrameManager&) = delete;
    FrameManager(FrameManager&&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;
    FrameManager& operator=(FrameManager&&) = delete;

};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_FRAME_MANAGER