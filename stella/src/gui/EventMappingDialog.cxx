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
// $Id: EventMappingDialog.cxx,v 1.1 2005-04-04 02:19:22 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "ListWidget.hxx"
#include "Dialog.hxx"
#include "GuiUtils.hxx"
#include "EventHandler.hxx"
#include "EventMappingDialog.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingDialog::EventMappingDialog(OSystem* osystem, uInt16 x, uInt16 y,
                                       uInt16 w, uInt16 h)
    : Dialog(osystem, x, y, w, h)
{
  // Add Previous, Next and Close buttons
  addButton(10, h - 24, "Defaults", kDefaultsCmd, 0);
  addButton(w - (kButtonWidth + 10), h - 24, "OK", kCloseCmd, 0);

  new StaticTextWidget(this, 10, 8, 200, 16, "Select an event to remap:", kTextAlignCenter);
  myActionsList = new ListWidget(this, 10, 20, 200, 100);
  myActionsList->setNumberingMode(kListNumberingOff);

  string title = "Hello Test!";
  myActionTitle = new StaticTextWidget(this, 10, 125, 200, 16, title, kTextAlignCenter);
  myKeyMapping  = new StaticTextWidget(this, 10, 130, 200, 16, "", kTextAlignCenter);

  myActionTitle->setFlags(WIDGET_CLEARBG);
  myKeyMapping->setFlags(WIDGET_CLEARBG);

  // Add remap and erase buttons
  addButton(220, 30, "Map", kMapCmd, 0);
  addButton(220, 50, "Erase", kEraseCmd, 0);

  // Get actions names
  StringList l;

  for(int i = 0; i < 58; ++i)  // FIXME - create a size() method
    l.push_back(EventHandler::ourActionList[i].action);

  myActionsList->setList(l);

  myActionSelected = -1;
//  CEActions::Instance()->beginMapping(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMappingDialog::~EventMappingDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  cerr << "EventMappingDialog::handleKeyDown received: " << ascii << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventMappingDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  switch(cmd)
  {
    case kListSelectionChangedCmd:
      if(myActionsList->getSelected() >= 0)
      {
cerr << "Item selected: " << myActionsList->getSelected() << endl;

/*        char selection[100];

        sprintf(selection, "Associated key : %s", CEDevice::getKeyName(CEActions::Instance()->getMapping((ActionType)(_actionsList->getSelected() + 1))).c_str());
        _keyMapping->setLabel(selection);
        _keyMapping->draw();
*/
      }
      break;

    case kMapCmd:
cerr << "Remap item: " << myActionsList->getSelected() << endl;
      break;

    case kEraseCmd:
cerr << "Erase item: " << myActionsList->getSelected() << endl;
      break;

    case kDefaultsCmd:
cerr << "Set default mapping\n";
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}
