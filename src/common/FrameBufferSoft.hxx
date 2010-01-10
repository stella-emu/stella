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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FRAMEBUFFER_SOFT_HXX
#define FRAMEBUFFER_SOFT_HXX

#include <SDL.h>

class OSystem;
class RectList;

#include "bspf.hxx"
#include "FrameBuffer.hxx"


/**
  This class implements an SDL software framebuffer.

  @author  Stephen Anthony
  @version $Id$
*/
class FrameBufferSoft : public FrameBuffer
{
  friend class FBSurfaceSoft;

  public:
    /**
      Creates a new software framebuffer
    */
    FrameBufferSoft(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~FrameBufferSoft();

    //////////////////////////////////////////////////////////////////////
    // The following are derived from public methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      Enable/disable phosphor effect.
    */
    void enablePhosphor(bool enable, int blend);

    /**
      This method is called to retrieve the R/G/B data from the given pixel.

      @param pixel  The pixel containing R/G/B data
      @param r      The red component of the color
      @param g      The green component of the color
      @param b      The blue component of the color
    */
    void getRGB(Uint32 pixel, Uint8* r, Uint8* g, Uint8* b) const
      { SDL_GetRGB(pixel, myScreen->format, r, g, b); }

    /**
      This method is called to map a given R/G/B triple to the screen palette.

      @param r  The red component of the color.
      @param g  The green component of the color.
      @param b  The blue component of the color.
    */
    Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
      { return SDL_MapRGB(myScreen->format, r, g, b); }

    /**
      This method is called to query the type of the FrameBuffer.
    */
    BufferType type() const { return kSoftBuffer; }

    /**
      This method is called to get the specified scanline data.

      @param row  The row we are looking for
      @param data The actual pixel data (in bytes)
    */
    void scanline(uInt32 row, uInt8* data) const;

  protected:
    //////////////////////////////////////////////////////////////////////
    // The following are derived from protected methods in FrameBuffer.hxx
    //////////////////////////////////////////////////////////////////////
    /**
      This method is called to initialize the video subsystem
      with the given video mode.  Normally, it will also call setVidMode().

      @param mode  The video mode to use

      @return  False on any errors, else true
    */
    bool initSubsystem(VideoMode& mode);

    /**
      This method is called to change to the given video mode.  If the mode
      is successfully changed, 'mode' holds the actual dimensions used.

      @param mode  The video mode to use

      @return  False on any errors (in which case 'mode' is invalid), else true
    */
    bool setVidMode(VideoMode& mode);

    /**
      This method is called to create a surface compatible with the one
      currently in use, but having the given dimensions.

      @param w       The requested width of the new surface.
      @param h       The requested height of the new surface.
      @param useBase Use the base surface instead of creating a new one
    */
    FBSurface* createSurface(int w, int h, bool useBase = false) const;

    /**
      This method should be called anytime the TIA needs to be redrawn
      to the screen (full indicating that a full redraw is required).
    */
    void drawTIA(bool full);

    /**
      This method is called after any drawing is done (per-frame).
    */
    void postFrameUpdate();

    /**
      This method is called to provide information about the FrameBuffer.
    */
    string about() const;

  private:
    int myBytesPerPixel;
    int myBaseOffset;
    int myPitch;
    SDL_PixelFormat* myFormat;

    enum RenderType {
      kSoftZoom_16,
      kSoftZoom_24,
      kSoftZoom_32,
      kPhosphor_16,
      kPhosphor_24,
      kPhosphor_32
    };
    RenderType myRenderType;

    // Indicates if the TIA image has been modified
    bool myTiaDirty;
	 	 
    // Indicates if we're in a purely UI mode
    bool myInUIMode;

    // Used in the dirty update of rectangles in non-TIA modes
    RectList* myRectList;
};

/**
  A surface suitable for software rendering mode.

  @author  Stephen Anthony
  @version $Id$
*/
class FBSurfaceSoft : public FBSurface
{
  public:
    FBSurfaceSoft(const FrameBufferSoft& buffer, SDL_Surface* surface,
                  uInt32 w, uInt32 h, bool isBase);
    virtual ~FBSurfaceSoft();

    void hLine(uInt32 x, uInt32 y, uInt32 x2, uInt32 color);
    void vLine(uInt32 x, uInt32 y, uInt32 y2, uInt32 color);
    void fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, uInt32 color);
    void drawChar(const GUI::Font* font, uInt8 c, uInt32 x, uInt32 y, uInt32 color);
    void drawBitmap(uInt32* bitmap, uInt32 x, uInt32 y, uInt32 color, uInt32 h = 8);
    void drawPixels(uInt32* data, uInt32 x, uInt32 y, uInt32 numpixels);
    void drawSurface(const FBSurface* surface, uInt32 x, uInt32 y);
    void addDirtyRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    void getPos(uInt32& x, uInt32& y) const;
    void setPos(uInt32 x, uInt32 y);
    uInt32 getWidth() const  { return myWidth;  }
    uInt32 getHeight() const { return myHeight; }
    void setWidth(uInt32 w);
    void setHeight(uInt32 h);
    void translateCoords(Int32& x, Int32& y) const;
    void update();
    void free()   { }   // Not required for software mode
    void reload() { }   // Not required for software mode

  private:
    void recalc();
    inline void* getBasePtr(uInt32 x, uInt32 y) {
      return static_cast<void *>(static_cast<uInt8*>(mySurface->pixels) +
          (myYOffset + y) * mySurface->pitch + (myXOffset + x) *
          mySurface->format->BytesPerPixel);
    }

  private:
    const FrameBufferSoft& myFB;
    SDL_Surface* mySurface;
    uInt32 myWidth, myHeight;
    bool myIsBaseSurface;
    bool mySurfaceIsDirty;
    int myPitch;

    uInt32 myXOrig, myYOrig;
    uInt32 myXOffset, myYOffset;
};

#endif
