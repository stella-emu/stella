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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: GameInfoDialog.cxx,v 1.2 2005-04-28 19:28:33 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Props.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "GameInfoDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::GameInfoDialog(OSystem* osystem, uInt16 x, uInt16 y, uInt16 w, uInt16 h)
    : Dialog(osystem, x, y, w, h),
      myPage(1),
      myNumPages(2)
{
  // Add Previous, Next and Close buttons
  myPrevButton = addButton(10, h - 24, "Previous", kPrevCmd, 'P');
  myNextButton = addButton((kButtonWidth + 15), h - 24,
                           "Next", kNextCmd, 'N');
  addButton(w - (kButtonWidth + 10), h - 24, "Close", kCloseCmd, 'C');
  myPrevButton->clearFlags(WIDGET_ENABLED);

  myTitle = new StaticTextWidget(this, 0, 5, w, 16, "", kTextAlignCenter);
  for(uInt8 i = 0; i < LINES_PER_PAGE; i++)
  {
    myKey[i]  = new StaticTextWidget(this, 10, 18 + (10 * i), 80, 16, "", kTextAlignLeft);
    myDesc[i] = new StaticTextWidget(this, 90, 18 + (10 * i), 160, 16, "", kTextAlignLeft);
  }

  displayInfo();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GameInfoDialog::~GameInfoDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::updateStrings(uInt8 page, uInt8 lines,
                                   string& title, string*& key, string* &dsc)
{
  key = new string[lines];
  dsc = new string[lines];

  uInt8 i = 0;
  switch(page)
  {
    case 1:
      title = "Common game properties";
      ADD_BIND("Name:", myGameProperties.get("Cartridge.Name"));
      ADD_LINE;
      ADD_BIND("Manufacturer:", myGameProperties.get("Cartridge.Manufacturer"));
      ADD_BIND("Model:",        myGameProperties.get("Cartridge.ModelNo"));
      ADD_BIND("Rarity:",       myGameProperties.get("Cartridge.Rarity"));
      ADD_BIND("Note:",         myGameProperties.get("Cartridge.Note"));
      ADD_BIND("Type:",         myGameProperties.get("Cartridge.Type"));
      ADD_LINE;
      ADD_BIND("MD5sum:",       myGameProperties.get("Cartridge.MD5"));
      break;

    case 2:
      title = "Other game properties";
      ADD_BIND("TV Type:",          myGameProperties.get("Console.TelevisionType"));
      ADD_BIND("Left Controller:",  myGameProperties.get("Controller.Left"));
      ADD_BIND("Right Controller:", myGameProperties.get("Controller.Left"));
      ADD_BIND("Format:",           myGameProperties.get("Display.Format"));
      ADD_BIND("XStart:",           myGameProperties.get("Display.XStart"));
      ADD_BIND("Width:",            myGameProperties.get("Display.Width"));
      ADD_BIND("YStart:",           myGameProperties.get("Display.YStart"));
      ADD_BIND("Height:",           myGameProperties.get("Display.Height"));
      ADD_BIND("CPU Type:",         myGameProperties.get("Emulation.CPU"));
      ADD_BIND("Use HMoveBlanks:",  myGameProperties.get("Emulation.HmoveBlanks"));
      break;
  }

  while(i < lines)
    ADD_LINE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::displayInfo()
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

  instance()->frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GameInfoDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
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
      Dialog::handleCommand(sender, cmd, data);
  }
}
