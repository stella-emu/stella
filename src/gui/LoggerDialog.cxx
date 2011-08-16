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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

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
#include "Widget.hxx"

#include "LoggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LoggerDialog::LoggerDialog(OSystem* osystem, DialogContainer* parent,
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
                                   _w - 2 * xpos, _h - buttonHeight - ypos - 20 -
                                   2 * lineHeight);
  myLogInfo->setNumberingMode(kListNumberingOff);
  myLogInfo->setEditable(false);
  wid.push_back(myLogInfo);
  ypos += myLogInfo->getHeight() + 8;

  // Level of logging (how much info to print)
  xpos += 20;
  StringMap items;
  items.clear();
  items.push_back("None", "0");
  items.push_back("Basic", "1");
  items.push_back("Verbose", "2");
  myLogLevel =
    new PopUpWidget(this, font, xpos, ypos, font.getStringWidth("Verbose"),
                    lineHeight, items, "Log level: ",
                    font.getStringWidth("Log level: "));
  wid.push_back(myLogLevel);

  // Should log output also be shown on the console?
  xpos += myLogLevel->getWidth() + 30;
  myLogToConsole = new CheckboxWidget(this, font, xpos, ypos, "Print to console");
  wid.push_back(myLogToConsole);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Save log to disk", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LoggerDialog::~LoggerDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::loadConfig()
{
  StringParser parser(instance().logMessages());
  myLogInfo->setList(parser.stringList());
  myLogInfo->setSelected(0);

  myLogLevel->setSelected(instance().settings().getString("loglevel"), "1");
  myLogToConsole->setState(instance().settings().getBool("logtoconsole"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::saveConfig()
{
  instance().settings().setString("loglevel",
    myLogLevel->getSelectedTag());
  instance().settings().setBool("logtoconsole", myLogToConsole->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::saveLogFile()
{
  string path = AbstractFilesystemNode::getAbsolutePath("stella", "~", "log");
  FilesystemNode node(path);

  ofstream out(node.getPath(true).c_str(), ios::out);
  if(out.is_open())
  {
    out << instance().logMessages();
    out.close();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerDialog::handleCommand(CommandSender* sender, int cmd,
                                 int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      saveLogFile();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
