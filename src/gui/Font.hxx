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
  // Glyph size in pixels
  uInt8 w;
  uInt8 h;
  // Offset of the box's origin from the font's own bounding box
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

  The mask is one bit per pixel, set where the glyph has ink, most significant
  bit leftmost, each row padded out to a whole number of bytes.  'stride' says
  how far apart the rows are, so the renderer needs to know none of that.

  This is deliberately the only shape the renderer sees, so that where the
  glyph came from, and how wide it is, are the font's business alone.
*/
struct Glyph
{
  const uInt8* mask{nullptr};  // h rows of 'stride' bytes, one bit per pixel
  int w{0}, h{0};              // size of the mask, in pixels
  int stride{0};               // bytes from one row of the mask to the next
  int dx{0}, dy{0};            // where it goes, relative to the pen
};

/**
  The unpacked glyphs of one font, shared by every Font that draws with it.

  Built once from a FontDesc, which is the only place the packed bitmap
  layout is understood.
*/
class GlyphSet
{
  public:
    /** Unpacks every glyph in 'desc' up front */
    explicit GlyphSet(const FontDesc& desc);
    ~GlyphSet() = default;

    /**
      Get a character's glyph, positioned relative to the pen.

      @param chr  The character to look up; one not in the font is replaced
                  by the font's default character

      @return  The glyph, or one with a null mask where there is nothing
               to draw
    */
    Glyph glyph(uInt8 chr) const;

  private:
    // Where one glyph lives in myMask, and where it goes once drawn
    struct GlyphInfo
    {
      uInt32 offset{0};  // byte offset of this glyph's mask
      Int16 w{0}, h{0};  // its size, in pixels
      Int16 dx{0}, dy{0};
    };

    // Every glyph's mask, one bit per pixel, back to back
    vector<uInt8> myMask;

    // Indexed by glyph number, i.e. character - myFirstChar
    vector<GlyphInfo> myGlyphs;

    // First character in the font, and the glyph substituted for one outside it
    int myFirstChar{0}, myDefaultChar{0};

  private:
    // Following constructors and assignment operators not supported
    GlyphSet(const GlyphSet&) = delete;
    GlyphSet(GlyphSet&&) = delete;
    GlyphSet& operator=(const GlyphSet&) = delete;
    GlyphSet& operator=(GlyphSet&&) = delete;
};

/**
  Owns one GlyphSet per distinct font, so that the roles sharing a font
  share its unpacked glyphs instead of each holding a copy.

  The storage lives here rather than in Font because several roles routinely
  name the same font, and a Font is swapped between fonts in place.
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
    // Every name is a static string, so a view is safe to key on.  A
    // node-based map, so that handing out references is safe as it grows
    std::unordered_map<string_view, GlyphSet> mySets;

  private:
    // Following constructors and assignment operators not supported
    GlyphCache(const GlyphCache&) = delete;
    GlyphCache(GlyphCache&&) = delete;
    GlyphCache& operator=(const GlyphCache&) = delete;
    GlyphCache& operator=(GlyphCache&&) = delete;
};

/**
  A single font: its glyphs (from the shared GlyphCache) plus the
  description used to draw and measure text.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class Font
{
  public:
    // Unpacks (or reuses, if already cached) the glyphs of 'desc'
    Font(GlyphCache& cache, const FontDesc& desc);
    ~Font() = default;

    // The raw font description this Font currently draws with
    const FontDesc& desc() const { return myFontDesc; }

    // Replace the font description in place.  Every widget holds a reference to
    // its Font, so swapping the descriptor here makes all of them immediately
    // use the new glyphs and metrics without having to reseat any reference
    // (which a reference member cannot do anyway).  Callers must then refresh
    // any font-derived state cached by the referrers, which is what
    // OSystem::refreshFonts() does for the whole application.
    void changeDesc(const FontDesc& desc);

    // Get a character's glyph, positioned relative to the pen; the glyphs
    // are the cache's, so this is a lookup and not an unpack
    Glyph glyph(uInt8 chr) const { return myGlyphs->glyph(chr); }

    // Height of a glyph, and of a full text line (glyph height plus leading)
    int getFontHeight() const { return myFontDesc.height; }
    int getLineHeight() const { return myFontDesc.height + 2; }
    // The widest a glyph in this font gets
    int getMaxCharWidth() const { return myFontDesc.maxwidth; }

    // Am I one of the large fonts?  The widgets draw the chrome around my text
    // (a checkbox's box, a scroll bar, a drop-down arrow, a field's inset) in one
    // of two hand-drawn sizes, and this is what picks between them.
    // Arrows and text insets now scale with the font instead of stepping; what's
    // left here is hand-drawn artwork (checkbox/radio circles, pictorial icons)
    // kept on this two-bucket switch deliberately, not pending further work.
    bool isLarge() const { return myFontDesc.height >= 24; }

    // Width of a single character, in pixels
    int getCharWidth(uInt8 chr) const;

    // Width of a string, in pixels, as the sum of its characters' widths
    int getStringWidth(string_view str) const;

  private:
    // The font description currently in effect (see changeDesc())
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
