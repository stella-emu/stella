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

#ifndef FB_BACKEND_HXX
#define FB_BACKEND_HXX

class FBSurface;

#include <cstdlib>
#include <unordered_map>

#include "Rect.hxx"
#include "Variant.hxx"
#include "FrameBufferConstants.hxx"
#include "VideoModeHandler.hxx"
#include "bspf.hxx"

/**
  How the display server behaves while the user drags a window edge.  Every
  live-resize policy derives from these conditions, rather than re-testing the
  platform at each site.

  Resolved at runtime, not compile time, because Linux is not one platform
  here: one binary serves both X11 and Wayland and they do not agree.
*/
namespace LiveResize
{
  struct Conditions
  {
    string driver;
    bool blocksMainLoop{false};
    bool awaitsOurFrame{false};
    bool rescalesOurLastFrame{false};
    bool acksOnPresent{false};
    // Policy, not a platform fact; see suspendsVsync()
    bool suspendsVsync{true};
  };

  // Resolved by initialize(); read through the accessors below
  inline Conditions ourConditions;

  /**
    Override one condition from the environment, if that variable is set.
  */
  inline void forceCondition(const char* name, bool& condition)
  {
  #ifdef BSPF_WINDOWS  // MSVC deprecates getenv
    char* value = nullptr;
    size_t length = 0;
    if(_dupenv_s(&value, &length, name) == 0 && value != nullptr)
    {
      condition = *value != '0';
      free(value);  // NOLINT(cppcoreguidelines-no-malloc)
    }
  #else
    if(const char* const value = std::getenv(name))  // NOLINT(concurrency-mt-unsafe)
      condition = *value != '0';
  #endif
  }

  /**
    Resolve the conditions for the display server actually in use.  Call once,
    from the video backend, before any other subsystem reads them.  'driver' is
    SDL's video driver name ("x11", "wayland", "windows", "cocoa").
  */
  inline void initialize(string_view driver)
  {
    const bool isX11 = driver == "x11";

    ourConditions.driver = driver;
  #if defined(BSPF_WINDOWS) || defined(BSPF_MACOS)
    ourConditions.blocksMainLoop = true;
    ourConditions.awaitsOurFrame = false;
  #else
    ourConditions.blocksMainLoop = false;
    // Anything more exotic (offscreen, dummy, KMSDRM) takes the safe path
    ourConditions.awaitsOurFrame = isX11 || driver == "wayland";
  #endif
  #if defined(BSPF_MACOS)
    ourConditions.rescalesOurLastFrame = true;
  #else
    ourConditions.rescalesOurLastFrame = false;
  #endif
    ourConditions.acksOnPresent = isX11;
    ourConditions.suspendsVsync = true;

    // Debug overrides, so a regression can be bisected on the machine showing
    // it without a rebuild: STELLA_LIVERESIZE_<CONDITION>=0|1
    forceCondition("STELLA_LIVERESIZE_BLOCKS_MAIN_LOOP",
                   ourConditions.blocksMainLoop);
    forceCondition("STELLA_LIVERESIZE_AWAITS_OUR_FRAME",
                   ourConditions.awaitsOurFrame);
    forceCondition("STELLA_LIVERESIZE_RESCALES_LAST_FRAME",
                   ourConditions.rescalesOurLastFrame);
    forceCondition("STELLA_LIVERESIZE_ACKS_ON_PRESENT",
                   ourConditions.acksOnPresent);
    forceCondition("STELLA_LIVERESIZE_SUSPEND_VSYNC",
                   ourConditions.suspendsVsync);
  }

  /**
    A drag runs in an OS modal loop that blocks our main loop, so the re-flow
    is driven from the SDL event watch and no resize event may be dropped.
  */
  inline bool blocksMainLoop() { return ourConditions.blocksMainLoop; }

  /**
    The display server withholds the resized window until we present at the
    new size, so nothing stale or rescaled appears between our frames.  This
    does NOT imply slack: see acksOnPresent().
    Gates nothing today -- skipping work on the strength of it floated X11.
  */
  inline bool awaitsOurFrame() { return ourConditions.awaitsOurFrame; }

  /**
    Our last frame is rescaled to the window as it changes, so anything we do
    not present for is shown stale and resampled: every change needs a frame.
  */
  inline bool rescalesOurLastFrame()
    { return ourConditions.rescalesOurLastFrame; }

  /**
    Presenting is itself the acknowledgement that we have drawn the new size
    (X11 _NET_WM_SYNC_REQUEST, acked from X11_GL_SwapWindow).  Blocking the
    present therefore stalls the WM's half of the drag as well as our own: with
    vsync on, the ack waits a full refresh and the drag rate halves.  That makes
    this the strongest reason to suspend vsync, not a reason against it.
    Gates nothing today -- see suspendsVsync().
  */
  inline bool acksOnPresent() { return ourConditions.acksOnPresent; }

  /**
    Suspend vsync for the duration of a drag.  A policy, not a platform fact:
    true everywhere, because a blocked present costs a frame's budget on every
    platform, and a resize is infrequent and off the hot path, so the CPU it
    spends is worth the smoothness.  Kept overridable because turning it off is
    what reproduces the X11 float and the old Wayland behaviour.
  */
  inline bool suspendsVsync() { return ourConditions.suspendsVsync; }

  /**
    One-line summary of the resolved conditions, for the startup log.
  */
  inline string describe()
  {
    return std::format("LiveResize: driver={} blocksMainLoop={} "
        "awaitsOurFrame={} rescalesLastFrame={} acksOnPresent={} "
        "suspendsVsync={}",
        ourConditions.driver, ourConditions.blocksMainLoop,
        ourConditions.awaitsOurFrame, ourConditions.rescalesOurLastFrame,
        ourConditions.acksOnPresent, ourConditions.suspendsVsync);
  }
}  // namespace LiveResize

/**
  This class provides an interface/abstraction for platform-specific,
  framebuffer-related rendering operations.  Different graphical
  platforms will inherit from this.  For most ports that means SDL,
  but some (such as libretro) use their own graphical subsystem.

  @author  Stephen Anthony
*/
class FBBackend
{
  friend class FrameBuffer;

  public:
    FBBackend() = default;
    virtual ~FBBackend() = default;

    /**
      Transform from window to renderer coordinates, x direction
     */
    virtual int scaleX(int x) const = 0;

    /**
      Transform from window to renderer coordinates, y direction
     */
    virtual int scaleY(int y) const = 0;

  protected:
    /**
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.  Since several
      monitors may be attached, we need the resolution for all of them.

      @param fullscreenRes  Maximum resolution supported in fullscreen mode
      @param windowedRes    Maximum resolution supported in windowed mode
      @param renderers      List of renderer names (internal name -> end-user name)
    */
    virtual void queryHardware(std::unordered_map<uInt32, Common::Size>& fullscreenRes,
                               std::unordered_map<uInt32, Common::Size>& windowedRes,
                               VariantList& renderers) = 0;

    /**
      This method is called to change to the given video mode.

      @param mode   The video mode to use
      @param winIdx The display/monitor that the window last opened on
      @param winPos The position that the window last opened at

      @return  False on any errors, else true
    */
    virtual bool setVideoMode(const VideoModeHandler::Mode& mode,
                              uInt32 winIdx, const Common::Point& winPos) = 0;

    /**
      Make the window user-resizable (or not).  Only meaningful for desktop
      windowed UI modes (the launcher, the debugger and its companion TIA
      window).  Each such window's owner sets its own minimum size, separately.
    */
    virtual void setWindowResizable(bool resizable) { }

    /**
      Set the window's minimum size (in pixels).  Honored by most window
      managers to prevent the user shrinking the window below this size.
    */
    virtual void setWindowMinSize(const Common::Size& minSize) { }

    /**
      Resize this backend's window in place (no destroy/recreate), in
      pixels.  Used to grow a resizable window (the launcher or the
      debugger) whose minimum size has just increased past its current size
      (e.g. a live font change) -- the window's owner learns the new size
      back through the normal live-resize event path, exactly as it would
      from the user dragging the border.
    */
    virtual void resizeWindow(const Common::Size& size) { }

    /**
      Refresh cached window/renderer dimensions after the window has been
      resized externally (e.g. the user dragging the window border).
    */
    virtual void refreshDimensions() { }

    /**
      An interactive resize of this window has started / has settled.  A
      backend may trade away whatever it must to keep up for the duration,
      since a re-flow and a present happen per dragged frame.
    */
    virtual void beginLiveResize() { }
    virtual void endLiveResize() { }

    /**
      The platform window ID of this backend's window, or 0 if no window
      exists.  Used to route window-specific events (mouse, keyboard, close,
      resize) to the FrameBuffer that owns the targeted window when more than
      one window is open (e.g. the debugger's companion TIA window).
    */
    virtual uInt32 windowId() const { return 0; }

    /**
      Show or hide this backend's window without destroying it or its
      surfaces.  Used to toggle a secondary window on and off cheaply.
    */
    virtual void setWindowVisible(bool visible) { }

    /**
      Clear the framebuffer.
    */
    virtual void clear() = 0;

    /**
      Flush pending render commands without presenting.  Used to ensure a
      clean command-queue boundary before clear() in dialog rendering, so
      any preceding geometry from the previous frame lands in its own
      command queue rather than sharing a queue with the subsequent clear.
    */
    virtual void flush() = 0;

    /**
      Updates window title.

      @param title   The title of the application / window
    */
    virtual void setTitle(string_view title) = 0;

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    virtual void showCursor(bool show) = 0;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    virtual void grabMouse(bool grab) = 0;

    /**
      Enable/disable text events (distinct from single-key events).
    */
    virtual void enableTextEvents(bool enable) = 0;

    /**
      This method must be called after all drawing is done, and indicates
      that the buffers should be pushed to the physical screen.
    */
    virtual void renderToScreen() = 0;

    /**
      Answers if the display is currently in fullscreen mode.

      @return  Whether the display is actually in fullscreen mode
    */
    virtual bool fullScreen() const = 0;

    /**
      Retrieve the current display's refresh rate.
    */
    virtual int refreshRate() const = 0;

    /**
      Checks if the OS theme is set to light.
    */
    virtual bool isLightTheme() const = 0;

    /**
      Checks if the OS theme is set to dark.
    */
    virtual bool isDarkTheme() const = 0;

    /**
      Retrieve the R/G/B/A masks from the FrameBuffer backend renderer.
    */
    virtual uInt32 rMask() const = 0;
    virtual uInt32 gMask() const = 0;
    virtual uInt32 bMask() const = 0;
    virtual uInt32 aMask() const = 0;

    /**
      This method is called to get a copy of the viewable framebuffer area
      as a surface.  Note that this isn't the same as any internal surfaces
      that may be in use; it should return the actual data as it is currently
      seen onscreen.

      @return  The surface used to store the current framebuffer.
    */
    virtual const FBSurface& compositedSurface() = 0;

    /**
      This method is called to query if the current window is not
      centered or fullscreen.

      @return  True, if the current window is positioned
    */
    virtual bool isCurrentWindowPositioned() const = 0;

    /**
      This method is called to query the video hardware for position of
      the current window.

      @return  The position of the currently displayed window
    */
    virtual Common::Point getCurrentWindowPos() const = 0;

    /**
      This method is called to query the video hardware for the index
      of the display the current window is displayed on.

      @return  The current display id or a 0 if no window is displayed
    */
    virtual uInt32 getCurrentDisplayID() const = 0;

    /**
      This method is called to create a surface with the given attributes.

      @param w      The requested width of the new surface.
      @param h      The requested height of the new surface.
      @param inter  Interpolation mode
      @param data   If non-null, use the given data values as a static surface
    */
    virtual unique_ptr<FBSurface>
        createSurface(
          uInt32 w,
          uInt32 h,
          ScalingInterpolation inter = ScalingInterpolation::none,
          const uInt32* data = nullptr
    ) = 0;

    /**
      This method is called to provide information about the backend.
    */
    virtual string about() const = 0;

    /**
      Sends a text message to the native display system for onscreen
      notification.  Backends with their own notification channel (e.g.
      libretro) override this; the default is a no-op.
    */
    virtual void showMessage(string_view) { }

    /**
      Sends a gauge message to the native display system.  The default
      is a no-op; backends that support notifications should override both
      this and showMessage.
    */
    virtual void showGaugeMessage(string_view, string_view,
                                  float, float = 0.F, float = 100.F) { }

  private:
    // Following constructors and assignment operators not supported
    FBBackend(const FBBackend&) = delete;
    FBBackend(FBBackend&&) = delete;
    FBBackend& operator=(const FBBackend&) = delete;
    FBBackend& operator=(FBBackend&&) = delete;
};

#endif  // FB_BACKEND_HXX
