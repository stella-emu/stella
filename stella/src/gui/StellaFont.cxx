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
// $Id: StellaFont.cxx,v 1.2 2005-04-29 19:05:06 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "FontData.hxx"
#include "StellaFont.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaFont::StellaFont(FrameBuffer* buffer)
    : myFrameBuffer(buffer)
{
  const FontDesc desc = {
    "04b-16b-10",
    9,
    10,
    8,
    33,
    94,
    _font_bits,
    0,  /* no encode table*/
    _sysfont_width,
    33,
    sizeof(_font_bits)/sizeof(uInt16)
  };

  myFontDesc = desc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 StellaFont::getCharWidth(uInt8 chr) const
{
  // If no width table is specified, return the maximum width
  if(!myFontDesc.width)
    return myFontDesc.maxwidth;

  // If this character is not included in the font, use the default char.
  if(chr < myFontDesc.firstchar || myFontDesc.firstchar + myFontDesc.size < chr)
  {
    if(chr == ' ')
      return myFontDesc.maxwidth / 2;
    chr = myFontDesc.defaultchar;
  }

  return myFontDesc.width[chr - myFontDesc.firstchar];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 StellaFont::getStringWidth(const string& str) const
{
  Int32 space = 0;

  for(uInt32 i = 0; i < str.size(); ++i)
    space += getCharWidth(str[i]);

  return space;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaFont::drawString(const string& s, uInt32 x, uInt32 y, uInt32 w,
                            OverlayColor color, TextAlignment align,
                            Int32 deltax, bool useEllipsis) const
{
  const uInt32 leftX = x, rightX = x + w;
  uInt32 i;
  uInt32 width = getStringWidth(s);
  string str;
	
  if(useEllipsis && width > w)
  {
    // String is too wide. So we shorten it "intelligently", by replacing
    // parts of it by an ellipsis ("..."). There are three possibilities
    // for this: replace the start, the end, or the middle of the string.
    // What is best really depends on the context; but unless we want to
    // make this configurable, replacing the middle probably is a good
    // compromise.
    const int ellipsisWidth = getStringWidth("...");
		
    // SLOW algorithm to remove enough of the middle. But it is good enough for now.
    const int halfWidth = (w - ellipsisWidth) / 2;
    int w2 = 0;
		
    for(i = 0; i < s.size(); ++i)
    {
      int charWidth = getCharWidth(s[i]);
      if(w2 + charWidth > halfWidth)
        break;

      w2 += charWidth;
      str += s[i];
    }

    // At this point we know that the first 'i' chars are together 'w2'
    // pixels wide. We took the first i-1, and add "..." to them.
    str += "...";
		
    // The original string is width wide. Of those we already skipped past
    // w2 pixels, which means (width - w2) remain.
    // The new str is (w2+ellipsisWidth) wide, so we can accomodate about
    // (w - (w2+ellipsisWidth)) more pixels.
    // Thus we skip ((width - w2) - (w - (w2+ellipsisWidth))) =
    // (width + ellipsisWidth - w)
    int skip = width + ellipsisWidth - w;
    for(; i < s.size() && skip > 0; ++i)
      skip -= getCharWidth(s[i]);

    // Append the remaining chars, if any
    for(; i < s.size(); ++i)
      str += s[i];

    width = getStringWidth(str);
  }
  else
    str = s;

  if(align == kTextAlignCenter)
    x = x + (w - width - 1)/2;
  else if(align == kTextAlignRight)
    x = x + w - width;

  x += deltax;
  for(i = 0; i < str.size(); ++i)
  {
    w = getCharWidth(str[i]);
    if(x+w > rightX)
      break;
    if(x >= leftX)
      myFrameBuffer->drawChar(str[i], x, y, color);

    x += w;
  }
}
