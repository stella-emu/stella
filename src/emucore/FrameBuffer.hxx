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

#ifndef FRAME_BUFFER_HXX
#define FRAME_BUFFER_HXX

#include <list>
#include <unordered_map>

class OSystem;
class Console;
class Settings;
class FBSurface;
class TIASurface;
class Bezel;
class DialogContainer;

#ifdef GUI_SUPPORT
  #include "Font.hxx"
#endif

#include "Rect.hxx"
#include "Variant.hxx"
#include "TIAConstants.hxx"
#include "FBBackend.hxx"
#include "FBMessageHandler.hxx"
#include "FrameBufferConstants.hxx"
#include "EventHandlerConstants.hxx"
#include "VideoModeHandler.hxx"
#include "bspf.hxx"

/**
  This class encapsulates all video buffers and is the basis for the video
  display in Stella.  The FBBackend object contained in this class is
  platform-specific, and most rendering tasks are delegated to it.

  The TIA is drawn here, and all GUI elements (ala ScummVM, which are drawn
  into FBSurfaces), are in turn drawn here as well.

  @author  Stephen Anthony
*/
class FrameBuffer
{
  public:
    // Zoom level step interval
    static constexpr double ZOOM_STEPS = 0.25;

    enum class UpdateMode: uInt8 {
      NONE = 0,
      REDRAW = 1,
      RERENDER = 2
    };

  public:
    explicit FrameBuffer(OSystem& osystem);
    ~FrameBuffer();

    /**
      Initialize the framebuffer object (set up the underlying hardware).
      Throws an exception upon encountering any errors.
    */
    void initialize();

    /**
      (Re)creates the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the application / window
      @param size    The dimensions of the display
      @param honourHiDPI  If true, consult the 'hidpi' setting and enlarge
                          the display size accordingly; if false, use the
                          exact dimensions as given

      @return  Status of initialization (see FBInitStatus 'enum')
    */
    FBInitStatus createDisplay(string_view title, BufferType type,
                               Common::Size size, bool honourHiDPI = true);

    /**
      Handle a user resize of a windowed UI.  Rebuilds the active video mode
      for the new window size and reloads all surfaces, *without* recreating
      the window.  Only applies to resizeable UI windows (the launcher).

      @param width   The new window width, in pixels
      @param height  The new window height, in pixels
    */
    void handleResize(int width, int height);

    /**
      Begin (or continue) a deferred, interactive resize of a resizeable UI
      window.  Instead of rebuilding the UI on every resize event (which
      re-letterboxes and flickers), the current frame is frozen and stretched
      to fill the window while dragging; the actual rebuild happens once via
      applyPendingResize() when the drag settles.

      @param width   The latest window width, in pixels
      @param height  The latest window height, in pixels
      @return  True if the resize was deferred (resizeable UI window); false
               if the caller should resize immediately via handleResize()
    */
    bool deferResize(int width, int height);

    /**
      Apply the most recent size recorded by deferResize(): stop stretching
      and rebuild the UI for the new window size.  No-op if no deferred resize
      is pending.
    */
    void applyPendingResize();

    /**
      Record the latest window size for a live, per-frame re-flow (rather than
      the stretch-then-settle path).  Returns true for a window that re-flows
      live — the caller then applies it via applyLiveResize() and re-lays-out
      its dialogs; false otherwise, so the caller falls back to deferResize()/
      handleResize().

      @param width   The latest window width, in pixels
      @param height  The latest window height, in pixels
      @return  True if this window re-flows live
    */
    bool liveResize(int width, int height);

    /**
      If a live resize is pending, rebuild the UI at the recorded size and clear
      the flag.  Returns true if it applied (the caller then re-flows its
      dialogs), false if nothing was pending.
    */
    bool applyLiveResize();

    /**
      Set the minimum size (in logical UI pixels) the current window may be
      resized to.  Used by resizeable UI dialogs to prevent the window being
      shrunk small enough to clip their content.

      @param size  The minimum size, in logical (unscaled) UI pixels
    */
    void setWindowMinSize(const Common::Size& size);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the TIA, any pending menus, etc.
    */
    void update(UpdateMode mode = UpdateMode::NONE);

    /**
      Secondary-window support.  In addition to the primary window (launcher /
      emulation / main debugger), the FrameBuffer can drive one additional
      window backed by its own FBBackend (e.g. the debugger's companion TIA
      window).  All other state -- palette, fonts, TIASurface -- is shared, so
      the secondary window is *not* a separate FrameBuffer; only the window /
      renderer / surfaces differ.  These methods scope the internal render
      target switch themselves; callers never see it.

      @param container  The DialogContainer rendered into the secondary window
      @param title      The secondary window title
      @param type       The BufferType (geometry/position key) for the window
      @param size       The secondary window size, in logical UI pixels
    */
    FBInitStatus openSecondaryWindow(DialogContainer& container,
                                     string_view title, BufferType type,
                                     Common::Size size);

    /**
      Draw the secondary window's container and present it.  No-op if no
      secondary window is open.
    */
    void renderSecondaryWindow(DialogContainer& container,
                               UpdateMode mode = UpdateMode::REDRAW);

    /**
      Hide the secondary window (its backend/surfaces are kept for re-open).
    */
    void closeSecondaryWindow();

    /**
      Whether the secondary window is currently shown.
    */
    bool secondaryWindowOpen() const { return mySecondaryActive; }

    /**
      The platform window ID of the secondary window (0 if none).  Used to
      route window-specific events to it.
    */
    uInt32 secondaryWindowId() const;

    /**
      There is a dedicated update method for emulation mode.
    */
    void updateInEmulationMode(float framesPerSecond);

    /**
      Set pending rendering flag.
    */
    void setPendingRender() { myPendingRender = true; }

    /**
      Shows a text message onscreen.

      @param message  The message to be shown
      @param position Onscreen position for the message
      @param force    Force showing this message, even if messages are disabled
    */
    void showTextMessage(string_view message,
                         MessagePosition position = MessagePosition::BottomCenter,
                         bool force = false);
    /**
      Shows a message with a gauge bar onscreen.

      @param message    The message to be shown
      @param valueText  The value of the gauge bar as text
      @param value      The gauge bar percentage
      @param minValue   The minimal value of the gauge bar
      @param maxValue   The maximal value of the gauge bar
    */
    void showGaugeMessage(string_view message, string_view valueText,
                          float value, float minValue = 0.F, float maxValue = 100.F);

    bool messageShown() const;

    /**
      Toggles showing or hiding framerate statistics.
    */
    void toggleFrameStats(bool toggle = true);

    /**
      Shows a message containing frame statistics for the current frame.
    */
    void showFrameStats(bool enable);

    /**
      Enable/disable any pending messages.  Disabled messages aren't removed
      from the message queue; they're just not redrawn into the framebuffer.
    */
    void enableMessages(bool enable);

    /**
      Reset 'Paused' display delay counter
    */
    void setPauseDelay();

    /**
      Allocate a new surface.  The FrameBuffer class takes all responsibility
      for freeing this surface (ie, other classes must not delete it directly).

      @param w      The requested width of the new surface
      @param h      The requested height of the new surface
      @param inter  Interpolation mode
      @param data   If non-null, use the given data values as a static surface

      @return  A pointer to a valid surface object, or nullptr
    */
    shared_ptr<FBSurface> allocateSurface(
      int w,
      int h,
      ScalingInterpolation inter = ScalingInterpolation::none,
      const uInt32* data = nullptr
    );

    /**
      Deallocate a previously allocated surface.  If no such surface exists,
      this method does nothing.

      @param surface  The surface to remove/deallocate
    */
    void deallocateSurface(const shared_ptr<FBSurface>& surface);

    /**
      Set up the TIA/emulation palette.  Due to the way the palette is stored,
      a call to this method implicitly calls setUIPalette() too.

      @param rgb_palette  The array of colors in R/G/B format
    */
    void setTIAPalette(const PaletteArray& rgb_palette);

    /**
      Set palette for user interface.
    */
    void setUIPalette();

    /**
      Set disassembly syntax colors.  The active UI theme determines which
      disasm palette is used: light themes (standard, light) use the standard
      disasm palette; dark themes (classic, dark) use the dark one.
      Called automatically by setUIPalette(); can also be called standalone
      when only the disassembly palette needs refreshing.
    */
    void setDisasmPalette();

    /**
      Returns the current dimensions of the framebuffer image.
      Note that this will take into account the current scaling (if any)
      as well as image 'centering'.
    */
    const Common::Rect& imageRect() const { return myActiveVidMode.imageR; }

    /**
      Returns the current dimensions of the framebuffer window.
      This is the entire area containing the framebuffer image as well as any
      'unusable' area.
    */
    const Common::Size& screenSize() const { return myActiveVidMode.screenS; }
    const Common::Rect& screenRect() const { return myActiveVidMode.screenR; }

    /**
      Returns the dimensions of the mode specific users' desktop, or if
      BufferType::None, return the dimensions of the current active screen.
    */
    const Common::Size& desktopSize(BufferType bufferType = BufferType::None) const {
      return myDesktopSize.at(displayId(bufferType));
    }

    /**
      Get the supported renderers for the video hardware.

      @return  An array of supported renderers
    */
    const VariantList& supportedRenderers() const { return myRenderers; }

    /**
      Get the minimum/maximum supported TIA zoom level (windowed mode)
      for the framebuffer.
    */
    double supportedTIAMinZoom() const { return myTIAMinZoom * hidpiScaleFactor(); }
    double supportedTIAMaxZoom() const { return maxWindowZoom(); }

    /**
      Get the TIA surface associated with the framebuffer.
      Note that this is the 'raw' TIA surface, without any post-processing
      effects included.
    */
    TIASurface& tiaSurface() const { return *myTIASurface; }

    /**
      This method is called to get the specified ARGB data from the viewable
      FrameBuffer area.  Note that this isn't the same as any internal
      surfaces that may be in use; it should return the actual data as it
      is currently seen onscreen.

      Currently this is used only for taking PNG snapshots.  As such, it is slow
      and should not be used for anything else.
    */
    const FBSurface& compositedSurface() {
      return myBackend->compositedSurface();
    }

    /**
      Toggles between fullscreen and window mode.
    */
    void toggleFullscreen(bool toggle = true);

  #ifdef ADAPTABLE_REFRESH_SUPPORT
    /**
      Toggles between adapt fullscreen refresh rate on and off.
    */
    void toggleAdaptRefresh(bool toggle = true);
  #endif

    /**
      Changes the fullscreen overscan.

      @param direction  +1 indicates increase, -1 indicates decrease
    */
    void changeOverscan(int direction = +1);

    /**
      This method is called when the user wants to switch to the previous/next
      available TIA video mode.  In windowed mode, this typically means going
      to the next/previous zoom level.  In fullscreen mode, this typically
      means switching between normal aspect and fully filling the screen.

      @param direction  +1 indicates next mode, -1 indicates previous mode
    */
    void switchVideoMode(int direction = +1);

    /**
      Toggles the bezel display.
    */
    void toggleBezel(bool toggle = true);

    /**
      Sets the state of the cursor (hidden or grabbed) based on the
      current mode.
    */
    void setCursorState();

    /**
      Enable/disable text events (distinct from single-key events).
    */
    void enableTextEvents(bool enable);

    /**
      Checks if mouse grabbing is allowed.
    */
    bool grabMouseAllowed();

    /**
      Sets the use of grabmouse.
    */
    void enableGrabMouse(bool enable);

    /**
      Toggles the use of grabmouse (only has effect in emulation mode).
    */
    void toggleGrabMouse(bool toggle = true);

    /**
      Query whether grabmouse is enabled.
    */
    bool grabMouseEnabled() const { return myGrabMouse; }

    /**
      Informs the Framebuffer of a change in EventHandler state.
    */
    void stateChanged(EventHandlerState state);

    /**
      Answer whether hidpi mode is allowed.  In this mode, all FBSurfaces
      are scaled to 2x normal size.
    */
    bool hidpiAllowed() const { return myHiDPIAllowed.at(displayId()); }

    /**
      Answer whether hidpi mode is enabled.  In this mode, all FBSurfaces
      are scaled to 2x normal size.
    */
    bool hidpiEnabled() const { return myHiDPIEnabled.at(displayId()); }
    uInt32 hidpiScaleFactor() const { return myHiDPIEnabled.at(displayId()) ? 2 : 1; }

    /**
      This method should be called to save the current settings of all
      its subsystems.  Note that the this may be called when the class
      hasn't been fully initialized, so we first need to check if the
      subsytems actually exist.
    */
    void saveConfig(Settings& settings) const;

  #ifdef GUI_SUPPORT
    /**
      Get the font object(s) of the framebuffer
    */
    const GUI::Font& font() const { return *myFont; }
    const GUI::Font& infoFont() const { return *myInfoFont; }
    const GUI::Font& smallFont() const { return *mySmallFont; }
    const GUI::Font& launcherFont() const { return *myLauncherFont; }

    /**
      Change the launcher font at runtime.  The existing Font object is mutated
      in place (rather than replaced) so that every widget's reference to it
      stays valid and immediately picks up the new glyphs and metrics.  Callers
      must afterwards refresh font-derived state cached by the launcher widgets
      and re-run its layout (see LauncherDialog).

      @param name  The settings name of the new launcher font
    */
    void changeLauncherFont(string_view name) {
      myLauncherFont->changeDesc(getFontDesc(name));
    }

    /**
      Change the dialog font at runtime.  Like changeLauncherFont(), the Font
      objects are mutated in place so every widget's reference stays valid.
      The dialog font drives both the general UI font and the (auto-sized) info
      font, so both are updated.  Callers must afterwards refresh the cached
      font-derived state of the affected dialogs and re-run their layout
      (see DialogContainer::refreshFont / Dialog::refreshFont).

      @param name  The settings name of the new dialog font
    */
    void changeDialogFont(string_view name) {
      const FontDesc fd = getFontDesc(name);
      myFont->changeDesc(fd);
      myInfoFont->changeDesc(infoFontDesc(fd));
    }

    /**
      Get the font description from the font name

      @param name  The settings name of the font

      @return  The description of the font
    */
    static FontDesc getFontDesc(string_view name);

    /**
      Determine the info-font description that pairs with a given dialog font,
      aiming for roughly a 1 / 1.4 size ratio.

      @param fd  The dialog font description

      @return  The matching info font description
    */
    static FontDesc infoFontDesc(const FontDesc& fd);
  #endif  // GUI_SUPPORT

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    void showCursor(bool show) { myBackend->showCursor(show); }

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() const { return myBackend->fullScreen(); }

    /**
      Updates theme according to OS setting.

      @return  true if theme has changed
    */
    bool updateTheme();

    /**
      Retrieve the R/G/B/A masks from the FrameBuffer backend renderer.
    */
    uInt32 rMask() const { return myBackend->rMask(); }
    uInt32 gMask() const { return myBackend->gMask(); }
    uInt32 bMask() const { return myBackend->bMask(); }
    uInt32 aMask() const { return myBackend->aMask(); }

    /**
      Clear the framebuffer.
    */
    void clear() { myBackend->clear(); }
    void flush() { myBackend->flush(); }

    /**
      Transform from window to renderer coordinates, x/y direction.
     */
    int scaleX(int x) const { return myBackend->scaleX(x); }
    int scaleY(int y) const { return myBackend->scaleY(y); }

  private:
    /**
      These methods are used to load/save position and display of the
      current window.
    */
    string getPositionKey() const;
    string getDisplayKey(BufferType bufferType = BufferType::None) const;
    void saveCurrentWindowPosition() const;

    /**
      Frees and reloads all surfaces that the framebuffer knows about.
    */
    void resetSurfaces();

    /**
      Renders TIA and overlaying, optional bezel surface

      @param doClear  Clear the framebuffer before rendering
      @param shade    Shade the TIA surface after rendering
    */
    //void renderTIA(bool shade = false, bool doClear = true);
    void renderTIA(bool doClear = true, bool shade = false);

    /**
      Get the display used for the current mode.
    */
    uInt32 displayId(BufferType bufferType = BufferType::None) const;

    /**
      Build an applicable video mode based on the current settings in
      effect, whether TIA mode is active, etc.  Then tell the backend
      to actually use the new mode.

      @return  Whether the operation succeeded or failed
    */
    FBInitStatus applyVideoMode();

    /**
      Calculate the maximum level by which the base window can be zoomed and
      still fit in the desktop screen.
    */
    double maxWindowZoom() const;

    /**
      Enables/disables fullscreen mode.
    */
    void setFullscreen(bool enable);

  #ifdef GUI_SUPPORT
    /**
      Setup the UI fonts
    */
    void setupFonts();
  #endif  // GUI_SUPPORT

    /**
      Switch the active render target between the primary (0) and secondary (1)
      window.  This swaps only the per-window state (backend, video mode, buffer
      type); all shared state (palette, fonts, TIASurface) is unaffected.  The
      secondary-window methods scope this so the rest of the code always sees
      the primary target.
    */
    void setRenderTarget(int target);

    /**
      Draw a DialogContainer into the current render target and present it.
    */
    void updateContainer(DialogContainer& container, UpdateMode mode);

  private:
    // The parent system for the framebuffer
    OSystem& myOSystem;

    // Backend used for all platform-specific graphics operations.
    // This always refers to the *current* render target's backend; the
    // inactive target's backend is parked in myOtherBackend (see
    // setRenderTarget()).  Most code only ever sees the primary backend.
    unique_ptr<FBBackend> myBackend;

    // Per-window state for the *inactive* render target, swapped with the live
    // members (myBackend / myActiveVidMode / myBufferType) by setRenderTarget().
    // Used to drive a single secondary window (e.g. the debugger's companion
    // TIA window) without duplicating the shared palette/fonts/TIASurface.
    unique_ptr<FBBackend> myOtherBackend;
    int myRenderTarget{0};           // 0 = primary, 1 = secondary
    bool mySecondaryCreated{false};  // secondary backend has been created
    bool mySecondaryActive{false};   // secondary window is currently shown

    // Indicates the number of times the framebuffer was initialized
    uInt32 myInitializedCount{0};

    // Maximum dimensions of each attached display desktop area
    // Note that this takes 'hidpi' mode into account, so in some cases
    // it will be less than the absolute desktop size
    std::unordered_map<uInt32, Common::Size> myDesktopSize;

    // Maximum absolute dimensions of each attached display desktop area
    std::unordered_map<uInt32, Common::Size> myAbsDesktopSize;

    // The resolution of each attached display in fullscreen mode
    // Windowed modes use myDesktopSize directly
    std::unordered_map<uInt32, Common::Size> myFullscreenDisplays;

    // The resolution of each attached display in windowed mode
    std::unordered_map<uInt32, Common::Size> myWindowedDisplays;

    // HiDPI settings of each attached display
    std::unordered_map<uInt32, bool> myHiDPIAllowed;
    std::unordered_map<uInt32, bool> myHiDPIEnabled;

    // Supported renderers
    VariantList myRenderers;

    // Flag for pending render
    bool myPendingRender{false};

    // Re-entrancy guard: true only while a backend is (re)creating its window
    // and renderer.  On X11, SDL pumps the event queue synchronously from
    // inside those calls and delivers WINDOW_EXPOSED, which fires the
    // EventHandlerSDL::resizeWatch hook and re-enters update().  For the
    // companion TIA window that would flush a renderer that does not exist yet
    // (segfault), so update() bails out while this is set.
    bool myInVideoMode{false};

    // The VideoModeHandler class takes responsibility for all video
    // mode functionality
    VideoModeHandler myVidModeHandler;
    VideoModeHandler::Mode myActiveVidMode;

    // Type of the frame buffer
    BufferType myBufferType{BufferType::None};

    // Parked video mode / buffer type for the inactive render target
    // (swapped with the live members above by setRenderTarget()).
    VideoModeHandler::Mode myOtherVidMode;
    BufferType myOtherBufferType{BufferType::None};

    // Deferred interactive-resize state: while the user drags the window
    // border, the current frame is stretched (see deferResize()) and the
    // actual rebuild is postponed until applyPendingResize() at drag settle
    bool myResizeActive{false};
    Common::Size myPendingResize;

    // A new window size arrived (live re-flow) and is waiting to be applied by
    // applyLiveResize()
    bool myLiveResizePending{false};

    // Last window minimum size forwarded to the backend (scaled), so an
    // unchanged minimum isn't re-applied on every layout() during a drag
    Common::Size myWindowMinSize;

  #ifdef GUI_SUPPORT
    // The font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myFont;

    // The info font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myInfoFont;

    // The font object to use when space is very limited
    unique_ptr<GUI::Font> mySmallFont;

    // The font object to use for the ROM launcher
    unique_ptr<GUI::Font> myLauncherFont;
  #endif  // GUI_SUPPORT

    // The TIASurface class takes responsibility for TIA rendering
    shared_ptr<TIASurface> myTIASurface;

    // The BezelSurface which blends over the TIA surface
    unique_ptr<Bezel> myBezel;

    // The FBMessageHandler class takes responsibility for all onscreen
    // message and frame-statistics overlay functionality
    FBMessageHandler myMsgHandler;

    bool myGrabMouse{false};

    // Minimum TIA zoom level that can be used for this framebuffer
    double myTIAMinZoom{2.};

    // Holds a reference to all the surfaces that have been created
    std::list<shared_ptr<FBSurface>> mySurfaceList;

    FullPaletteArray myFullPalette{0};
    // Holds UI palette data (for each variation)
    static UIPaletteArray ourStandardUIPalette, ourClassicUIPalette,
                          ourLightUIPalette, ourDarkUIPalette;
    // Holds disassembly palette data (independent of UI theme)
    static DisasmPaletteArray ourStandardDisasmPalette, ourDarkDisasmPalette;

  private:
    // Following constructors and assignment operators not supported
    FrameBuffer() = delete;
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;
};

#endif  // FRAME_BUFFER_HXX
