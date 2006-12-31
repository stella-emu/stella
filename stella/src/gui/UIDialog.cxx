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
// $Id: UIDialog.cxx,v 1.1 2006-12-31 23:20:13 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "PopUpWidget.hxx"
#include "Dialog.hxx"
#include "UIDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem* osystem, DialogContainer* parent,
                   const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Rom launcher size: "),
      pwidth = font.getStringWidth("xxxxxxx");
  WidgetArray wid;

  xpos = 10;  ypos = 10;

  // Launcher size
  myLauncherPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                    "Rom launcher size: ", lwidth);
  myLauncherPopup->appendEntry("320x240", 1);
  myLauncherPopup->appendEntry("400x300", 2);
  myLauncherPopup->appendEntry("512x384", 3);
  wid.push_back(myLauncherPopup);
  ypos += lineHeight + 4;

  // UI Palette
  myPalettePopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                   "Interface Palette: ", lwidth);
  myPalettePopup->appendEntry("Classic", 1);
  myPalettePopup->appendEntry("GP2X",    2);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Add message concerning usage
  lwidth = font.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(this, font, 10, _h - 38, lwidth, fontHeight,
                       "(*) Changes require application restart",
                       kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = addButton(font, 10, _h - 24, "Defaults", kDefaultsCmd);
  wid.push_back(b);
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::~UIDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::loadConfig()
{
  int i;

  // Launcher size
  i = instance()->settings().getInt("launchersize");
  if(i < 1 || i > 3)
    i = 1;
  myLauncherPopup->setSelectedTag(i);

  // UI palette
  i = instance()->settings().getInt("uipalette");
  if(i < 1 || i > 2)
    i = 1;
  myPalettePopup->setSelectedTag(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  Settings& settings = instance()->settings();
  int i;

  // Launcher size
  i = myLauncherPopup->getSelectedTag();
  settings.setInt("launchersize", i);

  // UI palette
  i = myPalettePopup->getSelectedTag();
  settings.setInt("uipalette", i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
#if !defined (GP2X)
  myLauncherPopup->setSelectedTag(2);
  myPalettePopup->setSelectedTag(1);
#else
  myLauncherPopup->setSelectedTag(1);
  myPalettePopup->setSelectedTag(2);
#endif

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
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
