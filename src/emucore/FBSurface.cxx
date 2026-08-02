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

#include <cmath>

#include "Rect.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Logger.hxx"

#ifdef GUI_SUPPORT
  #include "Font.hxx"
  #include "Icon.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::pixel(uInt32 x, uInt32 y, ColorId color)
{
  // Note: checkbounds() must be done in calling method
  uInt32* buffer = myPixels + (y * static_cast<size_t>(myPitch)) + x;

  *buffer = myPalette[color];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::line(uInt32 x, uInt32 y, uInt32 x2, uInt32 y2, ColorId color)
{
  if(!checkBounds(x, y) || !checkBounds(x2, y2))
    return;

  // draw line using Bresenham algorithm
  Int32 dx = (x2 - x);
  Int32 dy = (y2 - y);

  if(abs(dx) >= abs(dy))
  {
    // x is major axis
    if(dx < 0)
    {
      std::swap(x, x2);
      y = y2;
      dx = -dx;
      dy = -dy;
    }
    const Int32 yd = dy > 0 ? 1 : -1;
    dy = abs(dy);
    Int32 err = dx / 2;
    // now draw the line
    for(; x <= x2; ++x)
    {
      pixel(x, y, color);
      err -= dy;
      if(err < 0)
      {
        err += dx;
        y += yd;
      }
    }
  }
  else
  {
    // y is major axis
    if(dy < 0)
    {
      x = x2;
      std::swap(y, y2);
      dx = -dx;
      dy = -dy;
    }
    const Int32 xd = dx > 0 ? 1 : -1;
    dx = abs(dx);
    Int32 err = dy / 2;
    // now draw the line
    for(; y <= y2; ++y)
    {
      pixel(x, y, color);
      err -= dx;
      if(err < 0)
      {
        err += dy;
        x += xd;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::hLine(uInt32 x, uInt32 y, uInt32 x2, ColorId color)
{
  if(!checkBounds(x, y) || !checkBounds(x2, 2))
    return;

  // NOLINTNEXTLINE(misc-const-correctness)
  uInt32* buffer = myPixels + (y * static_cast<size_t>(myPitch)) + x;
  const uInt32 ink = myPalette[color];

  while(x++ <= x2)
    *buffer++ = ink;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::vLine(uInt32 x, uInt32 y, uInt32 y2, ColorId color)
{
  if(!checkBounds(x, y) || !checkBounds(x, y2))
    return;

  uInt32* buffer = myPixels + (y * static_cast<size_t>(myPitch)) + x;
  const uInt32 ink = myPalette[color];

  while(y++ <= y2)
  {
    *buffer = ink;
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::fillRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h, ColorId color)
{
  while(h--)
    hLine(x, y+h, x+w-1, color);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawChar(const GUI::Font& font, uInt8 chr,
                         uInt32 tx, uInt32 ty, ColorId color, ColorId shadowColor)
{
#ifdef GUI_SUPPORT
  if(shadowColor != kNone)
  {
    drawChar(font, chr, tx + 1, ty + 0, shadowColor);
    drawChar(font, chr, tx + 0, ty + 1, shadowColor);
    drawChar(font, chr, tx + 1, ty + 1, shadowColor);
  }

  // The font hands out the glyph's mask and where to put it, so how a glyph
  // is stored -- and how wide it is -- stays the font's business, not ours
  const GUI::Glyph glyph = font.glyph(chr);
  if(glyph.mask == nullptr)
    return;

  const uInt32 cx = tx + glyph.dx;
  const uInt32 cy = ty + glyph.dy;

  if(!checkBounds(cx , cy) || !checkBounds(cx + glyph.w - 1, cy + glyph.h - 1))
    return;

  const uInt8* mask = glyph.mask;
  uInt32* buffer = myPixels + (cy * static_cast<size_t>(myPitch)) + cx;
  const uInt32 ink = myPalette[color];

  for(int y = 0; y < glyph.h; ++y)
  {
    for(int x = 0; x < glyph.w; ++x)
      if(mask[x >> 3] & (0x80 >> (x & 7)))
        buffer[x] = ink;

    mask += glyph.stride;
    buffer += myPitch;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawIcon(const GUI::Icon& icon, uInt32 tx, uInt32 ty,
                         ColorId color)
{
#ifdef GUI_SUPPORT
  const uInt32 w = icon.width(), h = icon.height();

  if(!checkBounds(tx, ty) || !checkBounds(tx + w - 1, ty + h - 1))
    return;

  const uInt32* rows = icon.bitmap();
  uInt32* buffer = myPixels + (ty * static_cast<size_t>(myPitch)) + tx;
  const uInt32 ink = myPalette[color];

  for(uInt32 y = 0; y < h; ++y)
  {
    uInt32 mask = 1 << (w - 1);

    for(uInt32 x = 0; x < w; ++x, mask >>= 1)
      if(rows[y] & mask)
        buffer[x] = ink;

    buffer += myPitch;
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawArrow(uInt32 tx, uInt32 ty, uInt32 w, uInt32 h,
                          ArrowDirection dir, ColorId color, uInt32 thickness)
{
  if(w == 0 || h == 0)
    return;
  if(!checkBounds(tx, ty) || !checkBounds(tx + w - 1, ty + h - 1))
    return;

  const int aw = static_cast<int>(w), ah = static_cast<int>(h),
            thick = static_cast<int>(thickness);
  uInt32* buffer = myPixels + (ty * static_cast<size_t>(myPitch)) + tx;
  const uInt32 ink = myPalette[color];

  for(int y = 0; y < ah; ++y)
  {
    // How far down the arrow this row is, counting from the tip
    const int t = (dir == ArrowDirection::Up) ? y : ah - 1 - y;

    // The row's edges, measured in TWICE the distance from the centre line so
    // that odd and even widths both come out exact with no rounding.  A filled
    // arrow spans its box; a stroked one runs at 45 degrees and clips to it
    const int outer = thick ? std::min(aw - 1, 2 * t)
                            : (ah > 1 ? ((aw - 1) * t) / (ah - 1) : aw - 1);
    const int inner = thick ? std::max(0, (2 * t) - (2 * (thick - 1))) : 0;

    for(int x = 0; x < aw; ++x)
    {
      const int d = std::abs((2 * x) - (aw - 1));

      if(d >= inner && d <= outer + 1)
        buffer[x] = ink;
    }
    buffer += myPitch;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::drawPixels(const uInt32* data, uInt32 tx, uInt32 ty, uInt32 numpixels)
{
  if(!checkBounds(tx, ty) || !checkBounds(tx + numpixels - 1, ty))
    return;

  // NOLINTNEXTLINE(misc-const-correctness)
  uInt32* buffer = myPixels + (ty * static_cast<size_t>(myPitch)) + tx;

  for(uInt32 i = 0; i < numpixels; ++i)
    *buffer++ = data[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::box(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                    ColorId colorA, ColorId colorB)
{
  hLine(x + 1, y,     x + w - 2, colorA);
  hLine(x,     y + 1, x + w - 1, colorA);
  vLine(x,     y + 1, y + h - 2, colorA);
  vLine(x + 1, y,     y + h - 1, colorA);

  hLine(x + 1,     y + h - 2, x + w - 1, colorB);
  hLine(x + 1,     y + h - 1, x + w - 2, colorB);
  vLine(x + w - 1, y + 1,     y + h - 2, colorB);
  vLine(x + w - 2, y + 1,     y + h - 1, colorB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::frameRect(uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                          ColorId color, FrameStyle style)
{
  switch(style)
  {
    case FrameStyle::Solid:
      hLine(x,         y,         x + w - 1, color);
      hLine(x,         y + h - 1, x + w - 1, color);
      vLine(x,         y,         y + h - 1, color);
      vLine(x + w - 1, y,         y + h - 1, color);
      break;

    case FrameStyle::Dashed:
      for(uInt32 i = x; i < x + w; i += 2)
      {
        hLine(i, y, i, color);
        hLine(i, y + h - 1, i, color);
      }
      for(uInt32 i = y; i < y + h; i += 2)
      {
        vLine(x, i, i, color);
        vLine(x + w - 1, i, i, color);
      }
      break;

    default:
      break;  // Not supposed to get here
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FBSurface::splitString(const GUI::Font& font, string_view s, int w,
                            string& left, string& right)
{
#ifdef GUI_SUPPORT
  uInt32 pos = 0;
  int w2 = 0;
  bool split = false;

  // SLOW algorithm to find the acceptable length. But it is good enough for now.
  for(pos = 0; pos < s.size(); ++pos)
  {
    const int charWidth = font.getCharWidth(s[pos]);
    if(w2 + charWidth > w || s[pos] == '\n')
    {
      split = true;
      break;
    }
    w2 += charWidth;
  }

  if(split)
    for(int i = pos; i > 0; --i)
    {
      if(isWhiteSpace(s[i]))
      {
        left = s.substr(0, i);
        if(s[i] == ' ' || s[pos] == '\n') // skip leading space after line break
          i++;
        right = s.substr(i);
        return;
      }
    }
  left = s.substr(0, pos);
  right = s.substr(pos);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FBSurface::drawString(const GUI::Font& font, string_view s,
                          int x, int y, int w, int h,
                          ColorId color, TextAlign align,
                          int deltax, bool useEllipsis, ColorId shadowColor,
                          size_t linkStart, size_t linkLen, bool underline)
{
  int lines = 0;

#ifdef GUI_SUPPORT
  string inStr{s};

  // draw multiline string
  while(!inStr.empty() && h >= font.getFontHeight() * 2)
  {
    // String is too wide.
    string leftStr, rightStr;

    splitString(font, inStr, w, leftStr, rightStr);
    drawString(font, leftStr, x, y, w, color, align, deltax, false, shadowColor,
               linkStart, linkLen, underline);
    if(linkStart != string::npos)
      linkStart = std::max(0, static_cast<int>(linkStart - leftStr.length()));

    h -= font.getFontHeight();
    y += font.getFontHeight();
    inStr = rightStr;
    lines++;
  }
  if(!inStr.empty())
  {
    drawString(font, inStr, x, y, w, color, align, deltax, useEllipsis, shadowColor,
               linkStart, linkLen, underline);
    lines++;
  }
#endif
  return lines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FBSurface::drawString(const GUI::Font& font, string_view s,
                          int x, int y, int w,
                          ColorId color, TextAlign align,
                          int deltax, bool useEllipsis, ColorId shadowColor,
                          size_t linkStart, size_t linkLen, bool underline)
{
#ifdef GUI_SUPPORT
  const string ELLIPSIS = "\x1d"; // "..."
  const int leftX = x, rightX = x + w;
  int width = font.getStringWidth(s);
  string str;

  if(useEllipsis && width > w)
  {
    // String is too wide. So we shorten it "intelligently", by replacing
    // parts of it by an ellipsis ("..."). There are three possibilities
    // for this: replace the start, the end, or the middle of the string.
    // What is best really depends on the context; but most applications
    // replace the end. So we use that too.
    int w2 = font.getStringWidth(ELLIPSIS);

    // SLOW algorithm to find the acceptable length. But it is good enough for now.
    for(auto c: s)
    {
      const int charWidth = font.getCharWidth(c);
      if(w2 + charWidth > w)
        break;

      w2 += charWidth;
      str += c;
    }
    str += ELLIPSIS;

    width = font.getStringWidth(str);
  }
  else
    str = s;

  if(align == TextAlign::Center)
    x = x + (w - width - 1)/2;
  else if(align == TextAlign::Right)
    x = x + w - width;

  x += deltax;

  int x0 = x, x1 = 0;

  for(auto i = 0UZ; i < str.size(); ++i)
  {
    w = font.getCharWidth(str[i]);
    if(x + w > rightX)
      break;
    if(x >= leftX)
    {
      if(i == linkStart)
        x0 = x;
      else if(i < linkStart + linkLen)
        x1 = x + w;

      drawChar(font, str[i], x, y,
               (i >= linkStart && i < linkStart + linkLen) ? kTextColorLink : color,
               shadowColor);
    }
    x += w;
  }
  if(underline && x1 > 0)
    hLine(x0, y + font.getFontHeight() - 1, x1, kTextColorLink);

#endif
  return x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FBSurface::checkBounds(uInt32 x, uInt32 y) const
{
  if (x <= width() && y <= height())
    return true;

  // Downgraded from a raw cerr to a debug-level log so an oversized dialog
  // (e.g. after a large dialog-font change) no longer spams the console; the
  // too-large condition is detected and reported to the user elsewhere
  // (see UIDialog / DialogContainer::anyDialogExceedsScreen)
  Logger::debug("FBSurface::checkBounds() failed: " +
                std::to_string(x) + ", " + std::to_string(y) + " vs " +
                std::to_string(width()) + ", " + std::to_string(height()));
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FullPaletteArray FBSurface::myPalette = { 0 };
