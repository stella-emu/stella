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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaFont.hxx,v 1.1 2005-03-13 03:38:41 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef STELLA_FONT_HXX
#define STELLA_FONT_HXX

class FrameBuffer;

#include "bspf.hxx"
#include "GuiUtils.hxx"

// Text alignment modes for drawString()
enum TextAlignment {
	kTextAlignLeft,
	kTextAlignCenter,
	kTextAlignRight
};

/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
struct FontDesc
{
  const char*   name;        /* font name */
  Int32         maxwidth;    /* max width in pixels */
  Int32         height;      /* height in pixels */
  Int32         ascent;      /* ascent (baseline) height */
  Int32         firstchar;   /* first character in bitmap */
  Int32         size;        /* font size in glyphs */
  const uInt16* bits;        /* 16-bit right-padded bitmap data */
  const uInt32* offset;      /* offsets into bitmap data */
  const uInt8*  width;       /* character widths or NULL if fixed */
  Int32         defaultchar; /* default char (not glyph index) */
  long          bits_size;   /* # words of bitmap_t bits */
};

class StellaFont
{
  public:
    StellaFont(FrameBuffer* buffer, const FontDesc& desc)
      : myFrameBuffer(buffer), myFontDesc(desc) {}
	
    const FontDesc& desc() { return myFontDesc; }

    Int32 getFontHeight() const { return myFontDesc.height; }
    Int32 getMaxCharWidth() const { return myFontDesc.maxwidth; }

    Int32 getCharWidth(uInt8 chr) const;

    Int32 getStringWidth(const string& str) const;
    void  drawString(const string& str, uInt32 x, uInt32 y, uInt32 w,
                     OverlayColor color, TextAlignment align = kTextAlignLeft,
                     Int32 deltax = 0, bool useEllipsis = true) const;

  protected:
    FrameBuffer* myFrameBuffer;
    FontDesc myFontDesc;
};

#endif
