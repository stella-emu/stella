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

class Settings;

#include "Font.hxx"
#include "bspf.hxx"

/**
  Owns every GUI::Font in Stella, plus the registry that maps the font names
  used in the settings to the built-in font data.

  A Font is created once and never replaced.  Every widget holds its font as
  a 'const GUI::Font&' (see Widget::_font), so changing a font means pointing
  the existing object at new glyph data via GUI::Font::changeDesc().  Callers
  must afterwards refresh the font-derived state their widgets have cached --
  OSystem::refreshFonts() does both halves (this class, then every dialog).

  @author  Stephen Anthony
*/
class FontManager
{
  public:
    // A user-selectable font: the value stored in the settings, and the
    // label shown for it in the UI
    struct FontEntry
    {
      // The value stored in the settings
      string_view name;
      // The label shown for it in the UI
      string_view label;
    };

    // The parts of the UI that each get their own font; the accessors further
    // down are thin wrappers over font(FontRole)
    enum class FontRole: uInt8 {
      Dialog,         // the general UI font
      Info,           // the smaller font auto-sized to pair with it
      Small,          // where space is very limited
      Launcher,       // the ROM launcher list
      RomInfo,        // the ROM info panel beside it
      DebuggerLabel,  // the debugger's labels
      DebuggerText,   // the debugger's normal text
      DebuggerDisasm, // the disassembly listing
      numRoles
    };

    // What a role's setting holds when the role works its own font out
    // instead of naming one.  Only the roles with a derivation to fall back
    // on accept it; see isRoleFont()
    static constexpr string_view AUTO_FONT = "auto";

    /** Every role starts at the smallest font; loadConfig() then resolves each from 'settings' */
    explicit FontManager(const Settings& settings);
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
    static SpanOf<FontEntry> uiFonts();

    /**
      Is this the name of one of the uiFonts()?
    */
    static bool isUIFont(string_view name);

    /**
      Get the fonts offered for the debugger, in ascending size order.  They
      are the smallest of the uiFonts(), under the very same names.
    */
    static SpanOf<FontEntry> debuggerFonts();

    /**
      Is this the name of one of the debuggerFonts()?
    */
    static bool isDebuggerFont(string_view name);

    /**
      Get the settings key a role reads.
    */
    static string_view settingKey(FontRole role);

    /**
      Is this a valid value for a role's setting -- AUTO_FONT where the role
      has a derivation, or the name of a font the role is allowed to use?
    */
    static bool isRoleFont(FontRole role, string_view name);

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
    static SpanOf<const FontDesc*> romInfoFonts();

    //////////////////////////////////////////////////////////////////////
    // The fonts themselves
    //////////////////////////////////////////////////////////////////////

    /**
      Get the font a part of the UI draws with.  This is the accessor; the
      named ones below just spell a role out.

      @param role  The part of the UI asking
    */
    const GUI::Font& font(FontRole role) const
      { return myFonts[static_cast<size_t>(role)]; }

    // The general font used in all UI elements, and the smaller info font
    // that is auto-sized to pair with it
    const GUI::Font& font() const { return font(FontRole::Dialog); }
    const GUI::Font& infoFont() const { return font(FontRole::Info); }

    // Used in a variety of situations when a really small font is needed;
    // the specific widget/dialog decides when to use it
    const GUI::Font& smallFont() const { return font(FontRole::Small); }

    // The ROM launcher list, and the ROM info panel beside it
    const GUI::Font& launcherFont() const { return font(FontRole::Launcher); }
    const GUI::Font& romInfoFont() const { return font(FontRole::RomInfo); }

  #ifdef DEBUGGER_SUPPORT
    // The debugger's labels, its normal text, and its disassembly listing
    const GUI::Font& debuggerLabelFont() const
      { return font(FontRole::DebuggerLabel); }
    const GUI::Font& debuggerTextFont() const
      { return font(FontRole::DebuggerText); }
    const GUI::Font& debuggerDisasmFont() const
      { return font(FontRole::DebuggerDisasm); }
  #endif

    //////////////////////////////////////////////////////////////////////
    // Changing a font.  Each of these mutates the existing Font objects, so
    // every reference to them stays valid (see the class comment)
    //////////////////////////////////////////////////////////////////////

    /**
      Resolve every role from its setting.  A role naming a font gets it; a
      role on AUTO_FONT works its font out instead -- the info font from the
      dialog font, the disassembly font from the debugger's text font, and so
      on.  Callers must afterwards refresh the widgets that cached metrics.

      @param settings  The settings to read the roles from
    */
    void loadConfig(const Settings& settings);

    /**
      Change the font of the ROM info panel to one of the romInfoFonts().
      Only used while that role is on AUTO_FONT, since it is then the
      launcher's area, not a setting, that decides.

      @param fd  The font data the launcher settled on
    */
    void changeRomInfoFont(const FontDesc& fd);

  private:
    // The mutable side of font(); the change* methods above are the only
    // callers, since a role's Font is swapped in place and never replaced
    GUI::Font& fontFor(FontRole role)
      { return myFonts[static_cast<size_t>(role)]; }

  private:
    // The glyphs every font draws with, unpacked once per distinct font and
    // shared by the roles naming the same one.  Declared before myFonts,
    // which take it in their c'tor
    GUI::GlyphCache myGlyphCache;

    // One Font per role, indexed by FontRole.  The debugger's roles are here
    // even in a build without it, which costs a descriptor each and keeps the
    // role table the same shape everywhere
    std::array<GUI::Font, static_cast<size_t>(FontRole::numRoles)> myFonts;

  private:
    // Following constructors and assignment operators not supported
    FontManager() = delete;
    FontManager(const FontManager&) = delete;
    FontManager(FontManager&&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager& operator=(FontManager&&) = delete;
};

#endif  // FONT_MANAGER_HXX
