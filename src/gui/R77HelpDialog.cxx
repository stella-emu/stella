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

#include "bspf.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Font.hxx"

#include "R77HelpDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
R77HelpDialog::R77HelpDialog(OSystem& osystem, DialogContainer& parent,
  const GUI::Font& font)
  : Dialog(osystem, parent, font, "Basic Help"),
  myPage(1),
  myNumPages(4)
{
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight(),
    buttonWidth = font.getStringWidth("Defaults") + 20,
    buttonHeight = font.getLineHeight() + 4;
  const int HBORDER = 10;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  _w = 47 * fontWidth + HBORDER * 2;
  _h = (LINES_PER_PAGE + 2) * lineHeight + 20 + _th;

  // Add Previous, Next and Close buttons
  xpos = HBORDER;  ypos = _h - buttonHeight - 10;
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

  xpos = _w - buttonWidth - HBORDER;
  ButtonWidget* b =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
      "Close", GuiObject::kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);

  xpos = HBORDER;  ypos = 5 + _th;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - HBORDER * 2, fontHeight,
    "", TextAlign::Center);

  int jwidth = 11 * fontWidth;
  int bwidth = 11 * fontWidth;
  xpos = HBORDER;  ypos += lineHeight + 4;
  for (uInt8 i = 0; i < LINES_PER_PAGE; ++i)
  {
    myJoy[i] =
      new StaticTextWidget(this, font, xpos, ypos, jwidth,
        fontHeight, "", TextAlign::Left);
    myBtn[i] =
      new StaticTextWidget(this, font, xpos + jwidth, ypos, bwidth,
        fontHeight, "", TextAlign::Left);
    myDesc[i] =
      new StaticTextWidget(this, font, xpos + jwidth + bwidth, ypos, _w - jwidth - bwidth - HBORDER * 2,
        fontHeight, "", TextAlign::Left);
    ypos += fontHeight;
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void R77HelpDialog::updateStrings(uInt8 page, uInt8 lines, string& title)
{
#ifdef BSPF_MACOS
#define ALT_ "Cmd"
#else
#define ALT_ "Alt"
#endif

  int i = 0;
  auto ADD_BIND = [&](const string & j = "", const string & b = "", const string & d = "")
  {
    myJoyStr[i] = j; myBtnStr[i] = b;  myDescStr[i] = d;  i++;
  };
  auto ADD_TEXT = [&](const string & d) { ADD_BIND("", d.substr(0, 11), d.substr(11, 40)); };
  auto ADD_LINE = [&]() { ADD_BIND("-----------", "-----------", "------------------------"); };

  switch (page)
  {
    case 1:
      title = "Emulation commands";
      ADD_BIND("The joystic", "ks work nor", "mal and all console");
      ADD_BIND("buttons as ", "labeled exc", "ept of the following:");
      ADD_BIND();
      ADD_BIND("Joystick", "Button", "Command");
      ADD_LINE();
      ADD_BIND("Button 3", "4:3,16:9", "Open command dialog");
      ADD_BIND("Button 4", "-", "Open settings");
      ADD_BIND("Button 5", "FRY", "Return to launcher");

      break;

    case 2:
      title = "Launcher commands";
      ADD_BIND("Joystick", "Button", "Command");
      ADD_LINE();
      ADD_BIND("Up", "SAVE", "Previous game");
      ADD_BIND("Down", "RESET", "Next game");
      ADD_BIND("Left", "LOAD", "Page up");
      ADD_BIND("Right", "MODE", "Page down");
      ADD_BIND("Button 1", "SKILL P1", "Start selected game");
      ADD_BIND("Button 2", "SKILL P2", "Open power-on options");
      ADD_BIND("Button 4", "Color,B/W", "Open settings");
      break;

    case 3:
      title = "Dialog commands";
      ADD_BIND("Joystick", "Button", "Command");
      ADD_LINE();
      ADD_BIND("Up", "SAVE", "Increase current setting");
      ADD_BIND("Down", "RESET", "Decrease current setting");
      ADD_BIND("Left", "LOAD", "Previous dialog element");
      ADD_BIND("Right", "MODE", "Next dialog element");
      ADD_BIND("Button 1", "SKILL P1", "Select");
      ADD_BIND("Button 2", "SKILL P2", "Cancel dialog");
      ADD_BIND("Button 3", "4:3,16:9", "Previous tab");
      ADD_BIND("Button 4", "FRY", "Next tab");
      break;

    case 4:
      title = "All commands";
      ADD_BIND();
      ADD_BIND("Remapped Ev", "ents", "");
      ADD_BIND();
      ADD_TEXT("Most commands can be remapped.");
      ADD_BIND();
      ADD_TEXT("Please use 'Advanced Settings'");
      ADD_TEXT("and consult the 'Options/Input" + ELLIPSIS + "'");
      ADD_TEXT("dialog for more information.");
      break;
  }

  while (i < lines)
    ADD_BIND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void R77HelpDialog::displayInfo()
{
  string titleStr;
  updateStrings(myPage, LINES_PER_PAGE, titleStr);

  myTitle->setLabel(titleStr);
  for (uInt8 i = 0; i < LINES_PER_PAGE; ++i)
  {
    myJoy[i]->setLabel(myJoyStr[i]);
    myBtn[i]->setLabel(myBtnStr[i]);
    myDesc[i]->setLabel(myDescStr[i]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void R77HelpDialog::handleCommand(CommandSender * sender, int cmd,
  int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kNextCmd:
      ++myPage;
      if (myPage >= myNumPages)
        myNextButton->clearFlags(Widget::FLAG_ENABLED);
      if (myPage >= 2)
        myPrevButton->setFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    case GuiObject::kPrevCmd:
      --myPage;
      if (myPage <= myNumPages)
        myNextButton->setFlags(Widget::FLAG_ENABLED);
      if (myPage <= 1)
        myPrevButton->clearFlags(Widget::FLAG_ENABLED);

      displayInfo();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
