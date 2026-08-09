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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Font.hxx"

namespace GUI {

namespace {
  // A source glyph row is packed MSB-first into 16-bit words, so it takes
  // this many of them (the BITMAP_WORDS of src/tools/convbdf.c)
  constexpr int wordsPerRow(int w) { return (w + 15) / 16; }

  // Our own rows are padded out to whole bytes instead
  constexpr int bytesPerRow(int w) { return (w + 7) / 8; }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlyphSet::GlyphSet(const FontDesc& desc)
  : myFirstChar{desc.firstchar},
    myDefaultChar{desc.defaultchar}
{
  if(desc.size <= 0 || desc.bits == nullptr)
    return;

  myGlyphs.resize(desc.size);
  myMask.reserve(static_cast<size_t>(desc.size) * bytesPerRow(desc.fbbw) *
                 desc.fbbh);

  for(int i = 0; i < desc.size; ++i)
  {
    // The bounding box of this glyph, which only a proportional font varies
    const int bbw = desc.bbx ? desc.bbx[i].w : desc.fbbw;
    const int bbh = desc.bbx ? desc.bbx[i].h : desc.fbbh;
    const int bbx = desc.bbx ? desc.bbx[i].x : desc.fbbx;  // NOLINT(bugprone-signed-char-misuse,cert-str34-c)
    const int bby = desc.bbx ? desc.bbx[i].y : desc.fbby;  // NOLINT(bugprone-signed-char-misuse,cert-str34-c)

    GlyphInfo& info = myGlyphs[i];
    info.offset = static_cast<uInt32>(myMask.size());
    info.w = static_cast<Int16>(bbw);
    info.h = static_cast<Int16>(bbh);
    info.dx = static_cast<Int16>(bbx);
    info.dy = static_cast<Int16>(desc.ascent - bby - bbh);

    // Without an encode table the glyphs are fixed-size cells, one after
    // the other
    const uInt16* bits = desc.bits +
        (desc.offset ? desc.offset[i]
                     : static_cast<uInt32>(i) * desc.fbbh * wordsPerRow(desc.fbbw));

    // Repack the rows from 16-bit words into byte-aligned ones.  Both are
    // left-aligned and most significant bit first, so nothing here cares how
    // wide the glyph is
    const int words = wordsPerRow(bbw);
    const int stride = bytesPerRow(bbw);
    const size_t base = myMask.size();

    myMask.resize(base + (static_cast<size_t>(stride) * bbh));

    for(int y = 0; y < bbh; ++y)
      for(int x = 0; x < bbw; ++x)
      {
        const uInt16 word = bits[(y * words) + (x >> 4)];

        if(word & (0x8000 >> (x & 15)))
          myMask[base + (static_cast<size_t>(y) * stride) + (x >> 3)] |=
              0x80 >> (x & 7);
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Glyph GlyphSet::glyph(uInt8 chr) const
{
  // If this character is not included in the font, use the default char
  if(std::cmp_less(chr, myFirstChar) ||
     std::cmp_greater_equal(chr, myFirstChar + myGlyphs.size()))
  {
    if(chr == ' ')
      return {};
    chr = static_cast<uInt8>(myDefaultChar);
  }

  const int idx = chr - myFirstChar;
  if(idx < 0 || std::cmp_greater_equal(idx, myGlyphs.size()))
    return {};

  const GlyphInfo& info = myGlyphs[idx];
  if(info.w <= 0 || info.h <= 0)
    return {};

  return {
    .mask = myMask.data() + info.offset,
    .w = info.w,
    .h = info.h,
    .stride = bytesPerRow(info.w),
    .dx = info.dx,
    .dy = info.dy
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GlyphSet& GlyphCache::glyphs(const FontDesc& desc)
{
  const auto iter = mySets.find(desc.name);
  if(iter != mySets.end())
    return iter->second;

  return mySets.try_emplace(desc.name, desc).first->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Font::Font(GlyphCache& cache, const FontDesc& desc)
  : myFontDesc{desc},
    myCache{cache},
    myGlyphs{&cache.glyphs(desc)}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Font::changeDesc(const FontDesc& desc)
{
  myFontDesc = desc;
  myGlyphs = &myCache.glyphs(desc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Font::getCharWidth(uInt8 chr) const
{
  // If no width table is specified, return the maximum width
  if(!myFontDesc.width)
    return myFontDesc.maxwidth;

  // If this character is not included in the font, use the default char.
  if(std::cmp_less(chr, myFontDesc.firstchar) || myFontDesc.firstchar + myFontDesc.size < chr)
  {
    if(chr == ' ')
      return myFontDesc.maxwidth / 2;
    chr = myFontDesc.defaultchar;
  }

  return myFontDesc.width[chr - myFontDesc.firstchar];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Font::getStringWidth(string_view str) const
{
  // If no width table is specified, use the maximum width
  if(!myFontDesc.width)
    return myFontDesc.maxwidth * static_cast<int>(str.size());

  int width = 0;
  for(const char c: str)
    width += getCharWidth(c);
  return width;
}

} // namespace GUI
