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

#ifndef TIA_DISPLAY_WIDGET_HXX
#define TIA_DISPLAY_WIDGET_HXX

class GuiObject;
class ContextMenu;
class FBSurface;

#include "Widget.hxx"
#include "Command.hxx"

/**
  The TIA display for the debugger's companion window.  Unlike the main
  debugger's TiaOutputWidget/TiaZoomWidget pair (which stay in the main window),
  this is a single, self-contained view that:

    - renders the palette + phosphor processed TIA image (via
      TIASurface::mapIndexedPixel) into its OWN FBSurface, which lives on the
      companion window's (secondary) backend;
    - scales that surface to fill the widget area, preserving aspect ratio, so
      it grows/shrinks with the (resizable) companion window; and
    - supports mouse-wheel zoom (towards the cursor) and drag-pan to magnify a
      sub-region, implemented by adjusting the surface's source rectangle.

  It is a CommandSender so future on-screen controls can drive the debugger.

  @author  Stephen Anthony
*/
class TiaDisplayWidget : public Widget, public CommandSender
{
  public:
    TiaDisplayWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h);
    ~TiaDisplayWidget() override = default;

    void loadConfig() override;

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    void handleMouseMoved(int x, int y) override;
    void handleMouseLeft() override;

    bool wantsFocus() const override { return true; }

  protected:
    void drawWidget(bool hilite) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Current TIA frame dimensions (native, indexed-pixel space)
    void frameSize(uInt32& w, uInt32& h) const;
    // Visible source size derived from the current zoom factor
    void visibleSize(float& w, float& h) const;
    // The visible (zoom/pan) source region, in native TIA pixels: top-left
    // (sx,sy) and size (vw,vh)
    void visibleRegion(uInt32& sx, uInt32& sy, uInt32& vw, uInt32& vh) const;
    // Copy the visible (zoom/pan) region of the live TIA frame into the
    // surface's top-left (palette + phosphor) and set its source rectangle
    void updateSurface();
    // Redraw the pixel-locked overlay (electron-beam cursor, future TIA-pixel
    // aligned marks) for the current frame
    void drawMarkers();
    // Recompute the surface's destination (scaled-to-fit) rectangle for the
    // current widget size
    void recalcRects();
    // Change the zoom factor, keeping the given widget-local point anchored
    void applyZoom(float zoom, int anchorX, int anchorY);
    // Clamp the pan offset so the visible region stays inside the frame
    void clampSource();

  private:
    shared_ptr<FBSurface> myTiaSurface;

    // Overlay layers composited on top of the TIA image by the dialog's render
    // callback, in this order.  Neither ever touches the live image pixels:
    //  - myMarkSurface: pixel-locked (source space).  It is given the image
    //    surface's exact src/dst rectangles, so anything drawn into it stays
    //    glued to its TIA pixel and zooms/pans with the image.  Hosts the
    //    electron-beam cursor (and future TIA-pixel-aligned marks: sprite,
    //    collision, HMOVE, ... indicators).
    //  - myHudSurface: screen space (widget pixels, HiDPI-scaled).  Reserved
    //    for fixed on-screen-size readouts/icons; populated lazily when the
    //    first HUD element is added (see drawMarkers() for the pattern).
    shared_ptr<FBSurface> myMarkSurface;
    shared_ptr<FBSurface> myHudSurface;

    unique_ptr<ContextMenu> myMenu;

    static constexpr float MIN_ZOOM = 1.0F, MAX_ZOOM = 10.0F;

    // Zoom factor (1 == whole frame fits the widget) and pan offset, the latter
    // being the top-left of the visible region in native TIA pixels
    float myZoom{1.0F};
    float mySrcX{0.0F}, mySrcY{0.0F};

    // Area the scaled image occupies within the widget, in widget-local logical
    // pixels (cached from the last layout; used to map mouse coords to source)
    int myImgX{0}, myImgY{0}, myImgW{0}, myImgH{0};

    // Drag-to-pan state
    bool myMouseMoving{false};
    int myClickX{0}, myClickY{0};

  private:
    // Following constructors and assignment operators not supported
    TiaDisplayWidget() = delete;
    TiaDisplayWidget(const TiaDisplayWidget&) = delete;
    TiaDisplayWidget(TiaDisplayWidget&&) = delete;
    TiaDisplayWidget& operator=(const TiaDisplayWidget&) = delete;
    TiaDisplayWidget& operator=(TiaDisplayWidget&&) = delete;
};

#endif  // TIA_DISPLAY_WIDGET_HXX
