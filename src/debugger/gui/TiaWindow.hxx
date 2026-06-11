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

#ifndef TIA_WINDOW_HXX
#define TIA_WINDOW_HXX

class OSystem;
class Dialog;

#include "Rect.hxx"
#include "FrameBufferConstants.hxx"
#include "DialogContainer.hxx"

/**
  The DialogContainer for the debugger's companion TIA window.  It is owned and
  driven directly by the Debugger (which remains in DEBUGGER state) and renders
  into a dedicated, second FrameBuffer/window.  See TiaWindowDialog for content.

  @author  Stephen Anthony
*/
class TiaWindow : public DialogContainer
{
  public:
    explicit TiaWindow(OSystem& osystem);
    ~TiaWindow() override;

    /**
      The (fixed, for now) size of the companion window, in logical UI pixels.
    */
    const Common::Size& size() const { return mySize; }

    Dialog* baseDialog() override { return myBaseDialog; }

  private:
    Dialog* myBaseDialog{nullptr};

    // The (fixed, for now) size of the companion window, in logical UI pixels
    Common::Size mySize;

  private:
    // Following constructors and assignment operators not supported
    TiaWindow() = delete;
    TiaWindow(const TiaWindow&) = delete;
    TiaWindow(TiaWindow&&) = delete;
    TiaWindow& operator=(const TiaWindow&) = delete;
    TiaWindow& operator=(TiaWindow&&) = delete;
};

#endif  // TIA_WINDOW_HXX
