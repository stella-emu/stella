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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "Control.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "StringList.hxx"
#include "Widget.hxx"

#include "GlobalPropsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::
  GlobalPropsDialog(GuiObject* boss, const GUI::Font& font)
  : Dialog(&boss->instance(), &boss->parent(), 0, 0, 0, 0)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Right Difficulty: "),
      pwidth = font.getStringWidth("EFSC (64K H. Runner + ram)");
  WidgetArray wid;
  StringMap items;

  // Set real dimensions
  _w = lwidth + pwidth + fontWidth*3 + 15;
  _h = 10 * (lineHeight + 4) + buttonHeight + 10;

  xpos = 10;  ypos = 10;

  ////////////////////////////////////////////////////////////////////
  // The following items are also present in GameInfoDialog
  // If any changes are ever made here, GameInfoDialog should also
  // be updated accordingly
  ////////////////////////////////////////////////////////////////////

  // Bankswitch type
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Bankswitch type:", kTextAlignLeft);
  items.clear();
  items.push_back("Default", "DEFAULT");
  items.push_back("Auto-detect",          "AUTO-DETECT");
  items.push_back("0840 (8K ECONObank)",        "0840" );
  items.push_back("2K (2K Atari)",              "2K"   );
  items.push_back("3E (32K Tigervision)",       "3E"   );
  items.push_back("3F (512K Tigervision)",      "3F"   );
  items.push_back("4A50 (64K 4A50 + ram)",      "4A50" );
  items.push_back("4K (4K Atari)",              "4K"   );
  items.push_back("AR (Supercharger)",          "AR"   );
  items.push_back("CV (Commavid extra ram)",    "CV"   );
  items.push_back("DPC (Pitfall II)",           "DPC"  );
  items.push_back("E0 (8K Parker Bros)",        "E0"   );
  items.push_back("E7 (16K M-network)",         "E7"   );
  items.push_back("EF (64K H. Runner)",         "EF"   );
  items.push_back("EFSC (64K H. Runner + ram)", "EFSC" );
  items.push_back("F4 (32K Atari)",             "F4"   );
  items.push_back("F4SC (32K Atari + ram)",     "F4SC" );
  items.push_back("F6 (16K Atari)",             "F6"   );
  items.push_back("F6SC (16K Atari + ram)",     "F6SC" );
  items.push_back("F8 (8K Atari)",              "F8"   );
  items.push_back("F8SC (8K Atari + ram)",      "F8SC" );
  items.push_back("FASC (CBS RAM Plus)",        "FASC" );
  items.push_back("FE (8K Decathlon)",          "FE"   );
  items.push_back("MB (Dynacom Megaboy)",       "MB"   );
  items.push_back("MC (C. Wilkson Megacart)",   "MC"   );
  items.push_back("SB (128-256k SUPERbank)",    "SB"   );
  items.push_back("UA (8K UA Ltd.)",            "UA"   );
  items.push_back("X07 (64K AtariAge)",         "X07"  );
  myBSType = new PopUpWidget(this, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myBSType);
  ypos += lineHeight + 10;

  // Left difficulty
  pwidth = font.getStringWidth("Default");
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Left Difficulty:", kTextAlignLeft);
  items.clear();
  items.push_back("Default", "DEFAULT");
  items.push_back("B", "B");
  items.push_back("A", "A");
  myLeftDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                               pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myLeftDiff);
  ypos += lineHeight + 5;

  // Right difficulty
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "Right Difficulty:", kTextAlignLeft);
  // ... use same items as above
  myRightDiff = new PopUpWidget(this, font, xpos+lwidth, ypos,
                                pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myRightDiff);
  ypos += lineHeight + 5;

  // TV type
  new StaticTextWidget(this, font, xpos, ypos+1, lwidth, fontHeight,
                       "TV Type:", kTextAlignLeft);
  items.clear();
  items.push_back("Default", "DEFAULT");
  items.push_back("Color", "COLOR");
  items.push_back("B & W", "BLACKANDWHITE");
  myTVType = new PopUpWidget(this, font, xpos+lwidth, ypos,
                             pwidth, lineHeight, items, "", 0, 0);
  wid.push_back(myTVType);
  ypos += lineHeight + 5;

  xpos = 30;  ypos += 10;

  // Start with Select held down
  myHoldSelect = new CheckboxWidget(this, font, xpos, ypos,
                                    "Hold Select down");
  wid.push_back(myHoldSelect);
  ypos += lineHeight + 4;

  // Start with Reset held down
  myHoldReset = new CheckboxWidget(this, font, xpos, ypos,
                                   "Hold Reset down");
  wid.push_back(myHoldReset);
  ypos += lineHeight + 4;

  // Start with joy button 0 held down
  myHoldButton0 = new CheckboxWidget(this, font, xpos, ypos,
                                     "Hold Button 0 down");
  wid.push_back(myHoldButton0);

  // Add message concerning usage
  lwidth = font.getStringWidth("(*) These changes are not saved");
  new StaticTextWidget(this, font, 10, _h - 2*buttonHeight - 10, lwidth, fontHeight,
                       "(*) These changes are not saved", kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GlobalPropsDialog::~GlobalPropsDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::loadConfig()
{
  Settings& settings = instance().settings();

  myBSType->setSelected(settings.getString("bs"), "DEFAULT");
  myLeftDiff->setSelected(settings.getString("ld"), "DEFAULT");
  myRightDiff->setSelected(settings.getString("rd"), "DEFAULT");
  myTVType->setSelected(settings.getString("tv"), "DEFAULT");

  myHoldSelect->setState(settings.getBool("holdselect"));
  myHoldReset->setState(settings.getBool("holdreset"));
  myHoldButton0->setState(settings.getBool("holdbutton0"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::saveConfig()
{
  Settings& settings = instance().settings();
  string s;

  s = myBSType->getSelectedTag();
  if(s == "DEFAULT") s = "";
  settings.setString("bs", s);

  s = myLeftDiff->getSelectedTag();
  if(s == "DEFAULT") s = "";
  settings.setString("ld", s);

  s = myRightDiff->getSelectedTag();
  if(s == "DEFAULT") s = "";
  settings.setString("rd", s);

  s = myTVType->getSelectedTag();
  if(s == "DEFAULT") s = "";
  settings.setString("tv", s);

  settings.setBool("holdselect", myHoldSelect->getState());
  settings.setBool("holdreset", myHoldReset->getState());
  settings.setBool("holdbutton0", myHoldButton0->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::setDefaults()
{
  myBSType->setSelected("DEFAULT", "");
  myLeftDiff->setSelected("DEFAULT", "");
  myRightDiff->setSelected("DEFAULT", "");
  myTVType->setSelected("DEFAULT", "");

  myHoldSelect->setState(false);
  myHoldReset->setState(false);
  myHoldButton0->setState(false);

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GlobalPropsDialog::handleCommand(CommandSender* sender, int cmd,
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

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
