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

// The candidates for the auto-sized info font, in ascending size order
const std::array<const FontDesc*, 7> INFO_FONTS = {
  &GUI::consoleDesc, &GUI::consoleMediumDesc, &GUI::stellaMediumDesc,
  &GUI::stellaLargeDesc, &GUI::stella12x24tDesc, &GUI::stella14x28tDesc,
  &GUI::stella16x32tDesc
};

// The candidates for the ROM info panel, in descending size order
const std::array<const FontDesc*, 7> ROMINFO_FONTS = {
  &GUI::stella16x32tDesc, &GUI::stella14x28tDesc, &GUI::stella12x24tDesc,
  &GUI::stellaLargeDesc, &GUI::stellaMediumDesc,
  &GUI::consoleMediumBDesc, &GUI::consoleBDesc
};

} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FontManager::FontManager(OSystem& osystem)
  : myFont{fontDesc(osystem.settings().getString("dialogfont"))},
    myInfoFont{infoFontDesc(myFont.desc())},
    mySmallFont{smallestDesc()},
    myLauncherFont{fontDesc(osystem.settings().getString("launcherfont"))},
    myRomInfoFont{smallestDesc()}
#ifdef DEBUGGER_SUPPORT
  , myDebuggerLabelFont{GUI::consoleDesc},
    myDebuggerTextFont{GUI::consoleDesc}
#endif
{
#ifdef DEBUGGER_SUPPORT
  changeDebuggerFont(osystem.settings().getString("dbg.fontsize"),
                     osystem.settings().getInt("dbg.fontstyle"));
#endif
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
std::span<const FontManager::FontEntry> FontManager::uiFonts()
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
std::span<const FontDesc* const> FontManager::romInfoFonts()
{
  return ROMINFO_FONTS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::changeDialogFont(string_view name)
{
  const FontDesc& fd = fontDesc(name);

  myFont.changeDesc(fd);
  myInfoFont.changeDesc(infoFontDesc(fd));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::changeLauncherFont(string_view name)
{
  myLauncherFont.changeDesc(fontDesc(name));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::changeRomInfoFont(const FontDesc& fd)
{
  myRomInfoFont.changeDesc(fd);
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FontManager::changeDebuggerFont(string_view size, int style)
{
  const FontDesc *lDesc{nullptr}, *nDesc{nullptr};

  if(size == "large")
  {
    // Large font doesn't use style at all
    lDesc = nDesc = &GUI::stellaMediumDesc;
  }
  else
  {
    // Both remaining sizes offer the same four label/text style combinations
    const FontDesc& regular = size == "medium"
      ? GUI::consoleMediumDesc : GUI::consoleDesc;
    const FontDesc& bold = size == "medium"
      ? GUI::consoleMediumBDesc : GUI::consoleBDesc;

    switch(style)
    {
      case 1:
        lDesc = &bold;
        nDesc = &regular;
        break;
      case 2:
        lDesc = &regular;
        nDesc = &bold;
        break;
      case 3:
        lDesc = nDesc = &bold;
        break;
      default: // default to zero
        lDesc = nDesc = &regular;
        break;
    }
  }

  myDebuggerLabelFont.changeDesc(*lDesc);
  myDebuggerTextFont.changeDesc(*nDesc);
}
#endif  // DEBUGGER_SUPPORT
