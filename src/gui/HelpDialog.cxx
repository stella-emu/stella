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
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "MediaFactory.hxx"
#include "Layout.hxx"

#include "HelpDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelpDialog::HelpDialog(OSystem& osystem, DialogContainer& parent,
                       const GUI::Font& font)
  : Dialog(osystem, parent, font, "Help")
{
  const int fontHeight   = Dialog::fontHeight(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth(" << "),
            closeButtonWidth = Dialog::buttonWidth("Close");
  WidgetArray wid;

  // Previous, Next, Update and Close buttons
  myPrevButton =
    new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                     "<<", GuiObject::kPrevCmd);
  myPrevButton->clearFlags(Widget::FLAG_ENABLED);
  wid.push_back(myPrevButton);

  myNextButton =
    new ButtonWidget(this, font, 0, 0, buttonWidth, buttonHeight,
                     ">>", GuiObject::kNextCmd);
  wid.push_back(myNextButton);

  const int updButtonWidth = Dialog::buttonWidth("Check for Update" + ELLIPSIS);
  myUpdateButton =
    new ButtonWidget(this, font, 0, 0, updButtonWidth, buttonHeight,
                     "Check for Update" + ELLIPSIS, kUpdateCmd);
  myUpdateButton->setEnabled(true);
  wid.push_back(myUpdateButton);

  auto* b = new ButtonWidget(this, font, 0, 0, closeButtonWidth,
                             buttonHeight, "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);

  myTitle = new StaticTextWidget(this, font, 0, 0, 1, fontHeight,
                                 "", TextAlign::Center);
  myTitle->setTextColor(kTextColorEm);

  for(uInt32 i = 0; i < LINES_PER_PAGE; ++i)
  {
    myKey[i] = new StaticTextWidget(this, font, 0, 0, 1, fontHeight);
    myDesc[i] = new StaticTextWidget(this, font, 0, 0, 1, fontHeight);
    myDesc[i]->setID(i);
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontHeight   = Dialog::fontHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth(" << "),
            updButtonWidth = Dialog::buttonWidth("Check for Update" + ELLIPSIS),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  _w = 46 * fontWidth + HBORDER * 2;
  _h = _th + 11 * lineHeight + VGAP * 3 + buttonHeight + VBORDER * 2;

  // Centered title, full width
  myTitle->setPos(HBORDER, VBORDER + _th);
  myTitle->setWidth(_w - HBORDER * 2);

  // Key / description table: fixed key column, description column fills the rest
  const int lwidth = 15 * fontWidth;
  const int numRows = static_cast<int>(LINES_PER_PAGE);
  auto table = std::make_unique<GridLayout>(2, numRows);
  table->columnFixed(0, lwidth);
  table->columnStretch(1);
  for(int i = 0; i < numRows; ++i)
  {
    table->rowFixed(i, fontHeight);
    table->place(0, i, widgetItem(myKey[i]));
    table->place(1, i, widgetItem(myDesc[i]));
  }
  table->doLayout(HBORDER, VBORDER + _th + lineHeight + VGAP * 2,
                  _w - HBORDER * 2, numRows * fontHeight);

  // Bottom row: Prev / Next / Update on the left, Close (cancel widget) at right
  auto navButtons = std::make_unique<BoxLayout>(Dir::Horizontal, fontWidth);
  navButtons->addFixed(widgetItem(myPrevButton), buttonWidth);
  navButtons->addFixed(widgetItem(myNextButton), buttonWidth);
  navButtons->addFixed(widgetItem(myUpdateButton), updButtonWidth);
  navButtons->doLayout(HBORDER, _h - buttonHeight - VBORDER,
                       buttonWidth * 2 + updButtonWidth + fontWidth * 2, buttonHeight);
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::updateStrings(int page, int lines, string& title)
{
  int i = 0;
  const auto ADD_BIND = [&](string_view k, string_view d)
  {
    myKeyStr[i] = k;  myDescStr[i] = d;  i++;
  };
  const auto ADD_EVENT = [&](const Event::Type e, string_view d)
  {
    string desc = instance().eventHandler().getMappingDesc(e, EventMode::kEmulationMode);
    if(desc.empty())
      desc = instance().eventHandler().getMappingDesc(e, EventMode::kMenuMode);
    ADD_BIND(!desc.empty() ? desc : "None", d);
  };
  const auto ADD_TEXT = [&](string_view d) { ADD_BIND("", d); };
  const auto ADD_LINE = [&]() { ADD_BIND("", ""); };

  setHelpAnchor("Hotkeys");
  switch(page)
  {
    case 1:
      title = "Common commands";
      ADD_EVENT(Event::UIHelp,              "Open context-sensitive help");
      ADD_LINE();
      ADD_EVENT(Event::Quit,                "Quit emulation");
      ADD_EVENT(Event::ExitMode,            "Exit current mode/menu");
      ADD_EVENT(Event::OptionsMenuMode,     "Enter Options menu");
      ADD_EVENT(Event::CmdMenuMode,         "Toggle Command menu");
      ADD_EVENT(Event::VidmodeIncrease,     "Increase window size");
      ADD_EVENT(Event::VidmodeDecrease,     "Decrease window size");
      ADD_EVENT(Event::ToggleFullScreen,    "Toggle fullscreen /");
      ADD_BIND("",                          "  windowed mode");
      break;

    case 2:
      title = "Special commands";
      ADD_EVENT(Event::FormatIncrease,      "Switch between NTSC/PAL/SECAM");
      ADD_EVENT(Event::PaletteIncrease,     "Switch to next palette");
      ADD_EVENT(Event::TogglePhosphor,      "Toggle 'phosphor' effect");
      ADD_LINE();
      ADD_EVENT(Event::ToggleGrabMouse,     "Grab mouse (keep in window)");
      ADD_EVENT(Event::NextMouseControl,    "Toggle controller for mouse");
      ADD_EVENT(Event::ToggleSAPortOrder,   "Toggle Stelladaptor left/right");
      ADD_LINE();
      ADD_EVENT(Event::VolumeIncrease,      "Increase volume by 2%");
      ADD_EVENT(Event::VolumeDecrease,      "Decrease volume by 2%");
      break;

    case 3:
      title = "TV effects";
      ADD_EVENT(Event::NextVideoMode,       "Select next TV effect mode");
      ADD_EVENT(Event::PreviousVideoMode,   "Select previous TV effect mode");
      ADD_EVENT(Event::NextAttribute,       "Select next 'Custom' attribute");
      ADD_EVENT(Event::PreviousAttribute,   "Select previous 'Custom' attr.");
      ADD_EVENT(Event::IncreaseAttribute,   "Increase 'Custom' attribute");
      ADD_EVENT(Event::DecreaseAttribute,   "Decrease 'Custom' attribute");
      ADD_EVENT(Event::PhosphorIncrease,    "Increase phosphor blend");
      ADD_EVENT(Event::PhosphorDecrease,    "Decrease phosphor blend");
      ADD_EVENT(Event::ScanlinesIncrease,   "Increase scanline intensity");
      ADD_EVENT(Event::ScanlinesDecrease,   "Decrease scanline intensity");
      break;

    case 4:
      title = "Developer commands";
      ADD_EVENT(Event::DebuggerMode,        "Toggle debugger mode");
      ADD_EVENT(Event::ToggleFrameStats,    "Toggle frame stats");
      ADD_EVENT(Event::ToggleJitter,        "Toggle TV 'jitter'");
      ADD_EVENT(Event::ToggleColorLoss,     "Toggle PAL color loss");
      ADD_EVENT(Event::ToggleCollisions,    "Toggle collisions");
      ADD_EVENT(Event::ToggleFixedColors,   "Toggle 'Debug colors' mode");
      ADD_LINE();
      ADD_EVENT(Event::ToggleTimeMachine,   "Toggle 'Time Machine' mode");
      ADD_EVENT(Event::SaveAllStates,       "Save all 'Time Machine' states");
      ADD_EVENT(Event::LoadAllStates,       "Load all 'Time Machine' states");
      break;

    case 5:
      title = "All other commands";
      ADD_BIND("Remapped Events", "");
      ADD_TEXT("Most other commands can be");
      ADD_TEXT("remapped. Please consult the");
      ADD_TEXT("'Options/Input" + ELLIPSIS + "' dialog for");
      ADD_TEXT("more information.");
      setHelpAnchor("Remapping");
      break;

    default:
      break;
  }

  while(i < lines)
    ADD_LINE();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::displayInfo()
{
  string titleStr;
  updateStrings(myPage, LINES_PER_PAGE, titleStr);

  myTitle->setLabel(titleStr);
  for(uInt8 i = 0; i < LINES_PER_PAGE; ++i)
  {
    myKey[i]->setLabel(myKeyStr[i]);
    myDesc[i]->setLabel(myDescStr[i]);

    if(BSPF::containsIgnoreCase(myDescStr[i], "Options/Input" + ELLIPSIS))
      myDesc[i]->setUrl("https://stella-emu.github.io/docs/index.html#Remapping",
                        "Options/Input" + ELLIPSIS);
    else
      // extract URL from label
      myDesc[i]->setUrl();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::handleCommand(CommandSender* sender, int cmd,
                               int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kNextCmd:
      ++myPage;
      if(myPage >= myNumPages)
        myNextButton->clearFlags(Widget::FLAG_ENABLED);
      if(myPage >= 2)
        myPrevButton->setFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    case GuiObject::kPrevCmd:
      --myPage;
      if(myPage <= myNumPages)
        myNextButton->setFlags(Widget::FLAG_ENABLED);
      if(myPage <= 1)
        myPrevButton->clearFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    case kUpdateCmd:
      MediaFactory::openURL("https://stella-emu.github.io/downloads.html?version="
                            + instance().settings().getString("stella.version"));
      break;

    case StaticTextWidget::kOpenUrlCmd:
    {
      const string& url = myDesc[id]->getUrl();

      if(!url.empty())
        MediaFactory::openURL(url);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
