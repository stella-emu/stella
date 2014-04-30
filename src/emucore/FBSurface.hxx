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

#ifndef FBSURFACE_HXX
#define FBSURFACE_HXX

#include "bspf.hxx"
#include "Font.hxx"

/**
  This class is basically a thin wrapper around the video toolkit 'surface'
  structure.  We do it this way so the actual video toolkit won't be dragged
  into the depths of the codebase.  All drawing is done into FBSurfaces,
  which are then drawn into the FrameBuffer.  Each FrameBuffer-derived class
  is responsible for extending an FBSurface object suitable to the
  FrameBuffer type.

  @author  Stephen Anthony
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
    virtual void drawChar(const GUI::Font& font, uInt8 c, uInt32 x, uInt32 y,
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
      This method should be called to reset the surface to empty
      pixels / colour black.
    */
    virtual void invalidate() { }

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
        const GUI::Font& font, const string& s, int x, int y, int w,
        uInt32 color, TextAlignment align = kTextAlignLeft,
        int deltax = 0, bool useEllipsis = true);

  protected:
    /**
      This method answers the current position of the surface.
    */
//    virtual void getBufferPtr(uInt32& x, uInt32& y) const = 0;

};

#endif
