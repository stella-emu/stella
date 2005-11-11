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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CheatCodeDialog.cxx,v 1.8 2005-11-11 21:44:19 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Props.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "CheatCodeDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

/*
enum {
  kEnableCheat = 'ENAC',
  kLoadCmd     = 'LDCH'
};
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::CheatCodeDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myErrorFlag(false)
{
  const GUI::Font& font = instance()->font();
  const int fontHeight = font.getFontHeight(),
            lwidth = font.getMaxCharWidth() * 8;
  int xpos, ypos;

  xpos = 10;  ypos = 10;
  myTitle = new StaticTextWidget(this, xpos, ypos, lwidth, fontHeight,
                                 "Cheat Code", kTextAlignLeft);

  xpos += myTitle->getWidth();
  myInput = new EditTextWidget(this, xpos, ypos-1, 48, fontHeight, "");
  addFocusWidget(myInput);

  xpos = 10;  ypos += fontHeight + 5;
  myError = new StaticTextWidget(this, xpos, ypos, lwidth, kFontHeight,
                                 "", kTextAlignLeft);
  myError->setColor(kTextColorEm);

  //	addButton(w - (kButtonWidth + 10), 5, "Close", kCloseCmd, 'C');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::~CheatCodeDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case kEditAcceptCmd:
      // cerr << myInput->getEditString() << endl;
      const Cheat* cheat = 
        instance()->cheat().add("DLG", myInput->getEditString(), true);

      if(cheat)
      {
        // make sure "invalid code" isn't showing any more:
        myError->setLabel("");
        myErrorFlag = false;

        // get out of menu mode (back to emulation):
        Dialog::handleCommand(sender, kCloseCmd, data, id);
        instance()->eventHandler().leaveMenuMode();
      }
      else  // parse() returned 0 (null)
      { 
        myInput->setEditString("");

        // show error message "invalid code":
        myError->setLabel("Invalid Code");
        myErrorFlag = true;

        // not sure this does anything useful:
        Dialog::handleCommand(sender, cmd, data, 0);
      }
      break;

    case kEditCancelCmd:
      Dialog::handleCommand(sender, kCloseCmd, data, id);
      instance()->eventHandler().leaveMenuMode();
      break;

    case kEditChangedCmd:
      // Erase the invalid message once editing is restarted
      if(myErrorFlag)
      {
        myError->setLabel("");
        myErrorFlag = false;
      }
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
