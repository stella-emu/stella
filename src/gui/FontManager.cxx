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

#include "OSystem.hxx"
#include "Settings.hxx"

// This is the only place the built-in font data is pulled in.  Each of these
// headers defines its glyph bitmap with internal linkage, so every additional
// translation unit that includes one gets its own copy of the data
#include "StellaFont.hxx"
#include "ConsoleFont.hxx"
#include "ConsoleBFont.hxx"
#include "ConsoleMediumFont.hxx"
#include "ConsoleMediumBFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "Stella12x24tFont.hxx"
#include "Stella14x28tFont.hxx"
#include "Stella16x32tFont.hxx"

#include "FontManager.hxx"

namespace {

// The fonts offered for the general UI, in ascending size order
constexpr std::array<FontManager::FontEntry, 7> UI_FONTS = {{
  {"small",      "Small"},          //  8x13
  {"low_medium", "Low Medium"},     //  9x15
  {"medium",     "Medium"},         //  9x18
  {"large",      "Large (10pt)"},   // 10x20
  {"large12",    "Large (12pt)"},   // 12x24
  {"large14",    "Large (14pt)"},   // 14x28
  {"large16",    "Large (16pt)"}    // 16x32
}};

// The debugger offers the smallest of the UI fonts, under the very same
// names; it takes them from the front of the list so the two can never drift
constexpr size_t NUM_DEBUGGER_FONTS = 3;
static_assert(UI_FONTS[0].name == "small" &&
              UI_FONTS[1].name == "low_medium" &&
              UI_FONTS[2].name == "medium",
              "the debugger fonts must be the smallest three UI fonts");

// The candidates for the auto-sized info font, in ascending size order
const std::array<const FontDesc*, 7> INFO_FONTS = {
  &GUI::consoleDesc, &GUI::consoleMediumDesc, &GUI::stellaMediumDesc,
  &GUI::stellaLargeDesc, &GUI::stella12x24tDesc, &GUI::stella14x28tDesc,
  &GUI::stella16x32tDesc
};

// The settings key each role reads, in FontRole order.  Settings owns the
// defaults, as it does for every other setting; this is only the mapping
constexpr std::array<string_view,
                     static_cast<size_t>(FontManager::FontRole::numRoles)>
ROLE_KEYS = {
  "ui.font.dialog",
  "ui.font.info",
  "ui.font.small",
  "ui.font.launcher",
  "ui.font.rominfo",
  "ui.font.debuggerlabel",
  "ui.font.debuggertext",
  "ui.font.debuggerdisasm"
};

// The roles that work their own font out, and so accept AUTO_FONT
constexpr bool derivesFont(FontManager::FontRole role)
{
  return role == FontManager::FontRole::Info
      || role == FontManager::FontRole::Small
      || role == FontManager::FontRole::RomInfo
      || role == FontManager::FontRole::DebuggerDisasm;
}

// The debugger's roles draw from debuggerFonts() rather than uiFonts(), and
// take a bold or regular cut of the named font from 'dbg.fontstyle'
constexpr bool isDebuggerRole(FontManager::FontRole role)
{
  return role == FontManager::FontRole::DebuggerLabel
      || role == FontManager::FontRole::DebuggerText
      || role == FontManager::FontRole::DebuggerDisasm;
}

#ifdef DEBUGGER_SUPPORT
// Get the regular or bold cut of a debugger font.  These cannot go through
// fontDesc(), which answers one descriptor per name, because the debugger
// needs the pair to choose from
const FontDesc& debuggerDesc(string_view name, bool bold)
{
  if(name == "medium")
    return GUI::stellaMediumDesc;                             // 9x18, no cuts
  if(name == "low_medium")
    return bold ? GUI::consoleMediumBDesc : GUI::consoleMediumDesc;  // 9x15

  return bold ? GUI::consoleBDesc : GUI::consoleDesc;                // 8x13
}
#endif

// The candidates for the ROM info panel, in descending size order
const std::array<const FontDesc*, 7> ROMINFO_FONTS = {
  &GUI::stella16x32tDesc, &GUI::stella14x28tDesc, &GUI::stella12x24tDesc,
  &GUI::stellaLargeDesc, &GUI::stellaMediumDesc,
  &GUI::consoleMediumBDesc, &GUI::consoleBDesc
};

}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FontManager::FontManager(const Settings& settings)
  // Every role starts at the smallest font, and loadConfig() then resolves
  // each of them, exactly as it does for a later change
  : myFonts{{
      GUI::Font{myGlyphCache, smallestDesc()},  // Dialog
      GUI::Font{myGlyphCache, smallestDesc()},  // Info
      GUI::Font{myGlyphCache, smallestDesc()},  // Small
      GUI::Font{myGlyphCache, smallestDesc()},  // Launcher
      GUI::Font{myGlyphCache, smallestDesc()},  // RomInfo
      GUI::Font{myGlyphCache, smallestDesc()},  // DebuggerLabel
      GUI::Font{myGlyphCache, smallestDesc()},  // DebuggerText
      GUI::Font{myGlyphCache, smallestDesc()}   // DebuggerDisasm
    }}
{
  loadConfig(settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FontDesc& FontManager::fontDesc(string_view name)
{
  if(name == "small")
    return GUI::consoleDesc;        //  8x13
  else if(name == "low_medium")
    return GUI::consoleMediumBDesc; //  9x15
  else if(name == "medium")
    return GUI::stellaMediumDesc;   //  9x18
  else if(name == "large" || name == "large10")
    return GUI::stellaLargeDesc;    // 10x20
  else if(name == "large12")
    return GUI::stella12x24tDesc;   // 12x24
  else if(name == "large14")
    return GUI::stella14x28tDesc;   // 14x28
  else // "large16"
    return GUI::stella16x32tDesc;   // 16x32
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpanOf<FontManager::FontEntry> FontManager::uiFonts()
{
  return UI_FONTS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FontManager::isUIFont(string_view name)
{
  return std::ranges::any_of(UI_FONTS,
      [&](const FontEntry& entry) { return entry.name == name; });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpanOf<FontManager::FontEntry> FontManager::debuggerFonts()
{
  return SpanOf<FontEntry>{UI_FONTS}.first(NUM_DEBUGGER_FONTS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FontManager::isDebuggerFont(string_view name)
{
  return std::ranges::any_of(debuggerFonts(),
      [&](const FontEntry& entry) { return entry.name == name; });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view FontManager::settingKey(FontRole role)
{
  return ROLE_KEYS[static_cast<size_t>(role)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FontManager::isRoleFont(FontRole role, string_view name)
{
  if(name == AUTO_FONT)
    return derivesFont(role);

  return isDebuggerRole(role) ? isDebuggerFont(name) : isUIFont(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FontDesc& FontManager::referenceDesc()
{
  return GUI::stellaMediumDesc;     //  9x18
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FontDesc& FontManager::smallestDesc()
{
  return GUI::stellaDesc;           //  6x10
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FontDesc& FontManager::infoFontDesc(const FontDesc& fd)
{
  for(const FontDesc* desc: INFO_FONTS)
    if(fd.height <= desc->height * 1.4)
      return *desc;

  return *INFO_FONTS.front();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpanOf<const FontDesc*> FontManager::romInfoFonts()
{
  return ROMINFO_FONTS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::loadConfig(const Settings& settings)
{
  const auto value = [&](FontRole role) {
    return string_view{settings.getString(settingKey(role))};
  };

  // The dialog font, and the info font that pairs with it unless it names a
  // font of its own
  const FontDesc& dialog = fontDesc(value(FontRole::Dialog));
  fontFor(FontRole::Dialog).changeDesc(dialog);

  const string_view info = value(FontRole::Info);
  fontFor(FontRole::Info).changeDesc(info == AUTO_FONT ? infoFontDesc(dialog)
                                                       : fontDesc(info));

  fontFor(FontRole::Launcher).changeDesc(fontDesc(value(FontRole::Launcher)));

  // On AUTO_FONT the small font stays at the smallest one there is
  const string_view small = value(FontRole::Small);
  fontFor(FontRole::Small).changeDesc(small == AUTO_FONT ? smallestDesc()
                                                         : fontDesc(small));

  // The ROM info panel is the one role whose AUTO_FONT case is not decided
  // here: the launcher fits a font to the area it has (see changeRomInfoFont)
  if(const string_view romInfo = value(FontRole::RomInfo); romInfo != AUTO_FONT)
    fontFor(FontRole::RomInfo).changeDesc(fontDesc(romInfo));

#ifdef DEBUGGER_SUPPORT
  // 'dbg.fontstyle' says which of the debugger's roles wears the bold cut
  const int style = settings.getInt("dbg.fontstyle");
  const bool boldLabel = style == 1 || style == 3;
  const bool boldText  = style == 2 || style == 3;

  fontFor(FontRole::DebuggerLabel)
      .changeDesc(debuggerDesc(value(FontRole::DebuggerLabel), boldLabel));

  const FontDesc& text = debuggerDesc(value(FontRole::DebuggerText), boldText);
  fontFor(FontRole::DebuggerText).changeDesc(text);

  // The disassembly listing follows the debugger's text unless it names a
  // font of its own; either way it wears the same cut as that text
  const string_view disasm = value(FontRole::DebuggerDisasm);
  fontFor(FontRole::DebuggerDisasm)
      .changeDesc(disasm == AUTO_FONT ? text : debuggerDesc(disasm, boldText));
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::changeRomInfoFont(const FontDesc& fd)
{
  fontFor(FontRole::RomInfo).changeDesc(fd);
}
