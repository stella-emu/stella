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
// $Id: CheatCodeDialog.cxx,v 1.1 2005-08-05 02:28:22 urchlay Exp $
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
  myTitle = new StaticTextWidget(this, 10, 5, w - 20, kFontHeight, "Cheat Codes", kTextAlignCenter);
  addButton(w - (kButtonWidth + 10), h - 24, "Close", kCloseCmd, 'C');
  addButton(w - (kButtonWidth + 10), h - 48, "Load", kLoadCmd, 'C');
  myEnableCheat = new CheckboxWidget(this, 10, 20, kButtonWidth+10, kLineHeight,
                                            "Enabled", kEnableCheat);
  myEnableCheat->setState(false);
  myCheat = 0;
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
		case kLoadCmd:
			myCheat = Cheat::parse("db000f");
			loadConfig();
			break;

		case kEnableCheat:
			if(!myCheat)
				myEnableCheat->setState(false);
			else if(myCheat->enabled())
				myCheat->disable();
			else
				myCheat->enable();

			break;

		default:
			Dialog::handleCommand(sender, cmd, data, 0);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatCodeDialog::loadConfig() {
	cerr << "CheatCodeDialog::loadConfig()" << endl;

	myEnableCheat->setState(myCheat && myCheat->enabled());
	myEnableCheat->setEnabled(myCheat != 0);
}
