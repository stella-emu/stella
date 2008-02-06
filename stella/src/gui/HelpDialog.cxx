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
// $Id: HelpDialog.cxx,v 1.24 2008-02-06 13:45:23 stephena Exp $
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
  WidgetArray wid;

  // Add Previous, Next and Close buttons
  myPrevButton = addButton(font, 10, h - 24, "Previous", kPrevCmd);
  myPrevButton->clearFlags(WIDGET_ENABLED);
  wid.push_back(myPrevButton);

  myNextButton = addButton(font, (kButtonWidth + 15), h - 24,
                           "Next", kNextCmd);
  wid.push_back(myNextButton);

  ButtonWidget* b = addButton(font, w - (kButtonWidth + 10), h - 24,
                              "Close", kCloseCmd);
  wid.push_back(b);
  addOKWidget(b);  addCancelWidget(b);

  myTitle = new StaticTextWidget(this, font, 5, 5, w - 10, font.getFontHeight(),
                                 "", kTextAlignCenter);
  for(uInt8 i = 0; i < LINES_PER_PAGE; i++)
  {
    myKey[i]  = new StaticTextWidget(this, font, 10, 18 + (10 * i), 80,
                                     font.getFontHeight(), "", kTextAlignLeft);
    myDesc[i] = new StaticTextWidget(this, font, 90, 18 + (10 * i), 160,
                                     font.getFontHeight(), "", kTextAlignLeft);
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
      ADD_BIND("Alt Enter", "Toggle fullscreen/windowed mode");
      ADD_LINE;
      ADD_BIND("Alt ]",     "Increase volume by 2%");
      ADD_BIND("Alt [",     "Decrease volume by 2%");
      break;

    case 2:
      title = "Special commands:";
      ADD_BIND("Ctrl g", "Grab mouse (keep in window)");
      ADD_BIND("Ctrl f", "Switch between NTSC/PAL/PAL60");
      ADD_BIND("Ctrl s", "Save game properties to new file");
      ADD_LINE;
      ADD_BIND("Ctrl 0", "Mouse emulates paddle 0");
      ADD_BIND("Ctrl 1", "Mouse emulates paddle 1");
      ADD_BIND("Ctrl 2", "Mouse emulates paddle 2");
      ADD_BIND("Ctrl 3", "Mouse emulates paddle 3");
      break;

    case 3:
      title = "Developer commands:";
      ADD_BIND("Alt PageUp",    "Increase Display.YStart");
      ADD_BIND("Alt PageDown",  "Decrease Display.YStart");
      ADD_BIND("Ctrl PageUp",   "Increase Display.Height");
      ADD_BIND("Ctrl PageDown", "Decrease Display.Height");
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
      ADD_BIND("Cmd Enter",    "Toggle fullscreen/windowed mode");
      ADD_LINE;
      ADD_BIND("Shift-Cmd ]",  "Increase volume by 2%");
      ADD_BIND("Shift-Cmd [",  "Decrease volume by 2%");
      break;

    case 2:
      title = "Special commands:";
      ADD_BIND("Cmd g",       "Grab mouse (keep in window)");
      ADD_BIND("Cmd f",       "Switch between NTSC/PAL/PAL60");
      ADD_BIND("Cmd s",       "Save game properties to new file");
      ADD_LINE;
      ADD_BIND("Cmd 0",       "Mouse emulates paddle 0");
      ADD_BIND("Cmd 1",       "Mouse emulates paddle 1");
      ADD_BIND("Cmd 2",       "Mouse emulates paddle 2");
      ADD_BIND("Cmd 3",       "Mouse emulates paddle 3");
      break;

    case 3:
      title = "Developer commands:";
      ADD_BIND("Shift-Cmd PageUp",    "Increase Display.YStart");
      ADD_BIND("Shift-Cmd PageDown",  "Decrease Display.YStart");
      ADD_BIND("Cmd PageUp",          "Increase Display.Height");
      ADD_BIND("Cmd PageDown",        "Decrease Display.Height");
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
