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

    /**
      The state belonging to one window. Every window (primary or a companion
      such as TiaWindow/MemViewWindow) is one of these, held in a stable-address
      collection so a DialogContainer can keep a direct reference to its own
      for its lifetime.
    */
    struct WindowState {
      // This window's backend, and the surfaces bound to it
      unique_ptr<FBBackend> backend;
      std::list<shared_ptr<FBSurface>> surfaceList;

      VideoModeHandler::Mode vidMode;
      BufferType bufferType{BufferType::None};

      // The latest size recorded by liveResize(), waiting for applyLiveResize()
      Common::Size pendingResize;
      bool liveResizePending{false};

      // Last minimum size forwarded to the backend (scaled), so an unchanged
      // minimum isn't re-applied on every layout() during a drag
      Common::Size minSize;

      // Set when something must be presented even if nothing is dirty
      bool pendingRender{false};

      // Force vsync off for a window that redraws every emulation frame
      bool vsyncAlwaysOff{false};

      // Whether this window is currently shown (always true for the primary)
      bool active{false};
    };

  public:
    explicit FrameBuffer(OSystem& osystem);
    ~FrameBuffer();

    /**
      The primary window's state. Exists from construction onwards, so it is
      safe for a DialogContainer to take its address at construction time
      (see DialogContainer::window()).
    */
    WindowState& primaryWindow() const { return myPrimaryWindow; }

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
      Handle a resize of a window that does not re-flow live (see liveResize()).
      Rebuilds the active video mode for the new window size and reloads all
      surfaces, *without* recreating the window.

      @param width   The new window width, in pixels
      @param height  The new window height, in pixels
    */
    void handleResize(int width, int height);

    /**
      Whether a window of this type may be resized by the user.  Such windows
      all re-flow live, rather than resizing immediately via handleResize().
    */
    static constexpr bool isResizable(BufferType type) {
      return type == BufferType::Launcher || type == BufferType::Debugger
          || type == BufferType::TiaWindow || type == BufferType::MemViewWindow;
    }

    /**
      Record the latest window size for a live, per-frame re-flow.  Returns true
      for a user-resizable window — the caller then applies it via
      applyLiveResize() and re-lays-out its dialogs; false otherwise, so the
      caller falls back to handleResize().

      @param width   The latest window width, in pixels
      @param height  The latest window height, in pixels
      @return  True if this window re-flows live
    */
    bool liveResize(int width, int height);

    /**
      If a live resize is pending for the given window, rebuild its UI at the
      recorded size and clear the flag.  Returns true if it applied (the
      caller then re-flows its dialogs), false if nothing was pending.
    */
    bool applyLiveResize(WindowState& win);

    /**
      An interactive resize has settled.  Lets the backend undo whatever it
      traded away to keep up with the drag.
    */
    void resizeSettled();

    /**
      Set the minimum size (in logical UI pixels) the current window may be
      resized to.  Used by resizeable UI dialogs to prevent the window being
      shrunk small enough to clip their content.

      @param size  The minimum size, in logical (unscaled) UI pixels
    */
    void setWindowMinSize(const Common::Size& size);

    /**
      Ensure a resizeable UI window (the launcher or the debugger) is at
      least the given size, growing it in place if not -- used when a live
      font change raises the content's minimum past the window's current
      size.  Also applies the size as the new minimum (see setWindowMinSize).
      Never shrinks a window the user has already sized larger, and never
      exceeds the desktop.  A no-op window resize (already big enough) costs
      nothing beyond the minimum-size update.

      @param minSize  The new minimum, in logical (unscaled) UI pixels
    */
    void growWindowTo(const Common::Size& minSize);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the TIA, any pending menus, etc.
    */
    void update(UpdateMode mode = UpdateMode::NONE);

  #ifdef GUI_SUPPORT
    /**
      Secondary-window support.  In addition to the primary window (launcher /
      emulation / main debugger), the FrameBuffer can drive any number of
      additional windows, each backed by its own FBBackend and each owned by
      its own DialogContainer (e.g. the debugger's companion TIA window).  All
      other state (palette, fonts, TIASurface) is shared, so a secondary
      window is *not* a separate FrameBuffer; only the window/renderer/surfaces
      differ.  A container gets its own WindowState (see DialogContainer::window())
      the first time this is called for it, and keeps it (hidden, not
      destroyed) across close/re-open.

      @param container  The DialogContainer rendered into the secondary window
      @param title      The secondary window title
      @param type       The BufferType (geometry/position key) for the window
      @param size       The secondary window size, in logical UI pixels
      @param minSize    The smallest size the user may drag it to
    */
    FBInitStatus openSecondaryWindow(DialogContainer& container,
                                     string_view title, BufferType type,
                                     Common::Size size, Common::Size minSize);

    /**
      Apply a user resize of the secondary window: rebuild its video mode at the
      new size and re-flow its dialogs, whose surfaces are bound to its backend.

      @param container  The DialogContainer rendered into the secondary window
      @param width      The latest window width, in pixels
      @param height     The latest window height, in pixels
      @return  True if a resize was applied
    */
    bool resizeSecondaryWindow(DialogContainer& container,
                               int width, int height);

    /**
      An interactive resize of the secondary window has settled.  The secondary
      counterpart of resizeSettled(): lets its backend undo whatever it traded
      away to keep up with the drag.

      @param container  The DialogContainer rendered into the secondary window
    */
    void settleSecondaryWindow(DialogContainer& container);

    /**
      Draw the secondary window's container and present it.  No-op if it isn't
      open.
    */
    void renderSecondaryWindow(DialogContainer& container,
                               UpdateMode mode = UpdateMode::REDRAW);

    /**
      Hide the secondary window (its backend/surfaces are kept for re-open).
    */
    void closeSecondaryWindow(DialogContainer& container);
  #endif  // GUI_SUPPORT

    /**
      Whether any secondary window is currently shown.
    */
    bool secondaryWindowOpen() const;

    /**
      The platform window ID of the primary window (0 if none).  Used to ignore
      window-specific events reported for any other window.
    */
    uInt32 primaryWindowId() const;

    /**
      The platform window ID of the (currently shown) secondary window, or 0 if
      none is.  Used to route window-specific events to it.
    */
    uInt32 secondaryWindowId() const;

    /**
      The user has moved a window: remember its position and display, under that
      window's own settings keys.  Ignored for a window we do not own.

      @param windowId  The platform window ID reported by the move event
    */
    void saveWindowPosition(uInt32 windowId) const;

    /**
      There is a dedicated update method for emulation mode.
    */
    void updateInEmulationMode(float framesPerSecond);

    /**
      Set the given window's pending-rendering flag.
    */
    void setPendingRender(WindowState& win) { win.pendingRender = true; }

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
      Allocate a new surface, bound to the given window.  The FrameBuffer
      class takes all responsibility for freeing this surface (ie, other
      classes must not delete it directly).

      @param win    The window this surface belongs to (see Dialog::window(),
                     or primaryWindow() for a permanently primary-only caller)
      @param w      The requested width of the new surface
      @param h      The requested height of the new surface
      @param inter  Interpolation mode
      @param data   If non-null, use the given data values as a static surface

      @return  A pointer to a valid surface object, or nullptr
    */
    shared_ptr<FBSurface> allocateSurface(WindowState& win, int w, int h,
      ScalingInterpolation inter = ScalingInterpolation::none,
      const uInt32* data = nullptr);

    /**
      Deallocate a previously allocated surface.  If no such surface exists,
      this method does nothing.

      @param win      The window this surface belongs to
      @param surface  The surface to remove/deallocate
    */
    void deallocateSurface(WindowState& win, const shared_ptr<FBSurface>& surface);

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
      Returns the given window's image dimensions. Note that this takes into
      account the current scaling (if any) as well as image 'centering'.
      Pass primaryWindow() for a permanently primary-only caller.
    */
    const Common::Rect& imageRect(const WindowState& win) const { return win.vidMode.imageR; }

    /**
      Returns the given window's dimensions: the entire area containing the
      image as well as any 'unusable' area.
    */
    const Common::Size& screenSize(const WindowState& win) const { return win.vidMode.screenS; }
    const Common::Rect& screenRect(const WindowState& win) const { return win.vidMode.screenR; }

    /**
      Returns the dimensions of the mode specific users' desktop, or if
      BufferType::None, return the dimensions of the current active screen.
    */
    const Common::Size& desktopSize(BufferType bufferType = BufferType::None) const {
      return myDesktopSize.at(displayId(myPrimaryWindow, bufferType));
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
    double supportedTIAMinZoom() const { return myTIAMinZoom * hidpiScaleFactor(myPrimaryWindow); }
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
      return myPrimaryWindow.backend->compositedSurface();
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
    bool hidpiAllowed() const { return myHiDPIAllowed.at(displayId(myPrimaryWindow)); }

    /**
      Answer whether hidpi mode is enabled.  In this mode, all FBSurfaces
      are scaled to 2x normal size.
    */
    bool hidpiEnabled() const { return myHiDPIEnabled.at(displayId(myPrimaryWindow)); }
    // Scale factor for the given window; pass primaryWindow() for a
    // permanently primary-only caller
    uInt32 hidpiScaleFactor(const WindowState& win) const { return myHiDPIEnabled.at(displayId(win)) ? 2 : 1; }

    /**
      Re-evaluate the 'hidpi' setting for every attached display and, if the
      scale factor of the current one has changed, rebuild the window and
      re-flow the UI at the new scale.  Call after changing the setting.
    */
    void refreshHiDPI();

    /**
      This method should be called to save the current settings of all
      its subsystems.  Note that the this may be called when the class
      hasn't been fully initialized, so we first need to check if the
      subsytems actually exist.
    */
    void saveConfig(Settings& settings) const;

  #ifdef GUI_SUPPORT
    /**
      Get the font object(s) used by the UI.  The fonts are owned by the
      FontManager; these are here because the UI classes reach their fonts
      through the framebuffer
    */
    const GUI::Font& font() const;
    const GUI::Font& infoFont() const;
    const GUI::Font& smallFont() const;
    const GUI::Font& launcherFont() const;
  #endif  // GUI_SUPPORT

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    void showCursor(bool show) { myPrimaryWindow.backend->showCursor(show); }

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() const { return myPrimaryWindow.backend->fullScreen(); }

    /**
      Updates theme according to OS setting.

      @return  true if theme has changed
    */
    bool updateTheme();

    /**
      Retrieve the R/G/B/A masks from the FrameBuffer backend renderer.
    */
    uInt32 rMask() const { return myPrimaryWindow.backend->rMask(); }
    uInt32 gMask() const { return myPrimaryWindow.backend->gMask(); }
    uInt32 bMask() const { return myPrimaryWindow.backend->bMask(); }
    uInt32 aMask() const { return myPrimaryWindow.backend->aMask(); }

    /**
      Clear the framebuffer.
    */
    void clear() { myPrimaryWindow.backend->clear(); }
    void flush() { myPrimaryWindow.backend->flush(); }

    /**
      Transform from window to renderer coordinates, x/y direction.
     */
    int scaleX(int x) const { return myPrimaryWindow.backend->scaleX(x); }
    int scaleY(int y) const { return myPrimaryWindow.backend->scaleY(y); }

  private:
    /**
      These methods are used to load/save position and display of a window,
      named explicitly by its buffer type. Every caller supplies the type
      directly; there is no default.
    */
    string getPositionKey(BufferType bufferType) const;
    string getDisplayKey(BufferType bufferType) const;

    /**
      Save the given window's position and display under that window's own
      settings keys.
    */
    void saveCurrentWindowPosition(const WindowState& win) const;
    void savePosition(const FBBackend& backend, BufferType type) const;

    /**
      Work out the desktop size of every attached display, along with whether
      HiDPI mode is allowed and wanted there.  Runs at startup and again
      whenever the 'hidpi' setting changes.
    */
    void computeDesktopSizes();

    /**
      Answer whether HiDPI mode is wanted on a desktop of the given size.  The
      'hidpi' setting is either an explicit boolean, or 'auto': enabled on a
      very high resolution screen, and then only while the smallest UI Stella
      can build covers too little of it to read comfortably.
    */
    bool wantsHiDPI(const Common::Size& desktop) const;

    /**
      Frees and reloads all surfaces that the given window knows about.
    */
    void resetSurfaces(WindowState& win);

    /**
      Renders TIA and overlaying, optional bezel surface

      @param doClear  Clear the framebuffer before rendering
      @param shade    Shade the TIA surface after rendering
    */
    //void renderTIA(bool shade = false, bool doClear = true);
    void renderTIA(bool doClear = true, bool shade = false);

    /**
      Get the display used for the given window's mode.
    */
    uInt32 displayId(const WindowState& win, BufferType bufferType = BufferType::None) const;

    /**
      Build an applicable video mode based on the current settings in
      effect, whether TIA mode is active, etc.  Then tell the given
      window's backend to actually use the new mode.

      @return  Whether the operation succeeded or failed
    */
    FBInitStatus applyVideoMode(WindowState& win);

    /**
      Calculate the maximum level by which the base window can be zoomed and
      still fit in the desktop screen.
    */
    double maxWindowZoom() const;

    /**
      Enables/disables fullscreen mode for the given window.
    */
    void setFullscreen(WindowState& win, bool enable);

  #ifdef GUI_SUPPORT
    /**
      Determine the minimal TIA zoom level from the dialog font, so that what
      fits with the reference font also fits with a larger one
    */
    void setupTIAMinZoom();
  #endif  // GUI_SUPPORT

  #ifdef GUI_SUPPORT
    /**
      Draw a DialogContainer into the given window and present it.
    */
    void updateContainer(WindowState& win, DialogContainer& container, UpdateMode mode);
  #endif  // GUI_SUPPORT

    /**
      Window-scoped versions of the public API above.  Each public method
      forwards to one of these, naming the primary window explicitly.  The two
      that a dialog also needs for a non-primary window (allocateSurface,
      deallocateSurface) have their own public overloads above instead.
    */
    FBInitStatus createDisplay(WindowState& win, string_view title, BufferType type,
                               Common::Size size, bool honourHiDPI = true);
    void handleResize(WindowState& win, int width, int height);
    bool liveResize(WindowState& win, int width, int height);
    void resizeSettled(WindowState& win);
    void setWindowMinSize(WindowState& win, const Common::Size& size);
    void growWindowTo(WindowState& win, const Common::Size& minSize);
    void update(WindowState& win, UpdateMode mode);
    void updateInEmulationMode(WindowState& win, float framesPerSecond);
    void toggleFullscreen(WindowState& win, bool toggle);
  #ifdef ADAPTABLE_REFRESH_SUPPORT
    void toggleAdaptRefresh(WindowState& win, bool toggle);
  #endif
    void switchVideoMode(WindowState& win, int direction);
    void toggleBezel(WindowState& win, bool toggle);
    void setCursorState(WindowState& win);
    void enableTextEvents(WindowState& win, bool enable);
    bool updateTheme(WindowState& win);

  private:
    // The parent system for the framebuffer
    OSystem& myOSystem;

    // Every window (primary and any secondaries), stable addresses so a
    // DialogContainer can keep a direct reference to its own for its
    // lifetime.  Shared state (palette, fonts, TIASurface) stays below.
    std::list<WindowState> myWindows;
    // The first entry in myWindows, inserted by the constructor
    WindowState& myPrimaryWindow;

    // Indicates the number of times the framebuffer was initialized
    uInt32 myInitializedCount{0};

    // Builds a window's video mode; shared, since each window configures it
    // immediately before building its own mode (the result lives per-window)
    VideoModeHandler myVidModeHandler;

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

    // Re-entrancy guard: true only while a backend is (re)creating its window
    // and renderer.  On X11, SDL pumps the event queue synchronously from
    // inside those calls and delivers WINDOW_EXPOSED, which fires the
    // EventHandlerSDL::resizeWatch hook and re-enters update().  For the
    // companion TIA window that would flush a renderer that does not exist yet
    // (segfault), so update() bails out while this is set.
    bool myInVideoMode{false};

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
