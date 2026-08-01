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

#ifndef FONT_HXX
#define FONT_HXX

#include <unordered_map>

#include "bspf.hxx"

struct BBX
{
  uInt8 w;
  uInt8 h;
  Int8 x;
  Int8 y;
};

/* builtin C-based proportional/fixed font structure */
/* based on The Microwindows Project http://microwindows.org */
struct FontDesc
{
  string_view   name;                   /* font name */
  int           maxwidth;               /* max width in pixels */
  int           height;                 /* height in pixels */
  int           fbbw, fbbh, fbbx, fbby;	/* max bounding box */
  int           ascent;                 /* ascent (baseline) height */
  int           firstchar;              /* first character in bitmap */
  int           size;                   /* font size in glyphs */
  const uInt16* bits;                   /* 16-bit right-padded bitmap data */
  const uInt32* offset;                 /* offsets into bitmap data*/
  const uInt8*  width;                  /* character widths or nullptr if fixed */
  const BBX*    bbx;                    /* character bounding box or nullptr if fixed */
  int           defaultchar;            /* default char (not glyph index) */
  Int64         bits_size;              /* # words of bitmap_t bits */
};

namespace GUI {

/**
  One character's glyph, ready to be drawn at a pen position.

  The coverage is one byte per pixel: 0 where the glyph does not cover the
  pixel at all, 255 where it covers it fully, and the values between for a
  partially covered edge pixel.  Bitmap fonts only ever produce 0 or 255 --
  the intermediate values are what an antialiased source supplies.

  This is deliberately the only shape the renderer sees, so that where the
  glyph came from, and how wide it is, are the font's business alone.
*/
struct Glyph
{
  const uInt8* coverage{nullptr};  // h rows of w bytes, 'pitch' bytes apart
  int w{0}, h{0};                  // size of the coverage bitmap
  int pitch{0};                    // bytes from one row to the next
  int dx{0}, dy{0};                // where it goes, relative to the pen
};

/**
  The unpacked glyphs of one font, shared by every Font that draws with it.

  Built once from a FontDesc, which is the only place the packed bitmap
  layout is understood.  The coverage of each glyph is stored contiguously;
  Glyph::pitch is carried separately so that a future backing store which
  packs the glyphs into a 2D surface (the shape a GPU texture would want)
  needs no change here or in the renderer.
*/
class GlyphSet
{
  public:
    explicit GlyphSet(const FontDesc& desc);
    ~GlyphSet() = default;

    /**
      Get a character's glyph, positioned relative to the pen.

      @param chr  The character to look up; one not in the font is replaced
                  by the font's default character

      @return  The glyph, or one with a null coverage where there is nothing
               to draw
    */
    Glyph glyph(uInt8 chr) const;

  private:
    // Where one glyph lives in myCoverage, and where it goes once drawn
    struct GlyphInfo
    {
      uInt32 offset{0};  // start of this glyph's coverage bytes
      Int16 w{0}, h{0};  // its size
      Int16 dx{0}, dy{0};
    };

    // The coverage bytes of every glyph, back to back
    vector<uInt8> myCoverage;

    // Indexed by glyph number, i.e. character - myFirstChar
    vector<GlyphInfo> myGlyphs;

    int myFirstChar{0}, myDefaultChar{0};
};

/**
  Owns one GlyphSet per distinct font, so that the roles sharing a font
  share its unpacked glyphs instead of each holding a copy.

  This is a CPU-side glyph cache, not a GPU atlas -- see FONT_REWORK_PLAN.md.
  It is also where a font loaded at runtime will put its glyphs, which is why
  the storage lives here rather than in Font.
*/
class GlyphCache
{
  public:
    GlyphCache() = default;
    ~GlyphCache() = default;

    /**
      Get the unpacked glyphs of a font, unpacking them on first use.

      @param desc  The font data

      @return  Its glyphs, owned by this cache and stable for its lifetime
    */
    const GlyphSet& glyphs(const FontDesc& desc);

  private:
    // Keyed by font name, which is what identifies a font to the settings.
    // A node-based map, so that handing out references is safe as it grows
    std::unordered_map<string, GlyphSet> mySets;

  private:
    // Following constructors and assignment operators not supported
    GlyphCache(const GlyphCache&) = delete;
    GlyphCache(GlyphCache&&) = delete;
    GlyphCache& operator=(const GlyphCache&) = delete;
    GlyphCache& operator=(GlyphCache&&) = delete;
};

class Font
{
  public:
    Font(GlyphCache& cache, const FontDesc& desc);
    ~Font() = default;

    const FontDesc& desc() const { return myFontDesc; }

    // Replace the font description in place.  Every widget holds a reference to
    // its Font, so swapping the descriptor here makes all of them immediately
    // use the new glyphs and metrics without having to reseat any reference
    // (which a reference member cannot do anyway).  Callers must then refresh
    // any font-derived state cached by the referrers (see
    // Widget::refreshFontMetrics() and FrameBuffer::changeLauncherFont()).
    void changeDesc(const FontDesc& desc);

    // Get a character's glyph, positioned relative to the pen; the glyphs
    // are the cache's, so this is a lookup and not an unpack
    Glyph glyph(uInt8 chr) const { return myGlyphs->glyph(chr); }

    int getFontHeight() const { return myFontDesc.height; }
    int getLineHeight() const { return myFontDesc.height + 2; }
    int getMaxCharWidth() const { return myFontDesc.maxwidth; }

    // Am I one of the large fonts?  The widgets draw the chrome around my text
    // (a checkbox's box, a scroll bar, a drop-down arrow, a field's inset) in one
    // of two hand-drawn sizes, and this is what picks between them.
    // FIXME: this is a step, not a scale, and the widgets are really asking which
    //        BITMAP set to draw -- the numbers beside them are tuned to match.
    //        Revisit with the font rework, where fonts and bitmaps should be
    //        unified so the chrome follows the font instead of jumping at 24.
    bool isLarge() const { return myFontDesc.height >= 24; }

    int getCharWidth(uInt8 chr) const;

    int getStringWidth(string_view str) const;

  private:
    FontDesc myFontDesc;

    // Where the glyphs come from, and this font's set within it.  Both stay
    // valid across changeDesc(), which just points myGlyphs at another set
    GlyphCache& myCache;
    const GlyphSet* myGlyphs{nullptr};

  private:
    // Following constructors and assignment operators not supported
    Font() = delete;
    Font(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&) = delete;
};

} // namespace GUI

#endif  // FONT_HXX
