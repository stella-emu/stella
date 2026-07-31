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

#ifndef FONT_MANAGER_HXX
#define FONT_MANAGER_HXX

class OSystem;

#include "Font.hxx"
#include "bspf.hxx"

/**
  Owns every GUI::Font in Stella, plus the registry that maps the font names
  used in the settings to the built-in font data.

  The fonts used to be spread over three owners (FrameBuffer, DebuggerDialog
  and LauncherDialog), each with its own copy of the name lookup; they all
  live here now, and those classes forward to this class.

  A Font is created once and never replaced.  Every widget holds its font as
  a 'const GUI::Font&' (see Widget::_font), so changing a font means pointing
  the existing object at new glyph data via GUI::Font::changeDesc().  Callers
  must afterwards refresh the font-derived state their widgets have cached --
  see Dialog::refreshFont() and Widget::refreshFontMetrics().

  @author  Stephen Anthony
*/
class FontManager
{
  public:
    // A user-selectable font: the value stored in the settings, and the
    // label shown for it in the UI
    struct FontEntry
    {
      string_view name;
      string_view label;
    };

    explicit FontManager(OSystem& osystem);
    ~FontManager() = default;

    //////////////////////////////////////////////////////////////////////
    // The registry.  Static, since the settings are validated long before
    // any FontManager exists
    //////////////////////////////////////////////////////////////////////

    /**
      Get the font data for a registry name.

      @param name  The settings name of the font

      @return  The matching font data, or the largest font if the name is
               not a known one
    */
    static const FontDesc& fontDesc(string_view name);

    /**
      Get the fonts offered for the general UI (dialogs and launcher), in
      ascending size order.
    */
    static std::span<const FontEntry> uiFonts();

    /**
      Is this the name of one of the uiFonts()?
    */
    static bool isUIFont(string_view name);

    /**
      Get the font that layout minimums are measured against; whatever fits
      with this font is scaled up to fit with any larger one.
    */
    static const FontDesc& referenceDesc();

    /**
      Get the smallest built-in font, used where space is very limited.
    */
    static const FontDesc& smallestDesc();

    /**
      Get the info font that pairs with a dialog font, aiming for a size
      ratio of 1 / 1.4 (~= 18 / 13).

      @param fd  The dialog font data

      @return  The matching info font data
    */
    static const FontDesc& infoFontDesc(const FontDesc& fd);

    /**
      Get the fonts the ROM info panel may choose from, in descending size
      order.  Which one actually fits is up to the launcher, since the
      minimums it has to satisfy are part of its layout, not of the font.
    */
    static std::span<const FontDesc* const> romInfoFonts();

    //////////////////////////////////////////////////////////////////////
    // The fonts themselves
    //////////////////////////////////////////////////////////////////////

    // The general font used in all UI elements, and the smaller info font
    // that is auto-sized to pair with it
    const GUI::Font& font() const { return myFont; }
    const GUI::Font& infoFont() const { return myInfoFont; }

    // Used in a variety of situations when a really small font is needed;
    // the specific widget/dialog decides when to use it
    const GUI::Font& smallFont() const { return mySmallFont; }

    // The ROM launcher list, and the ROM info panel beside it
    const GUI::Font& launcherFont() const { return myLauncherFont; }
    const GUI::Font& romInfoFont() const { return myRomInfoFont; }

  #ifdef DEBUGGER_SUPPORT
    // The debugger's labels, and its normal text
    const GUI::Font& debuggerLabelFont() const { return myDebuggerLabelFont; }
    const GUI::Font& debuggerTextFont() const { return myDebuggerTextFont; }
  #endif

    //////////////////////////////////////////////////////////////////////
    // Changing a font.  Each of these mutates the existing Font object, so
    // every reference to it stays valid (see the class comment)
    //////////////////////////////////////////////////////////////////////

    /**
      Change the dialog font, which drives both the general UI font and the
      auto-sized info font.

      @param name  The settings name of the new dialog font
    */
    void changeDialogFont(string_view name);

    /**
      Change the ROM launcher font.

      @param name  The settings name of the new launcher font
    */
    void changeLauncherFont(string_view name);

    /**
      Change the font of the ROM info panel to one of the romInfoFonts().

      @param fd  The font data the launcher settled on
    */
    void changeRomInfoFont(const FontDesc& fd);

  #ifdef DEBUGGER_SUPPORT
    /**
      Change the debugger fonts.  The debugger picks its fonts by size and
      style rather than by name, and the two names it shares with the general
      UI ("medium" and "large") do not mean the same fonts there.

      @param size   The 'dbg.fontsize' setting
      @param style  The 'dbg.fontstyle' setting
    */
    void changeDebuggerFont(string_view size, int style);
  #endif

  private:
    GUI::Font myFont;
    GUI::Font myInfoFont;
    GUI::Font mySmallFont;
    GUI::Font myLauncherFont;
    GUI::Font myRomInfoFont;
  #ifdef DEBUGGER_SUPPORT
    GUI::Font myDebuggerLabelFont;
    GUI::Font myDebuggerTextFont;
  #endif

  private:
    // Following constructors and assignment operators not supported
    FontManager() = delete;
    FontManager(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager& operator=(FontManager&&) = delete;
};

#endif  // FONT_MANAGER_HXX
