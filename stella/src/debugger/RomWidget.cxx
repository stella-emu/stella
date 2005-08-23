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
// $Id: RomWidget.cxx,v 1.3 2005-08-23 18:32:51 stephena Exp $
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
  myRomList->setStyle(kSolidFill);
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
  switch(cmd)
  {
    case kListScrolledCmd:
      cerr << "data invalidated; refill list\n";
      break;

    case kListItemChecked:
      cerr << "(un)set a breakpoint at address " << data << endl;
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  /* FIXME
     We need logic here to only fill the grid at startup and when
     bankswitching.  At other times, we receive 'kListScrolledCmd'
     command, which means the current romlist view is invalid and
     should be filled with new data.
  */
cerr << "RomWidget::loadConfig()\n";
  fillGrid();
  myRomList->setDirty(); myRomList->draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::fillGrid()
{
  StringList l;
  BoolArray b;

  for(int i = 0; i < 50; ++i)
  {
    ostringstream tmp;
    tmp << "Test line " << i;
    l.push_back(tmp.str());

    b.push_back(false);
  }

  myRomList->setList(l, b);
}
