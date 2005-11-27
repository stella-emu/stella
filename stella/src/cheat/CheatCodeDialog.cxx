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
// $Id: CheatCodeDialog.cxx,v 1.4 2005-11-27 15:48:04 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Props.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "CheatCodeDialog.hxx"
#include "GuiUtils.hxx"
#include "CheckListWidget.hxx"
#include "CheatManager.hxx"

#include "bspf.hxx"

enum {
  kAddCheatCmd   = 'CHTA',
  kEditCheatCmd  = 'CHTE',
  kRemCheatCmd   = 'CHTR',
  kAddOneShotCmd = 'CHTO'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::CheatCodeDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const GUI::Font& font = instance()->font();
  int xpos, ypos;

  // List of cheats, with checkboxes to enable/disable
  xpos = 10;  ypos = 10;
  myCheatList = new CheckListWidget(this, font, xpos, ypos,
                                    _w - 25 - kButtonWidth, _h - 50);
  myCheatList->setStyle(kXFill);
  myCheatList->setEditable(false);
  myCheatList->setFlags(WIDGET_NODRAW_FOCUS);
  addFocusWidget(myCheatList);

  xpos += myCheatList->getWidth() + 15;  ypos = 15;
  addButton(xpos, ypos, "Add", kAddCheatCmd, 0);
  addButton(xpos, ypos+=20, "Edit", kEditCheatCmd, 0);
  addButton(xpos, ypos+=20, "Remove", kRemCheatCmd, 0);
  addButton(xpos, ypos+=30, "One shot", kAddOneShotCmd, 0);

/*
Move this to new dialog
  xpos = 10;  ypos = 10 + myCheatList->getHeight() + 10;
  myTitle = new StaticTextWidget(this, xpos, ypos, lwidth, fontHeight,
                                 "Cheat Code", kTextAlignLeft);

  xpos += myTitle->getWidth();
  myInput = new EditTextWidget(this, xpos, ypos-1, 48, fontHeight, "");

  xpos = 10;  ypos += fontHeight + 5;
  myError = new StaticTextWidget(this, xpos, ypos, lwidth, kFontHeight,
                                 "", kTextAlignLeft);
  myError->setColor(kTextColorEm);
*/


  // Add OK and Cancel buttons
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::~CheatCodeDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::loadConfig()
{
  // Load items from CheatManager
  // Note that the items are always in the same order/number as given in
  // the CheatManager, so the arrays will be one-to-one
  StringList l;
  BoolArray b;

  const CheatList& list = instance()->cheat().myCheatList;
  for(unsigned int i = 0; i < list.size(); ++i)
  {
    l.push_back(list[i]->name());
    b.push_back(bool(list[i]->enabled()));
  }
  myCheatList->setList(l, b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::saveConfig()
{
  // Inspect checkboxes for enable/disable codes
  for(unsigned int i = 0; i < myCheatList->getList().size(); ++i)
  {
    if(myCheatList->getState(i))
      instance()->cheat().myCheatList[i]->enable();
    else
      instance()->cheat().myCheatList[i]->disable();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::addCheat()
{
cerr << "CheatCodeDialog::addCheat()\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::editCheat(int cheatNumber)
{
cerr << "CheatCodeDialog::editCheat() " << cheatNumber << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::removeCheat(int cheatNumber)
{
cerr << "CheatCodeDialog::removeCheat() " << cheatNumber << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kCloseCmd:
      close();
      break;

    case kListItemDoubleClickedCmd:
      editCheat(myCheatList->getSelected());
      break;

    case kAddCheatCmd:
      addCheat();
      break;

    case kEditCheatCmd:
      editCheat(myCheatList->getSelected());
      break;

    case kRemCheatCmd:
      removeCheat(myCheatList->getSelected());
      break;

    case kAddOneShotCmd:
      cerr << "add one-shot cheat\n";
      break;

/*
    case kEditAcceptCmd:
    {
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
    }

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
*/
    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
