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

#ifndef ROM_INFO_WIDGET_HXX
#define ROM_INFO_WIDGET_HXX

class FSNode;
class FBSurface;

#include "Props.hxx"
#include "Widget.hxx"

/**
  Shows a ROM's textual properties (and any PNG metadata) as wrapped
  lines in the launcher, with a clickable link line.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class RomInfoWidget : public Widget, public CommandSender
{
  public:
    // Sent when the "Name:" line is clicked, if the ROM has a Cart_Url property
    struct Cmd {
      static constexpr GuiCmd::Code
        Clicked = GuiCmd::of("RomInfoWidget.Clicked");
    };

  public:
    RomInfoWidget(GuiObject *boss, const GUI::Font& font);
    ~RomInfoWidget() override = default;

    // Sets the properties to describe (parsed immediately if the launcher is active)
    void setProperties(const FSNode& node, const Properties& properties,
                       bool full = true);
    // Clears the display; used when nothing (or an invalid ROM) is selected
    void clearProperties();
    // Re-parses the current properties (e.g. after a change made in the ROM browser)
    void reloadProperties(const FSNode& node);

    const string& getUrl() const { return myUrl; }

    // Fires Cmd::Clicked if the highlighted "Name:" link is clicked
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;

  protected:
    // Draws as many lines of myRomInfo as fit, wrapping/eliding as needed
    void drawWidget(bool hilite) override;

  private:
    // Builds myRomInfo (and myUrl) from myProperties; 'full' additionally runs
    // controller/bankswitch auto-detection, which needs to open the ROM image
    void parseProperties(const FSNode& node, bool full = true);

  private:
    // Some ROM properties info, as well as 'tEXt' chunks from the PNG image
    StringList myRomInfo;

    // The properties for the currently selected ROM
    Properties myProperties;

    // Indicates if the current properties should actually be used
    bool myHaveProperties{false};

    // Optional cart link URL
    string myUrl;

  private:
    // Following constructors and assignment operators not supported
    RomInfoWidget() = delete;
    RomInfoWidget(const RomInfoWidget&) = delete;
    RomInfoWidget(RomInfoWidget&&) = delete;
    RomInfoWidget& operator=(const RomInfoWidget&) = delete;
    RomInfoWidget& operator=(RomInfoWidget&&) = delete;
};

#endif  // ROM_INFO_WIDGET_HXX
