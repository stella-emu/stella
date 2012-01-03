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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
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
#include <SDL.h>

class FBSurface;
class OSystem;
class Console;
class Settings;

namespace GUI {
  class Font;
  struct Rect;
}

#include "EventHandler.hxx"
#include "Rect.hxx"
#include "StringList.hxx"
#include "bspf.hxx"

// Different types of framebuffer derived objects
enum BufferType {
  kSoftBuffer,
  kDoubleBuffer
};

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
    /**
      Creates a new Frame Buffer
    */
    FrameBuffer(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBuffer();

    /**
      (Re)initializes the framebuffer display.  This must be called before any
      calls are made to derived methods.

      @param title   The title of the window
      @param width   The width of the framebuffer
      @param height  The height of the framebuffer

      @return  Status of initialization (see FBInitStatus 'enum')
    */
    FBInitStatus initialize(const string& title, uInt32 width, uInt32 height);

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
      Allocate a new surface with a unique ID.  The FrameBuffer class takes
      all responsibility for freeing this surface (ie, other classes must not
      delete it directly).

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one

      @return  A unique ID used to identify this surface
    */
    int allocateSurface(int w, int h, bool useBase = false);

    /**
      Retrieve the surface associated with the given ID.

      @param id  The ID for the surface to retrieve.
      @return    A pointer to a valid surface object, or NULL.
    */
    FBSurface* surface(int id) const;

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
    const GUI::Rect& screenRect() const { return myScreenRect; }

    /**
      Refresh display according to the current state, taking single vs.
      double-buffered modes into account, and redrawing accordingly.
    */
    void refresh();

    /**
      Toggles between fullscreen and window mode.
      Grabmouse activated when in fullscreen mode.
    */
    void toggleFullscreen();

    /**
      Enables/disables fullscreen mode.
      Grabmouse activated when in fullscreen mode.  

      @param enable  Set the fullscreen mode to this value
    */
    void setFullscreen(bool enable);

    /**
      This method is called when the user wants to switch to the next available
      video mode (functionality depends on fullscreen or windowed mode).
      direction = -1 means go to the next lower video mode
      direction =  0 means to reload the current video mode
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
      Toggles the use of grabmouse (only has effect in emulation mode).
      The method changes the 'grabmouse' setting and saves it.
    */
    void toggleGrabMouse();

    /**
      Shows or hides the cursor based on the given boolean value.
    */
    virtual void showCursor(bool show);

    /**
      Answers if the display is currently in fullscreen mode.
    */
    bool fullScreen() const;

    /**
      Set the title for the main SDL window.
    */
    void setWindowTitle(const string& title);

    /**
      Get the supported TIA filters for the given framebuffer type.
    */
    const StringMap& supportedTIAFilters(const string& type);

    /**
      Get the TIA pixel associated with the given TIA buffer index,
      shifting by the given offset (for greyscale values).
    */
    uInt32 tiaPixel(uInt32 idx, uInt8 shift = 0) const;

    /**
      Set up the TIA/emulation palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    virtual void setTIAPalette(const uInt32* palette);

    /**
      Set up the user interface palette for a screen of any depth > 8.

      @param palette  The array of colors
    */
    virtual void setUIPalette(const uInt32* palette);

    /**
      Informs the Framebuffer of a change in EventHandler state.
    */
    void stateChanged(EventHandler::State state);

    /**
      Get the zoom level.
    */
    uInt32 getZoomLevel() { return myZoomLevel; }

  //////////////////////////////////////////////////////////////////////
  // The following methods are system-specific and must be implemented
  // in derived classes.
  //////////////////////////////////////////////////////////////////////
  public:
    /**
      Enable/disable phosphor effect.
    */
    virtual void enablePhosphor(bool enable, int blend) = 0;

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
      This method is called to query the type of the FrameBuffer.
    */
    virtual BufferType type() const = 0;

    /**
      This method is called to get the specified scanline data from the
      viewable FrameBuffer area.  Note that this isn't the same as any
      internal surfaces that may be in use; it should return the actual
      data as it is currently seen onscreen.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    virtual void scanline(uInt32 row, uInt8* data) const = 0;

  protected:
    // Different types of graphic filters to apply to the TIA image
    enum GfxID {
      GFX_Zoom1x,
      GFX_Zoom2x,
      GFX_Zoom3x,
      GFX_Zoom4x,
      GFX_Zoom5x,
      GFX_Zoom6x,
      GFX_Zoom7x,
      GFX_Zoom8x,
      GFX_Zoom9x,
      GFX_Zoom10x,
      GFX_NumModes
    };

    struct GraphicsMode {
      GfxID type;
      const char* name;
      const char* description;
      uInt32 zoom;
      uInt8 avail;  // 0x1 bit -> software, 0x2 bit -> opengl
    };

    // Contains all relevant info for the dimensions of an SDL screen
    // Also takes care of the case when the SDL image should be 'centered'
    // within the given screen
    //   image_XXX are the image offsets into the SDL screen
    //   screen_XXX are the dimensions of the SDL screen itself
    // Also contains relevant info for the graphics mode/filter to use
    // when rendering the image
    struct VideoMode {
      uInt32 image_x, image_y, image_w, image_h;
      uInt32 screen_w, screen_h;
      GraphicsMode gfxmode;
    };

    /**
      This method is called to initialize the video subsystem
      with the given video mode.  Normally, it will also call setVidMode().

      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    virtual bool initSubsystem(VideoMode& mode) = 0;

    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param mode  The video mode to use

      @return  False on any errors (in which case 'mode' is invalid), else true
    */
    virtual bool setVidMode(VideoMode& mode) = 0;

    /**
      This method is called to invalidate the contents of the entire
      framebuffer (ie, mark the current content as invalid, and erase it on
      the next drawing pass).
    */
    virtual void invalidate() = 0;

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    virtual FBSurface* createSurface(int w, int h, bool useBase = false) const = 0;

    /**
      This method should be called anytime the TIA needs to be redrawn
      to the screen (full indicating that a full redraw is required).
    */
    virtual void drawTIA(bool full) = 0;

    /**	 
      This method is called after any drawing is done (per-frame).	 
    */	 
    virtual void postFrameUpdate() = 0;

    /**
      This method is called to provide information about the FrameBuffer.
    */
    virtual string about() const = 0;

    /**
      Issues a 'free' and 'reload' instruction to all surfaces that the
      framebuffer knows about.
    */
    void resetSurfaces(FBSurface* tiasurface = (FBSurface*)0);

  protected:
    // The parent system for the framebuffer
    OSystem* myOSystem;

    // The SDL video buffer
    SDL_Surface* myScreen;

    // SDL initialization flags
    // This is set by the base FrameBuffer class, and read by the derived classes
    // If a FrameBuffer is successfully created, the derived classes must modify
    // it to point to the actual flags used by the SDL_Surface
    uInt32 mySDLFlags;

    // Indicates if the entire frame need to redrawn
    bool myRedrawEntireFrame;

    // Use phosphor effect (aka no flicker on 30Hz screens)
    bool myUsePhosphor;

    // Amount to blend when using phosphor effect
    int myPhosphorBlend;

    // TIA palettes for normal and phosphor modes
    // 'myDefPalette' also contains the UI palette
    // The '24' version of myDefPalette is used in 24-bit colour mode,
    // eliminating having to deal with endian and shift issues
    // Phosphor mode doesn't have a corresponding '24' mode, since it
    // would require a 192KB lookup table
    Uint32 myDefPalette[256+kNumColors];
    Uint32 myAvgPalette[256][256];
    Uint8 myDefPalette24[256+kNumColors][3];

    // Names of the TIA filters that can be used for this framebuffer
    StringMap myTIAFilters;

    // Holds the zoom level being used
    uInt32 myZoomLevel;

  private:
    /**
      Grabs or ungrabs the mouse based on the given boolean value.
    */
    void grabMouse(bool grab);

    /**
      Set the icon for the main SDL window.
    */
    void setWindowIcon();

    /**
      Draw pending messages.
    */
    void drawMessage();

    /**
      Used to calculate an averaged color for the 'phosphor' effect.

      @param c1  Color 1
      @param c2  Color 2

      @return  Averaged value of the two colors
    */
    uInt8 getPhosphor(uInt8 c1, uInt8 c2) const;

    /**
      Calculate the maximum level by which the base window can be zoomed and
      still fit in the given screen dimensions.
    */
    uInt32 maxWindowSizeForScreen(uInt32 baseWidth, uInt32 baseHeight,
                                  uInt32 screenWidth, uInt32 screenHeight);

    /**
      Set all possible video modes (both windowed and fullscreen) available for
      this framebuffer based on given image dimensions and maximum window size.
    */
    void setAvailableVidModes(uInt32 basewidth, uInt32 baseheight);

    /**
      Adds the given video mode to both windowed and fullscreen lists.
      In the case of fullscreen, we make sure a valid resolution exists.
    */
    void addVidMode(VideoMode& mode);

    /**
      Returns an appropriate video mode based on the current eventhandler
      state, taking into account the maximum size of the window.

      @return  A valid VideoMode for this framebuffer
    */
    VideoMode getSavedVidMode();

    /**
      Attempt to center the application window in windowed mode.
    */
    void centerAppWindow(const VideoMode& mode);

  private:
    /**
      This class implements an iterator around an array of VideoMode objects.
    */
    class VideoModeList
    {
      public:
        VideoModeList();
        ~VideoModeList();

        void add(VideoMode mode);
        void clear();

        bool isEmpty() const;
        uInt32 size() const;

        void previous();
        const FrameBuffer::VideoMode current(const Settings& settings,
                                             bool isFullscreen) const;
        void next();

        void setByGfxMode(GfxID id);
        void setByGfxMode(const string& name);
        void print();
        static void print(const VideoMode& mode);

      private:
        void set(const GraphicsMode& gfxmode);

      private:
        Common::Array<VideoMode> myModeList;
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

    // Dimensions of the SDL window (not always the same as the image)
    GUI::Rect myScreenRect;

    // Used for onscreen messages and frame statistics
    // (scanline count and framerate)
    struct Message {
      string text;
      int counter;
      int x, y, w, h;
      MessagePosition position;
      uInt32 color;
      FBSurface* surface;
      int surfaceID;
      bool enabled;
    };
    Message myMsg;
    Message myStatsMsg;

    // The list of all available video modes for this framebuffer
    VideoModeList myWindowedModeList;
    VideoModeList myFullscreenModeList;
    VideoModeList* myCurrentModeList;

    // Holds a reference to all the surfaces that have been created
    map<int,FBSurface*> mySurfaceList;

    // Holds static strings for the remap menu (emulation and menu events)
    static GraphicsMode ourGraphicsModes[GFX_NumModes];
};


/**
  This class is basically a thin wrapper around an SDL_Surface structure.
  We do it this way so the SDL stuff won't be dragged into the depths of
  the codebase.  All drawing is done into FBSurfaces, which are then
  drawn into the FrameBuffer.  Each FrameBuffer-derived class is
  responsible for extending an FBSurface object suitable to the
  FrameBuffer type.

  @author  Stephen Anthony
  @version $Id$
*/
// Text alignment modes for drawString()
enum TextAlignment {
  kTextAlignLeft,
  kTextAlignCenter,
  kTextAlignRight
};
// Line types for drawing rectangular frames
enum FrameStyle {
  kSolidLine,
  kDashLine
};

class FBSurface
{
  public:
    /**
      Creates a new FBSurface object
    */
    FBSurface() { }

    /**
      Destructor
    */
    virtual ~FBSurface() { }

    /**
      This method should be called to draw a horizontal line.

      @param x     The first x coordinate
      @param y     The y coordinate
      @param x2    The second x coordinate
      @param color The color of the line
    */
    virtual void hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color) { }

    /**
      This method should be called to draw a vertical line.

      @param x     The x coordinate
      @param y     The first y coordinate
      @param y2    The second y coordinate
      @param color The color of the line
    */
    virtual void vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color) { }

    /**
      This method should be called to draw a filled rectangle.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  
    */
    virtual void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          uInt32 color) { }

    /**
      This method should be called to draw the specified character.

      @param font   The font to use to draw the character
      @param c      The character to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
    */
    virtual void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y,
                          uInt32 color) { }

    /**
      This method should be called to draw the bitmap image.

      @param bitmap The data to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param color  The color of the character
      @param h      The height of the data image
    */
    virtual void drawBitmap(uInt32* bitmap, uInt32 x, uInt32 y, uInt32 color,
                            uInt32 h = 8) { }

    /**
      This method should be called to convert and copy a given row of pixel
      data into a FrameBuffer surface.  The pixels must already be in the
      format used by the surface.

      @param data     The data in uInt8 R/G/B format
      @param row      The row of the surface the data should be placed in
      @param rowbytes The number of bytes in row of 'data'
    */
    virtual void drawPixels(uInt32* data, uInt32 x, uInt32 y, uInt32 numpixels) { }

    /**
      This method should be called copy the contents of the given
      surface into the FrameBuffer surface.

      @param surface The data to draw
      @param x       The x coordinate
      @param y       The y coordinate
    */
    virtual void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y) { }

    /**
      This method should be called to add a dirty rectangle
      (ie, an area of the screen that has changed)

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
    */
    virtual void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h) { }

    /**
      This method answers the current position of the surface.
    */
    virtual void getPos(uInt32& x, uInt32& y) const { }

    /**
      This method should be called to set the position of the surface.
    */
    virtual void setPos(uInt32 x, uInt32 y) { }

    /**
      This method answers the current dimensions of the surface.
    */
    virtual uInt32 getWidth() const { return 0; }
    virtual uInt32 getHeight() const { return 0; }

    /**
      This method sets the width of the drawable area of the surface.
    */
    virtual void setWidth(uInt32 w) { }

    /**
      This method sets the width of the drawable area of the surface.
    */
    virtual void setHeight(uInt32 h) { }

    /**
      This method should be called to translate the given coordinates
      to the surface coordinates.

      @param x  X coordinate to translate
      @param y  Y coordinate to translate
    */
    virtual void translateCoords(Int32& x, Int32& y) const { }

    /**
      This method should be called to draw the surface to the screen.
    */
    virtual void update() { }

    /**
      This method should be called to free any resources being used by
      the surface.
    */
    virtual void free() { }

    /**
      This method should be called to reload the surface data/state.
      It will normally be called after free().
    */
    virtual void reload() { }

    /**
      This method should be called to draw a rectangular box with sides
      at the specified coordinates.

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the box
      @param h      The height of the box
      @param colorA Lighter color for outside line.
      @param colorB Darker color for inside line.
    */
    virtual void box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                     uInt32 colorA, uInt32 colorB);

    /**
      This method should be called to draw a framed rectangle.
      I'm not exactly sure what it is, so I can't explain it :)

      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the area
      @param h      The height of the area
      @param color  The color of the surrounding frame
    */
    virtual void frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                           uInt32 color, FrameStyle style = kSolidLine);

    /**
      This method should be called to draw the specified string.

      @param font   The font to draw the string with
      @param str    The string to draw
      @param x      The x coordinate
      @param y      The y coordinate
      @param w      The width of the string area
      @param h      The height of the string area
      @param color  The color of the text
      @param align  The alignment of the text in the string width area
      @param deltax 
      @param useEllipsis  Whether to use '...' when the string is too long
    */
    virtual void drawString(
        const GUI::Font* font, const string& str, int x, int y, int w,
        uInt32 color, TextAlignment align = kTextAlignLeft,
        int deltax = 0, bool useEllipsis = true);
};

#endif
