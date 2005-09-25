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
// $Id: CheatCodeDialog.cxx,v 1.4 2005-09-25 20:18:46 urchlay Exp $
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

enum {
	kEnableCheat = 'ENAC',
	kLoadCmd = 'LDCH',
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::CheatCodeDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h)
{
  //	const GUI::Font& font = instance()->font();

  myTitle = new StaticTextWidget(this, 10, 5, w - 20, kFontHeight, "Cheat Code", kTextAlignCenter);
  myError = new StaticTextWidget(this, 10, 32, w - 20, kFontHeight, "Invalid Code", kTextAlignLeft);
  myError->setFlags(WIDGET_INVISIBLE);
  myInput = new EditTextWidget(this, 10, 20, 48, kFontHeight, "");
  addFocusWidget(myInput);
  addButton(w - (kButtonWidth + 10), h - 24, "Close", kCloseCmd, 'C');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatCodeDialog::~CheatCodeDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::handleCommand(CommandSender* sender, int cmd,
		int data, int id)
{
	switch(cmd)
	{
		case kEditAcceptCmd:
			cerr << myInput->getEditString() << endl;
			myCheat = Cheat::parse(instance(), myInput->getEditString());
			if(myCheat) {
				myError->setFlags(WIDGET_INVISIBLE);
				loadConfig();
				draw();
				myCheat->enable();
				delete myCheat; // TODO: keep and add to list
				Dialog::handleCommand(sender, kCloseCmd, data, id);
			} else {
				cerr << "bad cheat code" << endl;
				myInput->setEditString("");
				myError->clearFlags(WIDGET_INVISIBLE);
				loadConfig();
				draw();
				Dialog::handleCommand(sender, cmd, data, 0);
			}
			break;

		case kEditCancelCmd:
			Dialog::handleCommand(sender, kCloseCmd, data, id);
			break;

		default:
			Dialog::handleCommand(sender, cmd, data, 0);
			break;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::loadConfig() {
}
