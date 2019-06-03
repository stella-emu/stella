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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include <map>

class OSystem;
class Console;
class Settings;
class FBSurface;
class TIASurface;

namespace GUI {
  class Font;
}

#include "Rect.hxx"
#include "Variant.hxx"
#include "TIAConstants.hxx"
#include "FrameBufferConstants.hxx"
#include "EventHandlerConstants.hxx"
#include "bspf.hxx"

/**
  This class encapsulates all video buffers and is the basis for the video
  display in Stella.  All graphics ports should derive from this class for
  platform-specific video stuff.

  The TIA is drawn here, and all GUI elements (ala ScummVM, which are drawn
  into FBSurfaces), are in turn drawn here as well.

  @author  Stephen Anthony
*/
class FrameBuffer
{
  public:
    // Contains all relevant info for the dimensions of a video screen
    // Also takes care of the case when the image should be 'centered'
    // within the given screen:
    //   'image' is the image dimensions into the screen
    //   'screen' are the dimensions of the screen itself
    struct VideoMode
    {
      enum class Stretch { Preserve, Fill, None };

      Common::Rect image;
      Common::Size screen;
      Stretch stretch;
      string description;
      float zoom;
      Int32 fsIndex;

      VideoMode();
      VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh, Stretch smode, float overscan = 1.0,
                const string& desc = "", float zoomLevel = 1, Int32 fsindex = -1);

      friend ostream& operator<<(ostream& os, const VideoMode& vm)
      {
        os << "image=" << vm.image << "  screen=" << vm.screen
           << "  stretch=" << (vm.stretch == Stretch::Preserve ? "preserve" :
                               vm.stretch == Stretch::Fill ? "fill" : "none")
           << "  desc=" << vm.description << "  zoom=" << vm.zoom
           << "  fsIndex= " << vm.fsIndex;
        return os;
      }
    };

    // Zoom level step interval
    static constexpr float ZOOM_STEPS = 0.25;

  public:
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer(OSystem& osystem);
    virtual ~FrameBuffer();

    /**
      Initialize the framebuffer object (set up the underlying hardware)
    */
    bool initialize();

    /**
      Set palette for user interface
    */
    void setUIPalette();

    /**
      (Re)creates the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the application / window
      @param width   The width of the framebuffer
      @param height  The height of the framebuffer
      @param honourHiDPI  If true, consult the 'hidpi' setting and enlarge
                          the display size accordingly; if false, use the
                          exact dimensions as given

      @return  Status of initialization (see FBInitStatus 'enum')
    */
    FBInitStatus createDisplay(const string& title, uInt32 width, uInt32 height,
                               bool honourHiDPI = true);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the TIA, any pending menus, etc.
    */
    void update(bool force = false);

    /**
      There is a dedicated update method for emulation mode.
     */
    void updateInEmulationMode(float framesPerSecond);

    /**
      Shows a message onscreen.

      @param message  The message to be shown
      @param position Onscreen position for the message
      @param force    Force showing this message, even if messages are disabled
    */
    void showMessage(const string& message,
                     MessagePosition position = MessagePosition::BottomCenter,
                     bool force = false);

    /**
      Toggles showing or hiding framerate statistics.
    */
    void toggleFrameStats();

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

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
      @param data  If non-null, use the given data values as a static surface

      @return  A pointer to a valid surface object, or nullptr.
    */
    shared_ptr<FBSurface> allocateSurface(int w, int h, const uInt32* data = nullptr);

    /**
      Returns the current dimensions of the framebuffer image.
      Note that this will take into account the current scaling (if any)
      as well as image 'centering'.
    */
    const Common::Rect& imageRect() const { return myImageRect; }

    /**
      Returns the current dimensions of the framebuffer window.
      This is the entire area containing the framebuffer image as well as any
      'unusable' area.
    */
    const Common::Size& screenSize() const { return myScreenSize; }
    const Common::Rect& screenRect() const { return myScreenRect; }

    /**
      Returns the current dimensions of the users' desktop.
    */
    const Common::Size& desktopSize() const { return myDesktopSize; }

    /**
      Get the supported renderers for the video hardware.

      @return  An array of supported renderers
    */
    const VariantList& supportedRenderers() const { return myRenderers; }

    /**
      Get the minimum/maximum supported TIA zoom level (windowed mode)
      for the framebuffer.
    */
    float supportedTIAMinZoom() const { return 2 * hidpiScaleFactor(); }
    float supportedTIAMaxZoom() const { return myTIAMaxZoom; }

    /**
      Get the TIA surface associated with the framebuffer.
    */
    TIASurface& tiaSurface() const { return *myTIASurface; }

    /**
      Enables/disables fullscreen mode.
    */
    void setFullscreen(bool enable);

    /**
      Toggles between fullscreen and window mode.
    */
    void toggleFullscreen();

    /**
      Changes the fullscreen overscan.
        direction = -1 means less overscan
        direction = +1 means more overscan
    */
    void changeOverscan(int direction);

    /**
      This method is called when the user wants to switch to the next
      available video mode.  In windowed mode, this typically means going to
      the next/previous zoom level.  In fullscreen mode, this typically means
      switching between normal aspect and fully filling the screen.
        direction = -1 means go to the next lower video mode
        direction = +1 means go to the next higher video mode

      @param direction  Described above
    */
    bool changeVidMode(int direction);

    /**
      Sets the state of the cursor (hidden or grabbed) based on the
      current mode.
    */
    void setCursorState();

    /**
      Sets the use of grabmouse.
    */
    void enableGrabMouse(bool enable);

    /**
      Sets the use of grabmouse.
    */
    bool grabMouseEnabled() const { return myGrabMouse; }

    /**
      Toggles the use of grabmouse (only has effect in emulation mode).
    */
    void toggleGrabMouse();

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param raw_palette  The array of colors in R/G/B format
    */
    void setPalette(const uInt32* raw_palette);

    /**
      Informs the Framebuffer of a change in EventHandler state.
    */
    void stateChanged(EventHandlerState state);

    /**
      Answer whether hidpi mode is allowed.  In this mode, all FBSurfaces
      are scaled to 2x normal size.
    */
    bool hidpiAllowed() const { return myHiDPIAllowed; }

    /**
      Answer whether hidpi mode is enabled.  In this mode, all FBSurfaces
      are scaled to 2x normal size.
    */
    bool hidpiEnabled() const { return myHiDPIEnabled; }
    uInt32 hidpiScaleFactor() const { return myHiDPIEnabled ? 2 : 1; }

  #ifdef GUI_SUPPORT
    /**
      Get the font object(s) of the framebuffer
    */
    const GUI::Font& font() const { return *myFont; }
    const GUI::Font& infoFont() const { return *myInfoFont; }
    const GUI::Font& smallFont() const { return *mySmallFont; }
    const GUI::Font& launcherFont() const { return *myLauncherFont; }
  #endif

  //////////////////////////////////////////////////////////////////////
  // The following methods are system-specific and can/must be
  // implemented in derived classes.
  //////////////////////////////////////////////////////////////////////
  public:
    /**
      Updates window title

      @param title   The title of the application / window
    */
    virtual void setTitle(const string& title) = 0;

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    virtual void showCursor(bool show) = 0;

    /**
      Answers if the display is currently in fullscreen mode.
    */
    virtual bool fullScreen() const = 0;

    /**
      This method is called to retrieve the R/G/B data from the given pixel.

      @param pixel  The pixel containing R/G/B data
      @param r      The red component of the color
      @param g      The green component of the color
      @param b      The blue component of the color
    */
    virtual void getRGB(uInt32 pixel, uInt8* r, uInt8* g, uInt8* b) const = 0;

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual uInt32 mapRGB(uInt8 r, uInt8 g, uInt8 b) const = 0;

    /**
      This method is called to get the specified ARGB data from the viewable
      FrameBuffer area.  Note that this isn't the same as any internal
      surfaces that may be in use; it should return the actual data as it
      is currently seen onscreen.

      @param buffer  The actual pixel data in ARGB8888 format
      @param pitch   The pitch (in bytes) for the pixel data
      @param rect    The bounding rectangle for the buffer
    */
    virtual void readPixels(uInt8* buffer, uInt32 pitch, const Common::Rect& rect) const = 0;

    /**
      This method is called to query the video hardware for the index
      of the display the current window is displayed on

      @return  the current display index or a negative value if no
               window is displayed
    */
    virtual Int32 getCurrentDisplayIndex() = 0;

    /**
      This method is called to preserve the last current windowed position.
    */
    virtual void updateWindowedPos() = 0;

    /**
      Clear the framebuffer.
    */
    virtual void clear() = 0;

  protected:
    /**
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.  Since several
      monitors may be attached, we need the resolution for all of them.

      @param fullscreenRes  Maximum resolution supported in fullscreen mode
      @param windowedRes    Maximum resolution supported in windowed mode
      @param renderers      List of renderer names (internal name -> end-user name)
    */
    virtual void queryHardware(vector<Common::Size>& fullscreenRes,
                               vector<Common::Size>& windowedRes,
                               VariantList& renderers) = 0;

    /**
      This method is called to change to the given video mode.

      @param title The title for the created window
      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    virtual bool setVideoMode(const string& title,
                              const FrameBuffer::VideoMode& mode) = 0;

    /**
      This method is called to create a surface with the given attributes.

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
      @param data  If non-null, use the given data values as a static surface
    */
    virtual unique_ptr<FBSurface>
        createSurface(uInt32 w, uInt32 h, const uInt32* data) const = 0;

    /**
      Calls 'free()' on all surfaces that the framebuffer knows about.
    */
    void freeSurfaces();

    /**
      Calls 'reload()' on all surfaces that the framebuffer knows about.
    */
    void reloadSurfaces();

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    virtual void grabMouse(bool grab) = 0;

    /**
      Set the icon for the main window.
    */
    virtual void setWindowIcon() = 0;

    /**
      This method must be called after all drawing is done, and indicates
      that the buffers should be pushed to the physical screen.
    */
    virtual void renderToScreen() = 0;

    /**
      This method is called to provide information about the FrameBuffer.
    */
    virtual string about() const = 0;

  protected:
    // The parent system for the framebuffer
    OSystem& myOSystem;

    // Color palette for TIA and UI modes
    uInt32 myPalette[256+kNumColors];

  private:
    /**
      Draw pending messages.

      @return  Indicates whether any changes actually occurred.
    */
    bool drawMessage();

    /**
      Frees and reloads all surfaces that the framebuffer knows about.
    */
    void resetSurfaces();

    /**
      Calculate the maximum level by which the base window can be zoomed and
      still fit in the given screen dimensions.
    */
    float maxZoomForScreen(uInt32 baseWidth, uInt32 baseHeight,
               uInt32 screenWidth, uInt32 screenHeight) const;

    /**
      Set all possible video modes (both windowed and fullscreen) available for
      this framebuffer based on given image dimensions and maximum window size.
    */
    void setAvailableVidModes(uInt32 basewidth, uInt32 baseheight);

    /**
      Returns an appropriate video mode based on the current eventhandler
      state, taking into account the maximum size of the window.

      @param fullscreen  Whether to use a windowed or fullscreen mode
      @return  A valid VideoMode for this framebuffer
    */
    const FrameBuffer::VideoMode& getSavedVidMode(bool fullscreen);

  private:
    /**
      This class implements an iterator around an array of VideoMode objects.
    */
    class VideoModeList
    {
      public:
        VideoModeList();
        VideoModeList(const VideoModeList&) = default;
        VideoModeList& operator=(const VideoModeList&) = default;

        void add(const FrameBuffer::VideoMode& mode);
        void clear();

        bool empty() const;
        uInt32 size() const;

        void previous();
        const FrameBuffer::VideoMode& current() const;
        void next();

        void setByZoom(float zoom);
        void setByStretch(FrameBuffer::VideoMode::Stretch stretch);

        friend ostream& operator<<(ostream& os, const VideoModeList& l)
        {
          for(const auto& vm: l.myModeList)
            os << "-----\n" << vm << endl << "-----\n";
          return os;
        }

      private:
        vector<FrameBuffer::VideoMode> myModeList;
        int myIdx;
    };

  protected:
    // Title of the main window/screen
    string myScreenTitle;

    // Number of displays
    int myNumDisplays;

    // The resolution of the attached displays in fullscreen mode
    // The primary display is typically the first in the array
    // Windowed modes use myDesktopSize directly
    vector<Common::Size> myFullscreenDisplays;

  private:
    // Draws the frame stats overlay
    void drawFrameStats(float framesPerSecond);

    // Indicates the number of times the framebuffer was initialized
    uInt32 myInitializedCount;

    // Used to set intervals between messages while in pause mode
    Int32 myPausedCount;

    // Dimensions of the actual image, after zooming, and taking into account
    // any image 'centering'
    Common::Rect myImageRect;

    // Dimensions of the main window (not always the same as the image)
    // Use 'size' version when only wxh are required
    // Use 'rect' version when x/y, wxh are required
    Common::Size myScreenSize;
    Common::Rect myScreenRect;

    // Maximum dimensions of the desktop area
    // Note that this takes 'hidpi' mode into account, so in some cases
    // it will be less than the absolute desktop size
    Common::Size myDesktopSize;

    // Maximum absolute dimensions of the desktop area
    Common::Size myAbsDesktopSize;

    // Supported renderers
    VariantList myRenderers;

  #ifdef GUI_SUPPORT
    // The font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myFont;

    // The info font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myInfoFont;

    // The font object to use when space is very limited
    unique_ptr<GUI::Font> mySmallFont;

    // The font object to use for the ROM launcher
    unique_ptr<GUI::Font> myLauncherFont;
  #endif

    // The TIASurface class takes responsibility for TIA rendering
    unique_ptr<TIASurface> myTIASurface;

    // Used for onscreen messages and frame statistics
    // (scanline count and framerate)
    struct Message {
      string text;
      int counter;
      int x, y, w, h;
      MessagePosition position;
      ColorId color;
      shared_ptr<FBSurface> surface;
      bool enabled;

      Message()
        : counter(-1), x(0), y(0), w(0), h(0), position(MessagePosition::BottomCenter),
          color(kNone), enabled(false) { }
    };
    Message myMsg;
    Message myStatsMsg;
    bool myStatsEnabled;
    uInt32 myLastScanlines;

    bool myGrabMouse;
    bool myHiDPIAllowed;
    bool myHiDPIEnabled;

    // The list of all available video modes for this framebuffer
    VideoModeList* myCurrentModeList;
    VideoModeList myWindowedModeList;
    vector<VideoModeList> myFullscreenModeLists;

    // Maximum TIA zoom level that can be used for this framebuffer
    float myTIAMaxZoom;

    // Holds a reference to all the surfaces that have been created
    vector<shared_ptr<FBSurface>> mySurfaceList;

    // Holds UI palette data (standard and classic colours)
    static uInt32 ourGUIColors[3][kNumColors-256];

  private:
    // Following constructors and assignment operators not supported
    FrameBuffer() = delete;
    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer& operator=(FrameBuffer&&) = delete;
};

#endif
