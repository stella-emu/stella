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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: HelpDialog.cxx,v 1.26 2008-12-25 23:05:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"

#include "HelpDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelpDialog::HelpDialog(OSystem* osystem, DialogContainer* parent,
                       const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myPage(1),
    myNumPages(4)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Add Previous, Next and Close buttons
  xpos = 10;  ypos = _h - buttonHeight - 10;
  myPrevButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Previous", kPrevCmd);
  myPrevButton->clearFlags(WIDGET_ENABLED);
  wid.push_back(myPrevButton);

  xpos += buttonWidth + 7;
  myNextButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Next", kNextCmd);
  wid.push_back(myNextButton);

  xpos = _w - buttonWidth - 10;
  ButtonWidget* b =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "Close", kCloseCmd);
  wid.push_back(b);
  addOKWidget(b);  addCancelWidget(b);

  xpos = 5;  ypos = 5;
  myTitle = new StaticTextWidget(this, font, xpos, ypos, _w - 10, fontHeight,
                                 "", kTextAlignCenter);

  int lwidth = 15 * fontWidth;
  xpos += 5;  ypos += lineHeight + 4;
  for(uInt8 i = 0; i < LINES_PER_PAGE; i++)
  {
    myKey[i] =
      new StaticTextWidget(this, font, xpos, ypos, lwidth,
                           fontHeight, "", kTextAlignLeft);
    myDesc[i] =
      new StaticTextWidget(this, font, xpos+lwidth, ypos, _w - xpos - lwidth - 5,
                           fontHeight, "", kTextAlignLeft);
    ypos += fontHeight;
  }

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelpDialog::~HelpDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::updateStrings(uInt8 page, uInt8 lines,
                               string& title, string*& key, string* &dsc)
{
  key = new string[lines];
  dsc = new string[lines];

  uInt8 i = 0;
  switch(page)
  {
#ifndef MAC_OSX
    case 1:
      title = "Common commands:";
      ADD_BIND("Ctrl Q",    "Quit emulation");
      ADD_BIND("Escape",    "Exit current game");
      ADD_BIND("Tab",       "Enter options menu");
      ADD_BIND("\\",        "Toggle command menu");
      ADD_BIND("Alt =",     "Increase window size");
      ADD_BIND("Alt -",     "Decrease window size");
      ADD_BIND("Alt Enter", "Toggle fullscreen /");
      ADD_BIND("",          "  windowed mode");
      ADD_BIND("Alt ]",     "Increase volume by 2%");
      ADD_BIND("Alt [",     "Decrease volume by 2%");
      break;

    case 2:
      title = "Special commands:";
      ADD_BIND("Ctrl g", "Grab mouse (keep in window)");
      ADD_BIND("Ctrl f", "Switch between NTSC/PAL/SECAM");
      ADD_BIND("Ctrl s", "Save game properties");
      ADD_BIND("",       "  to a new file");
      ADD_LINE;
      ADD_BIND("Ctrl 0", "Mouse emulates paddle 0");
      ADD_BIND("Ctrl 1", "Mouse emulates paddle 1");
      ADD_BIND("Ctrl 2", "Mouse emulates paddle 2");
      ADD_BIND("Ctrl 3", "Mouse emulates paddle 3");
      break;

    case 3:
      title = "Developer commands:";
      ADD_BIND("Alt PgUp",  "Increase Display.YStart");
      ADD_BIND("Alt PgDn",  "Decrease Display.YStart");
      ADD_BIND("Ctrl PgUp", "Increase Display.Height");
      ADD_BIND("Ctrl PgDn", "Decrease Display.Height");
      break;
#else
    case 1:
      title = "Common commands:";
      ADD_BIND("Cmd Q",        "Quit emulation");
      ADD_BIND("Escape",       "Exit current game");
      ADD_BIND("Tab",          "Enter options menu");
      ADD_BIND("\\",           "Toggle command menu");
      ADD_BIND("Cmd =",        "Increase window size");
      ADD_BIND("Cmd -",        "Decrease window size");
      ADD_BIND("Cmd Enter",    "Toggle fullscreen /");
      ADD_BIND("",             "  windowed mode");
      ADD_BIND("Shift-Cmd ]",  "Increase volume by 2%");
      ADD_BIND("Shift-Cmd [",  "Decrease volume by 2%");
      break;

    case 2:
      title = "Special commands:";
      ADD_BIND("Cmd g",       "Grab mouse (keep in window)");
      ADD_BIND("Cmd f",       "Switch between NTSC/PAL/SECAM");
      ADD_BIND("Cmd s",       "Save game properties");
      ADD_BIND("",            "  to a new file");
      ADD_LINE;
      ADD_BIND("Cmd 0",       "Mouse emulates paddle 0");
      ADD_BIND("Cmd 1",       "Mouse emulates paddle 1");
      ADD_BIND("Cmd 2",       "Mouse emulates paddle 2");
      ADD_BIND("Cmd 3",       "Mouse emulates paddle 3");
      break;

    case 3:
      title = "Developer commands:";
      ADD_BIND("Shift-Cmd PgUp", "Increase Display.YStart");
      ADD_BIND("Shift-Cmd PgDn", "Decrease Display.YStart");
      ADD_BIND("Cmd PgUp",       "Increase Display.Height");
      ADD_BIND("Cmd PgDn",       "Decrease Display.Height");
      break;
#endif
    case 4:
      title = "All other commands:";
      ADD_LINE;
      ADD_BIND("Remapped Events", "");
      ADD_TEXT("Most other commands can be");
      ADD_TEXT("remapped.  Please consult the");
      ADD_TEXT("'Input Settings' section for");
      ADD_TEXT("more information.");
      break;
  }

  while(i < lines)
    ADD_LINE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::displayInfo()
{
  string titleStr, *keyStr, *dscStr;

  updateStrings(myPage, LINES_PER_PAGE, titleStr, keyStr, dscStr);

  myTitle->setLabel(titleStr);
  for(uInt8 i = 0; i < LINES_PER_PAGE; i++)
  {
    myKey[i]->setLabel(keyStr[i]);
    myDesc[i]->setLabel(dscStr[i]);
  }

  delete[] keyStr;
  delete[] dscStr;

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HelpDialog::handleCommand(CommandSender* sender, int cmd,
                               int data, int id)
{
  switch(cmd)
  {
    case kNextCmd:
      myPage++;
      if(myPage >= myNumPages)
        myNextButton->clearFlags(WIDGET_ENABLED);
      if(myPage >= 2)
        myPrevButton->setFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    case kPrevCmd:
      myPage--;
      if(myPage <= myNumPages)
        myNextButton->setFlags(WIDGET_ENABLED);
      if(myPage <= 1)
        myPrevButton->clearFlags(WIDGET_ENABLED);

      displayInfo();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
