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

  The window is freely resizable, independently of the debugger window; its size
  is persisted in the 'tiawindow.res' setting.

  @author  Stephen Anthony
*/
class TiaWindow : public DialogContainer
{
  public:
    explicit TiaWindow(OSystem& osystem);
    ~TiaWindow() override;

    /**
      The current size of the companion window, in logical UI pixels.  The
      dialog takes its own size from this each time it lays out.
    */
    const Common::Size& size() const { return mySize; }

    /**
      The smallest size the window may be dragged to: the whole TIA output
      buffer at 1x, which TiaDisplayWidget scales its image to fit.
    */
    static Common::Size minSize();

    /**
      The size the window opens at before the user has ever resized it: the
      whole TIA output buffer at 2x, with no downscaling.
    */
    static Common::Size defaultSize();

    /**
      Apply a pending live resize of the companion window and re-flow it.  Only
      ever called with the secondary render target selected, from
      FrameBuffer::resizeSecondaryWindow().
    */
    bool applyResize() override;

    Dialog* baseDialog() override { return myBaseDialog.get(); }

  private:
    unique_ptr<Dialog> myBaseDialog;

    // The current size of the companion window, in logical UI pixels
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
