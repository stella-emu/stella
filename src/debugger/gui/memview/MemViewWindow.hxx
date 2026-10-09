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

#ifndef MEM_VIEW_WINDOW_HXX
#define MEM_VIEW_WINDOW_HXX

class OSystem;
class Dialog;

#include "Rect.hxx"
#include "FrameBufferConstants.hxx"
#include "DialogContainer.hxx"

/**
  The DialogContainer for the debugger's memory-view window.  It is owned and
  driven directly by the Debugger, and renders into its own window, which can
  be open alongside the companion TIA window.  See MemViewWindowDialog for
  content.

  The window is freely resizable, independently of the debugger window; its
  size is persisted in the 'memview.res' setting.

  @author  Stephen Anthony
*/
class MemViewWindow : public DialogContainer
{
  public:
    explicit MemViewWindow(OSystem& osystem);
    ~MemViewWindow() override;

    /**
      The current size of the window, in logical UI pixels.  The dialog takes
      its own size from this each time it lays out.
    */
    const Common::Size& size() const { return mySize; }

    /**
      The smallest size the window may be dragged to: the dialog's
      layout-tree minimum (see MemViewWindowDialog).
    */
    Common::Size minSize() const;

    /**
      The size the window opens at before the user has ever resized it.
      This is its minimum size; the user can enlarge it from there.
    */
    static Common::Size defaultSize();

    /**
      Apply a pending live resize of the window and re-flow it; called from
      FrameBuffer::resizeSecondaryWindow().
    */
    bool applyResize() override;

    // The debugger tracks this window's open state and resize settling
    void handleWindowResized(int width, int height) override;
    void handleWindowClose() override;

    /**
      Re-flow the dialog and re-assert its (possibly changed) minimum on the
      window, growing the window if it no longer fits.  The settings panel
      changes the bank arrangement, and with it what the content needs.
    */
    void updateMinSize();

    Dialog* baseDialog() override { return myBaseDialog.get(); }

  private:
    unique_ptr<Dialog> myBaseDialog;

    // The current size of the window, in logical UI pixels
    Common::Size mySize;

  private:
    // Following constructors and assignment operators not supported
    MemViewWindow() = delete;
    MemViewWindow(const MemViewWindow&) = delete;
    MemViewWindow(MemViewWindow&&) = delete;
    MemViewWindow& operator=(const MemViewWindow&) = delete;
    MemViewWindow& operator=(MemViewWindow&&) = delete;
};

#endif  // MEM_VIEW_WINDOW_HXX
