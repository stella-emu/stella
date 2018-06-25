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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================
#include <iostream>
#include <fstream>

#include "bspf.hxx"

#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "FSNode.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "PopUpWidget.hxx"
#include "StringListWidget.hxx"
#include "StringParser.hxx"
#include "StringList.hxx"
#include "Widget.hxx"

#include "LicenseDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LicenseDialog::LicenseDialog(OSystem* osystem, DialogContainer* parent,
                           const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    myLogInfo(NULL)
{
  const int lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Save log to disk") + 20,
            buttonHeight = font.getLineHeight() + 4;

  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  // This is one dialog that can take as much space as is available
  _w = BSPF_min(max_w, 480);
  _h = BSPF_min(max_h, 380);

  // Test listing of the log output
  xpos = 10;  ypos = 10;
  myLogInfo = new StringListWidget(this, instance().consoleFont(), xpos, ypos,
                                   _w - 2 * xpos, _h - ypos - lineHeight);
  myLogInfo->setNumberingMode(kListNumberingOff);
  myLogInfo->setEditable(false);
  wid.push_back(myLogInfo);
  ypos += myLogInfo->getHeight() + 8;

  // Level of logging (how much info to print)
  xpos += 20;

  // Should log output also be shown on the console?

  // Add Defaults, OK and Cancel buttons
  //ButtonWidget* b;
  //b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
  //                     buttonWidth, buttonHeight, "Close", kDefaultsCmd);
  //wid.push_back(b);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LicenseDialog::~LicenseDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LicenseDialog::loadConfig()
{
//  StringParser parser(instance().logMessages());
//  myLogInfo->setList(parser.stringList());
//  myLogInfo->setSelected(0);
}

void LicenseDialog::setFilename(string name){
	string line;
	ifstream license_file(name.c_str());
	if(license_file.is_open()){
		myLicenseTxt.clear();
		while(!license_file.eof()){
			getline(license_file, line);
			myLicenseTxt.push_back(line);
		}
		license_file.close();
		myLogInfo->setList(myLicenseTxt);
		myLogInfo->setSelected(0);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LicenseDialog::handleCommand(CommandSender* sender, int cmd,
                                 int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      break;

    case kDefaultsCmd:
	close();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

void LicenseDialog::handleJoyDown(int stick, int button){
  Event::Type e =
    instance().eventHandler().eventForJoyButton(stick, button, kMenuMode);

  if(e == Event::UISelect)
    close();
  else
    Dialog::handleJoyDown(stick, button);
}
