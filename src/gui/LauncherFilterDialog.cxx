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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <algorithm>
#include <sstream>

#include "bspf.hxx"

#include "Control.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "StringList.hxx"
#include "Widget.hxx"
#include "LauncherDialog.hxx"

#include "LauncherFilterDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherFilterDialog::LauncherFilterDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 0, 0),
    CommandSender(boss)
{
  const int lineHeight   = font.getLineHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Show: "),
      pwidth = font.getStringWidth("ROMs ending with");
  WidgetArray wid;
  StringMap items;

  // Set real dimensions
  _w = 3 * buttonWidth;//lwidth + pwidth + fontWidth*5 + 10;

  xpos = 10;  ypos = 10;

  // Types of files to show
  items.clear();
  items.push_back("All files", "allfiles");
  items.push_back("All roms", "allroms");
  items.push_back("ROMs ending with", "__EXTS");
  myFileType =
    new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight, items,
                    "Show: ", lwidth, kFileTypeChanged);
  wid.push_back(myFileType);
  ypos += lineHeight + 10;

  // Different types of ROM extensions
  xpos = 40;
  myRomType[0] = new CheckboxWidget(this, font, xpos, ypos, ourRomTypes[0][0]);
  int rightcol = xpos + myRomType[0]->getWidth() + 10;
  myRomType[3] = new CheckboxWidget(this, font, xpos+rightcol, ypos, ourRomTypes[0][3]);
  ypos += lineHeight + 4;
  myRomType[1] = new CheckboxWidget(this, font, xpos, ypos, ourRomTypes[0][1]);
  myRomType[4] = new CheckboxWidget(this, font, xpos+rightcol, ypos, ourRomTypes[0][4]);
  ypos += lineHeight + 4;
  myRomType[2] = new CheckboxWidget(this, font, xpos, ypos, ourRomTypes[0][2]);
  ypos += lineHeight + 10;

  _h = ypos + buttonHeight + 20;

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherFilterDialog::~LauncherFilterDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::parseExts(StringList& list, const string& type)
{
  // Assume the list is empty before this method is called
  if(type == "allroms")
  {
    for(uInt32 i = 0; i < 5; ++i)
      list.push_back(ourRomTypes[1][i]);
  }
  else if(type != "allfiles")
  {
    // Since istringstream swallows whitespace, we have to make the
    // delimiters be spaces
    string exts = type, ext;
    replace(exts.begin(), exts.end(), ':', ' ');
    istringstream buf(exts);

    while(buf >> ext)
    {
      for(uInt32 i = 0; i < 5; ++i)
      {
        if(ourRomTypes[1][i] == ext)
        {
          list.push_back(ext);
          break;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherFilterDialog::isValidRomName(const string& name,
                                          const StringList& exts)
{
  string::size_type idx = name.find_last_of('.');
  if(idx != string::npos)
  {
    const char* ext = name.c_str() + idx + 1;

    for(uInt32 i = 0; i < exts.size(); ++i)
      if(BSPF_equalsIgnoreCase(ext, exts[i]))
        return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherFilterDialog::isValidRomName(const string& name, string& ext)
{
  string::size_type idx = name.find_last_of('.');
  if(idx != string::npos)
  {
    const char* e = name.c_str() + idx + 1;

    for(uInt32 i = 0; i < 5; ++i)
    {
      if(BSPF_equalsIgnoreCase(e, ourRomTypes[1][i]))
      {
        ext = e;
        return true;
      }
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::loadConfig()
{
  handleFileTypeChange(instance().settings().getString("launcherexts"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::saveConfig()
{
  const string& type = myFileType->getSelectedTag();
  if(type == "allfiles" || type == "allroms")
    instance().settings().setString("launcherexts", type);
  else
  {
    ostringstream buf;
    for(uInt32 i = 0; i < 5; ++i)
      if(myRomType[i]->getState())
        buf << ourRomTypes[1][i] << ":";

    // No ROMs selected means use all files
    if(buf.str() == "")
      instance().settings().setString("launcherexts", "allfiles");
    else
      instance().settings().setString("launcherexts", buf.str());
  }

  // Let parent know about the changes
  sendCommand(kReloadFiltersCmd, 0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::setDefaults()
{
  handleFileTypeChange("allfiles");

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::handleFileTypeChange(const string& type)
{
  bool enable = (type != "allfiles" && type != "allroms");
  for(uInt32 i = 0; i < 5; ++i)
    myRomType[i]->setEnabled(enable);

  if(enable)
  {
    myFileType->setSelected("__EXTS", "");

    // Since istringstream swallows whitespace, we have to make the
    // delimiters be spaces
    string exts = type, ext;
    replace(exts.begin(), exts.end(), ':', ' ');
    istringstream buf(exts);

    while(buf >> ext)
    {
      for(uInt32 i = 0; i < 5; ++i)
      {
        if(ourRomTypes[1][i] == ext)
        {
          myRomType[i]->setState(true);
          break;
        }
      }
    }
  }
  else
    myFileType->setSelected(type, "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFilterDialog::handleCommand(CommandSender* sender, int cmd,
                                         int data, int id)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    case kFileTypeChanged:
      handleFileTypeChange(myFileType->getSelectedTag());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* LauncherFilterDialog::ourRomTypes[2][5] = {
  { ".a26", ".bin", ".rom", ".zip", ".gz" },
  { "a26", "bin", "rom", "zip", "gz" }
};
