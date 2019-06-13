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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
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

#include "HelpDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelpDialog::HelpDialog(OSystem& osystem, DialogContainer& parent,
                       const GUI::Font& font)
  : Dialog(osystem, parent, font, "Help"),
    myPage(1),
    myNumPages(5)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 46 * fontWidth + 10;
  _h = 12 * lineHeight + 20 + _th;

  // Add Previous, Next and Close buttons
  xpos = 10;  ypos = _h - buttonHeight - 10;
  myPrevButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Previous", GuiObject::kPrevCmd);
  myPrevButton->clearFlags(Widget::FLAG_ENABLED);
  wid.push_back(myPrevButton);

  xpos += buttonWidth + 8;
  myNextButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Next", GuiObject::kNextCmd);
  wid.push_back(myNextButton);

  xpos = _w - buttonWidth - 10;
  ButtonWidget* b =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);

  xpos = 5;  ypos = 5 + _th;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - 10, fontHeight,
                                 "", TextAlign::Center);

  int lwidth = 13 * fontWidth;
  xpos += 5;  ypos += lineHeight + 4;
  for(uInt8 i = 0; i < LINES_PER_PAGE; ++i)
  {
    myKey[i] =
      new StaticTextWidget(this, font, xpos, ypos, lwidth,
                           fontHeight, "", TextAlign::Left);
    myDesc[i] =
      new StaticTextWidget(this, font, xpos+lwidth, ypos, _w - xpos - lwidth - 5,
                           fontHeight, "", TextAlign::Left);
    ypos += fontHeight;
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::updateStrings(uInt8 page, uInt8 lines, string& title)
{
  int i = 0;
  auto ADD_BIND = [&](const string& k, const string& d)
  {
    myKeyStr[i] = k;  myDescStr[i] = d;  i++;
  };
  auto ADD_EVENT = [&](const Event::Type e, const string & d)
  {
    ADD_BIND(instance().eventHandler().getMappingDesc(e, kEmulationMode), d);
  };
  auto ADD_TEXT = [&](const string& d) { ADD_BIND("", d); };
  auto ADD_LINE = [&]() { ADD_BIND("", ""); };

  switch(page)
  {
    case 1:
      title = "Common commands";
      ADD_EVENT(Event::Quit,                "Quit emulation");
      ADD_EVENT(Event::ExitMode,            "Exit current game");
      ADD_EVENT(Event::OptionsMenuMode,     "Enter 'Options' menu");
      ADD_EVENT(Event::CmdMenuMode,         "Toggle command menu");
      ADD_EVENT(Event::VidmodeIncrease,     "Increase window size");
      ADD_EVENT(Event::VidmodeDecrease,     "Decrease window size");
      ADD_EVENT(Event::ToggleFullScreen,    "Toggle fullscreen /");
      ADD_BIND("",                          "  windowed mode");
      ADD_EVENT(Event::IncreaseOverScan,    "Increase overscan in FS mode");
      ADD_EVENT(Event::DecreaseOverscan,    "Decrease overscan in FS mode");
      break;

    case 2:
      title = "Special commands";
      ADD_EVENT(Event::IncreaseFormat,      "Switch between NTSC/PAL/SECAM");
      ADD_EVENT(Event::TogglePalette,       "Switch palette");
      ADD_EVENT(Event::TogglePhosphor,      "Toggle 'phosphor' effect");
      ADD_LINE();
      ADD_EVENT(Event::ToggleGrabMouse,     "Grab mouse (keep in window)");
      ADD_EVENT(Event::HandleMouseControl,  "Toggle controller for mouse");
      ADD_EVENT(Event::ToggleSAPortOrder,   "Toggle Stelladaptor left/right");
      ADD_LINE();
      ADD_EVENT(Event::VolumeIncrease,      "Increase volume by 2%");
      ADD_EVENT(Event::VolumeDecrease,      "Decrease volume by 2%");
      break;

    case 3:
      title = "TV Filters";
      ADD_EVENT(Event::VidmodeStd,          "Disable filtering");
      ADD_EVENT(Event::VidmodeRGB,          "Enable 'RGB' mode");
      ADD_EVENT(Event::VidmodeSVideo,       "Enable 'S-Video' mode");
      ADD_EVENT(Event::VidModeComposite,    "Enable 'Composite' mode");
      ADD_EVENT(Event::VidModeBad,          "Enable 'Bad adjust' mode");
      ADD_EVENT(Event::VidModeCustom,       "Enable 'Custom' mode");
      ADD_EVENT(Event::NextAttribute,       "Select 'Custom' adjustable");
      ADD_EVENT(Event::IncreaseAttribute,   "Modify 'Custom' adjustable");
      ADD_EVENT(Event::IncreasePhosphor,    "Adjust phosphor blend");
      ADD_EVENT(Event::ScanlinesIncrease,   "Adjust scanline intensity");
      break;

    case 4:
      title = "Developer commands";
      ADD_EVENT(Event::DebuggerMode,        "Enter/exit debugger");
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
      ADD_LINE();
      ADD_BIND("Remapped Even", "ts");
      ADD_TEXT("Most other commands can be");
      ADD_TEXT("remapped. Please consult the");
      ADD_TEXT("'Options/Input" + ELLIPSIS + "' dialog for");
      ADD_TEXT("more information.");
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

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
