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
// $Id: RomWidget.cxx,v 1.2 2005-08-22 18:17:10 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "GuiObject.hxx"
#include "CheckListWidget.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss)
{
  int w = 58 * font.getMaxCharWidth(),
      h = 31 * font.getLineHeight();

  myRomList = new CheckListWidget(boss, font, x, y, w, h);
  myRomList->setTarget(this);
  addFocusWidget(myRomList);

  // Calculate real dimensions
  _w = myRomList->getWidth();
  _h = myRomList->getHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::~RomWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
cerr << "RomWidget::loadConfig()\n";
  fillGrid();
  myRomList->setDirty(); myRomList->draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::fillGrid()
{
  StringList l;

  for(int i = 0; i < 100; ++i)
  {
    ostringstream tmp;
    tmp << "Test line " << i;
    l.push_back(tmp.str());
  }

  myRomList->setList(l);
}
