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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FRAMEBUFFER_HXX
#define FRAMEBUFFER_HXX

#include <map>

class OSystem;
class Console;
class Settings;

namespace GUI {
  class Font;
}

#include "EventHandler.hxx"
#include "Rect.hxx"
#include "Variant.hxx"
#include "FBSurface.hxx"
#include "TIASurface.hxx"
#include "bspf.hxx"

// Return values for initialization of framebuffer window
enum FBInitStatus {
  kSuccess,
  kFailComplete,
  kFailTooLarge,
  kFailNotSupported,
};

// Positions for onscreen/overlaid messages
enum MessagePosition {
  kTopLeft,
  kTopCenter,
  kTopRight,
  kMiddleLeft,
  kMiddleCenter,
  kMiddleRight,
  kBottomLeft,
  kBottomCenter,
  kBottomRight
};

// Colors indices to use for the various GUI elements
enum {
  kColor = 256,
  kBGColor,
  kShadowColor,
  kTextColor,
  kTextColorHi,
  kTextColorEm,
  kDlgColor,
  kWidColor,
  kWidFrameColor,
  kBtnColor,
  kBtnColorHi,
  kBtnTextColor,
  kBtnTextColorHi,
  kCheckColor,
  kScrollColor,
  kScrollColorHi,
  kSliderColor,
  kSliderColorHi,
  kDbgChangedColor,
  kDbgChangedTextColor,
  kDbgColorHi,
  kNumColors
};

// Contains all relevant info for the dimensions of a video screen
// Also takes care of the case when the image should be 'centered'
// within the given screen:
//   'image' is the image dimensions into the screen
//   'screen' are the dimensions of the screen itself
class VideoMode
{
  friend class FrameBuffer;

  public:
    GUI::Rect image;
    GUI::Size screen;
    Int32 fsIndex;
    uInt32 zoom;
    string description;

  public:
    VideoMode();
    VideoMode(uInt32 iw, uInt32 ih, uInt32 sw, uInt32 sh, Int32 full,
      uInt32 z = 1, const string& desc = "");

    friend ostream& operator<<(ostream& os, const VideoMode& vm)
    {
      os << "image=" << vm.image << "  screen=" << vm.screen
         << " full= " << vm.fsIndex << "  zoom=" << vm.zoom
         << "  desc=" << vm.description;
      return os;
    }

  private:
    void applyAspectCorrection(uInt32 aspect, bool stretch = false);
};

/**
  This class encapsulates all video buffers and is the basis for the video
  display in Stella.  All graphics ports should derive from this class for
  platform-specific video stuff.

  The TIA is drawn here, and all GUI elements (ala ScummVM, which are drawn
  into FBSurfaces), are in turn drawn here as well.

  @author  Stephen Anthony
  @version $Id$
*/
class FrameBuffer
{
  public:
    enum {
      kTIAMinW = 320u, kTIAMinH = 210u,
      kFBMinW  = 640u, kFBMinH  = 480u
    };

    /**
      Creates a new Frame Buffer
    */
    FrameBuffer(OSystem& osystem);

    /**
      Destructor
    */
    virtual ~FrameBuffer();

    /**
      Initialize the framebuffer object (set up the underlying hardware)
    */
    bool initialize();

    /**
      (Re)creates the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the application / window
      @param width   The width of the framebuffer
      @param height  The height of the framebuffer

      @return  Status of initialization (see FBInitStatus 'enum')
    */
    FBInitStatus createDisplay(const string& title, uInt32 width, uInt32 height);

    /**
      Updates the display, which depending on the current mode could mean
      drawing the TIA, any pending menus, etc.
    */
    void update();

    /**
      Shows a message onscreen.

      @param message  The message to be shown
      @param position Onscreen position for the message
      @param force    Force showing this message, even if messages are disabled
    */
    void showMessage(const string& message,
                     MessagePosition position = kBottomCenter,
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
    const GUI::Rect& imageRect() const { return myImageRect; }

    /**
      Returns the current dimensions of the framebuffer window.
      This is the entire area containing the framebuffer image as well as any
      'unusable' area.
    */
    const GUI::Size& screenSize() const { return myScreenSize; }

    /**
      Returns the current dimensions of the users' desktop.
    */
    const GUI::Size& desktopSize() const { return myDesktopSize; }

    /**
      Get the supported renderers for the video hardware.

      @return  An array of supported renderers
    */
    const VariantList& supportedRenderers() const { return myRenderers; }

    /**
      Get the supported TIA zoom levels (windowed mode) for the framebuffer.
    */
    const VariantList& supportedTIAZoomLevels() const { return myTIAZoomLevels; }

    /**
      Get the font object(s) of the framebuffer
    */
    const GUI::Font& font() const { return *myFont; }
    const GUI::Font& infoFont() const { return *myInfoFont; }
    const GUI::Font& smallFont() const { return *mySmallFont; }
    const GUI::Font& launcherFont() const { return *myLauncherFont; }

    /**
      Get the TIA surface associated with the framebuffer.
    */
    TIASurface& tiaSurface() const { return *myTIASurface; }

    /**
      Refresh display according to the current state, taking single vs.
      double-buffered modes into account, and redrawing accordingly.
    */
    void refresh();

    /**
      Enables/disables fullscreen mode.
    */
    void setFullscreen(bool enable);

    /**
      Toggles between fullscreen and window mode.
    */
    void toggleFullscreen();

    /**
      This method is called when the user wants to switch to the next available
      windowed video mode.
      direction = -1 means go to the next lower windowed video mode
      direction = +1 means go to the next higher windowed video mode

      @param direction  Described above
    */
    bool changeWindowedVidMode(int direction);

    /**
      Sets the state of the cursor (hidden or grabbed) based on the
      current mode.
    */
    void setCursorState();

    /**
      Toggles the use of grabmouse (only has effect in emulation mode).
      The method changes the 'grabmouse' setting and saves it.
    */
    void toggleGrabMouse();

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param palette  The array of colors in R/G/B format
    */
    void setPalette(const uInt32* raw_palette);

    /**
      Informs the Framebuffer of a change in EventHandler state.
    */
    void stateChanged(EventHandler::State state);

  //////////////////////////////////////////////////////////////////////
  // The following methods are system-specific and can/must be
  // implemented in derived classes.
  //////////////////////////////////////////////////////////////////////
  public:
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
    virtual void getRGB(Uint32 pixel, Uint8* r, Uint8* g, Uint8* b) const = 0;

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    virtual Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const = 0;

    /**
      This method is called to query the buffering type of the FrameBuffer.
    */
    virtual bool isDoubleBuffered() const = 0;

    /**
      This method is called to get the specified ARGB data from the viewable
      FrameBuffer area.  Note that this isn't the same as any internal
      surfaces that may be in use; it should return the actual data as it
      is currently seen onscreen.

      @param buffer  The actual pixel data in ARGB8888 format
      @param pitch   The pitch (in bytes) for the pixel data
      @param rect    The bounding rectangle for the buffer
    */
    virtual void readPixels(uInt8* buffer, uInt32 pitch, const GUI::Rect& rect) const = 0;

  protected:
    /**
      This method is called to query and initialize the video hardware
      for desktop and fullscreen resolution information.
    */
    virtual void queryHardware(vector<GUI::Size>& mons, VariantList& ren) = 0;

    virtual Int32 getCurrentDisplayIndex() = 0;

    /**
      This method is called to change to the given video mode.

      @param title The title for the created window
      @param mode  The video mode to use

      @param fullscreen_toggle  Indicate whether this video mode change is
                due to a fullscreen/windowed toggle or not; some backends
                can use this information to perform a more optimized mode
                change

      @return  False on any errors, else true
    */
    virtual bool setVideoMode(const string& title, const VideoMode& mode,
                              bool fullscreen_toggle = false) = 0;

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    virtual void invalidate() = 0;

    /**
      This method is called to create a surface with the given attributes.

      @param w     The requested width of the new surface.
      @param h     The requested height of the new surface.
      @param data  If non-null, use the given data values as a static surface
    */
    virtual unique_ptr<FBSurface> createSurface(uInt32 w, uInt32 h,
                                      const uInt32* data) const = 0;

    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    virtual void grabMouse(bool grab) = 0;

    /**
      Set the icon for the main window.
    */
    virtual void setWindowIcon() = 0;

    /**	 
      This method is called after any drawing is done (per-frame).	 
    */	 
    virtual void postFrameUpdate() = 0;

    /**
      This method is called to provide information about the FrameBuffer.
    */
    virtual string about() const = 0;

  protected:
    // The parent system for the framebuffer
    OSystem& myOSystem;

    // Color palette for TIA and UI modes
    Uint32 myPalette[256+kNumColors];

  private:
    /**
      Draw pending messages.
    */
    void drawMessage();

    /**
      This method should be called anytime the TIA needs to be redrawn
      to the screen.
    */
    void drawTIA();

    /**
      Issues a 'free' and 'reload' instruction to all surfaces that the
      framebuffer knows about.
    */
    void resetSurfaces();

    /**
      Calculate the maximum level by which the base window can be zoomed and
      still fit in the given screen dimensions.
    */
    uInt32 maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
               uInt32 screenWidth, uInt32 screenHeight) const;

    /**
      Set all possible video modes (both windowed and fullscreen) available for
      this framebuffer based on given image dimensions and maximum window size.
    */
    void setAvailableVidModes(uInt32 basewidth, uInt32 baseheight);

    /**
      Returns an appropriate video mode based on the current eventhandler
      state, taking into account the maximum size of the window.

      @param full  Whether to use a windowed or fullscreen mode
      @return      A valid VideoMode for this framebuffer
    */
    const VideoMode& getSavedVidMode(bool fullscreen);

  private:
    /**
      This class implements an iterator around an array of VideoMode objects.
    */
    class VideoModeList
    {
      public:
        VideoModeList();
        ~VideoModeList();

        void add(const VideoMode& mode);
        void clear();

        bool empty() const;
        uInt32 size() const;

        void previous();
        const VideoMode& current() const;
        void next();

        void setZoom(uInt32 zoom);

        friend ostream& operator<<(ostream& os, const VideoModeList& l)
        {
          for(const auto& vm: l.myModeList)
            os << "-----\n" << vm << endl << "-----\n";
          return os;
        }

      private:
        vector<VideoMode> myModeList;
        int myIdx;
    };

  private:
    // Indicates the number of times the framebuffer was initialized
    uInt32 myInitializedCount;

    // Used to set intervals between messages while in pause mode
    uInt32 myPausedCount;

    // Dimensions of the actual image, after zooming, and taking into account
    // any image 'centering'
    GUI::Rect myImageRect;

    // Dimensions of the main window (not always the same as the image)
    GUI::Size myScreenSize;

    // Title of the main window/screen
    string myScreenTitle;

    // Maximum dimensions of the desktop area
    GUI::Size myDesktopSize;

    // The resolution of the attached displays
    // The primary display is first in the array
    vector<GUI::Size> myDisplays;

    // Supported renderers
    VariantList myRenderers;

    // The font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myFont;

    // The info font object to use for the normal in-game GUI
    unique_ptr<GUI::Font> myInfoFont;

    // The font object to use when space is very limited
    unique_ptr<GUI::Font> mySmallFont;

    // The font object to use for the ROM launcher
    unique_ptr<GUI::Font> myLauncherFont;

    // The TIASurface class takes responsibility for TIA rendering
    unique_ptr<TIASurface> myTIASurface;

    // Used for onscreen messages and frame statistics
    // (scanline count and framerate)
    struct Message {
      string text;
      int counter;
      int x, y, w, h;
      MessagePosition position;
      uInt32 color;
      shared_ptr<FBSurface> surface;
      bool enabled;
    };
    Message myMsg;
    Message myStatsMsg;

    // The list of all available video modes for this framebuffer
    VideoModeList* myCurrentModeList;
    VideoModeList myWindowedModeList;
    vector<VideoModeList> myFullscreenModeLists;

    // Names of the TIA zoom levels that can be used for this framebuffer
    VariantList myTIAZoomLevels;

    // Holds a reference to all the surfaces that have been created
    vector<shared_ptr<FBSurface>> mySurfaceList;

    // Holds UI palette data (standard and classic colours)
    static uInt32 ourGUIColors[2][kNumColors-256];
};

#endif
