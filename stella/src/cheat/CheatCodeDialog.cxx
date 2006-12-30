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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CheatCodeDialog.cxx,v 1.13 2006-12-30 22:26:27 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Props.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "CheatCodeDialog.hxx"
#include "GuiUtils.hxx"
#include "CheckListWidget.hxx"
#include "CheatManager.hxx"
#include "InputTextDialog.hxx"
#include "StringList.hxx"

#include "bspf.hxx"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::CheatCodeDialog(OSystem* osystem, DialogContainer* parent,
                                 const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  int xpos, ypos;
  WidgetArray wid;
  ButtonWidget* b;

  // List of cheats, with checkboxes to enable/disable
  xpos = 10;  ypos = 10;
  myCheatList = new CheckListWidget(this, font, xpos, ypos,
                                    _w - 25 - kButtonWidth, _h - 50);
  myCheatList->setStyle(kXFill);
  myCheatList->setEditable(false);
  wid.push_back(myCheatList);

  xpos += myCheatList->getWidth() + 15;  ypos = 15;
  b = addButton(font, xpos, ypos, "Add", kAddCheatCmd);
  wid.push_back(b);
  myEditButton = addButton(font, xpos, ypos+=20, "Edit", kEditCheatCmd);
  wid.push_back(myEditButton);
  myRemoveButton = addButton(font, xpos, ypos+=20, "Remove", kRemCheatCmd);
  wid.push_back(myRemoveButton);
  b = addButton(font, xpos, ypos+=30, "One shot", kAddOneShotCmd);
  wid.push_back(b);

  // Inputbox which will pop up when adding/editing a cheat
  StringList labels;
  labels.push_back("Name: ");
  labels.push_back("Code: ");
  myCheatInput = new InputTextDialog(this, font, labels, 0, 0);
  myCheatInput->setTarget(this);

  // Add OK and Cancel buttons  **** FIXME - coordinates
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  myCancelButton = addButton(font, _w - (kButtonWidth + 10), _h - 24,
                             "Cancel", kCloseCmd);
  wid.push_back(myCancelButton);
  addCancelWidget(myCancelButton);
#else
  myCancelButton = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24,
                             "Cancel", kCloseCmd);
  wid.push_back(myCancelButton);
  addCancelWidget(myCancelButton);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif

  addToFocusList(wid);
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

  const CheatList& list = instance()->cheat().list();
  for(unsigned int i = 0; i < list.size(); ++i)
  {
    l.push_back(list[i]->name());
    b.push_back(bool(list[i]->enabled()));
  }
  myCheatList->setList(l, b);

  // Redraw the list, auto-selecting the first item if possible
  myCheatList->setSelected(l.size() > 0 ? 0 : -1);

  bool enabled = (list.size() > 0);
  myEditButton->setEnabled(enabled);
  myRemoveButton->setEnabled(enabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::saveConfig()
{
  // Inspect checkboxes for enable/disable codes
  const CheatList& list = instance()->cheat().list();
  for(unsigned int i = 0; i < myCheatList->getList().size(); ++i)
  {
    if(myCheatList->getState(i))
      list[i]->enable();
    else
      list[i]->disable();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::addCheat()
{
  // We have to add the dialog first, so it can be centered
  // The methods after this depend on the dialog having the correct dimensions
  parent()->addDialog(myCheatInput);
  myCheatInput->setEditString("", 0);
  myCheatInput->setEditString("", 1);
  myCheatInput->setTitle("");
  myCheatInput->setFocus(0);
  myCheatInput->setEmitSignal(kCheatAdded);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::editCheat()
{
  int idx = myCheatList->getSelected();
  if(idx < 0)
    return;

  const CheatList& list = instance()->cheat().list();
  const string& name = list[idx]->name();
  const string& code = list[idx]->code();

  // We have to add the dialog first, so it can be centered
  // The methods after this depend on the dialog having the correct dimensions
  parent()->addDialog(myCheatInput);
  myCheatInput->setEditString(name, 0);
  myCheatInput->setEditString(code, 1);
  myCheatInput->setTitle("");
  myCheatInput->setFocus(1);
  myCheatInput->setEmitSignal(kCheatEdited);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::removeCheat()
{
  instance()->cheat().remove(myCheatList->getSelected());
  loadConfig();  // reload the cheat list
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::addOneShotCheat()
{
  // We have to add the dialog first, so it can be centered
  // The methods after this depend on the dialog having the correct dimensions
  parent()->addDialog(myCheatInput);
  myCheatInput->setEditString("One-shot cheat", 0);
  myCheatInput->setEditString("", 1);
  myCheatInput->setTitle("");
  myCheatInput->setFocus(1);
  myCheatInput->setEmitSignal(kOneShotCheatAdded);
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
      editCheat();
      break;

    case kAddCheatCmd:
      addCheat();
      break;

    case kEditCheatCmd:
      editCheat();
      break;

    case kCheatAdded:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      if(instance()->cheat().isValidCode(code))
      {
        instance()->cheat().add(name, code);
        parent()->removeDialog();
        loadConfig();  // show changes onscreen
        myCancelButton->setEnabled(false);  // cannot cancel when a new cheat added
      }
      else
        myCheatInput->setTitle("Invalid code");
      break;
    }

    case kCheatEdited:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      bool enable = myCheatList->getSelectedState();
      int idx = myCheatList->getSelected();
      if(instance()->cheat().isValidCode(code))
      {
        instance()->cheat().add(name, code, enable, idx);
        parent()->removeDialog();
        loadConfig();  // show changes onscreen
        myCancelButton->setEnabled(false);  // cannot cancel when a new cheat added
      }
      else
        myCheatInput->setTitle("Invalid code");
      break;
    }

    case kRemCheatCmd:
      removeCheat();
      break;

    case kAddOneShotCmd:
      addOneShotCheat();
      break;

    case kOneShotCheatAdded:
    {
      const string& name = myCheatInput->getResult(0);
      const string& code = myCheatInput->getResult(1);
      if(instance()->cheat().isValidCode(code))
      {
        instance()->cheat().addOneShot(name, code);
        parent()->removeDialog();
      }
      else
        myCheatInput->setTitle("Invalid code");
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
