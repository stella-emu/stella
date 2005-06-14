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
// $Id: CheatWidget.cxx,v 1.3 2005-06-14 18:55:36 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "Debugger.hxx"
#include "Widget.hxx"
#include "EditNumWidget.hxx"
#include "ListWidget.hxx"

#include "CheatWidget.hxx"

enum {
  kSearchCmd  = 'CSEA',
  kCmpCmd     = 'CCMP',
  kRestartCmd = 'CRST'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::CheatWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss)
{
  const int border = 20;
  const int bwidth = 50;
  const int charWidth  = instance()->consoleFont().getMaxCharWidth();
  const int charHeight = instance()->consoleFont().getFontHeight() + 2;
  int xpos = x + border;
  int ypos = y + border;

  // Add the numedit label and box
  new StaticTextWidget(boss, xpos, ypos, 70, kLineHeight,
                       "Enter a value:", kTextAlignLeft);

  myEditBox = new EditNumWidget(boss, 90, ypos - 2, charWidth*10, charHeight, "");
  myEditBox->setFont(_boss->instance()->consoleFont());
//  myEditBox->setTarget(this);
  myActiveWidget = myEditBox;
    ypos += border;

  // Add the result text string area
  myResult = new StaticTextWidget(boss, border + 5, ypos, 175, kLineHeight,
                       "", kTextAlignLeft);
  myResult->setColor(kTextColorEm);

  // Add the three search-related buttons
  xpos = x + border;
  ypos += border * 2;
  mySearchButton  = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Search", kSearchCmd, 0);
  mySearchButton->setTarget(this);
    xpos += 8 + bwidth;

  myCompareButton = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Compare", kCmpCmd, 0);
  myCompareButton->setTarget(this);
    xpos += 8 + bwidth;

  myRestartButton = new ButtonWidget(boss, xpos, ypos, bwidth, 16,
                                     "Restart", kRestartCmd, 0);
  myRestartButton->setTarget(this);

  // Add the list showing the results of a search/compare
  // FIXME - change this to a AddrValueWidget (or something like that)
  xpos = 200;  ypos = border/2;

  new StaticTextWidget(boss, xpos + 10, ypos, 70, kLineHeight,
                       "Address  Value", kTextAlignLeft);
  ypos += kLineHeight;

  myResultsList = new ListWidget(boss, xpos, ypos, 100, 75);
  myResultsList->setNumberingMode(kListNumberingOff);
  myResultsList->setFont(instance()->consoleFont());
  myResultsList->setTarget(this);

  // Start in a known state
  doRestart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::~CheatWidget()
{
  mySearchArray.clear();
  myCompareArray.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch(cmd)
  {
    case kSearchCmd:
      doSearch();
      break;

    case kCmpCmd:
      doCompare();
      break;

    case kRestartCmd:
      doRestart();
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::doSearch()
{
  bool comparisonSearch = true;

  string str = myEditBox->getEditString();
  if(str.length() == 0)
  {
    // An empty field means return all memory locations
    comparisonSearch = false;
  }
  else if(str[0] == '+' || str[0] == '-')
  {
    // Don't accept these characters here, only in compare
    myEditBox->setEditString("");
    return;  // FIXME - message about invalid format
  }

  int searchVal = atoi(str.c_str());

  // Clear the search array of previous items
  mySearchArray.clear();

  // Now, search all memory locations for this value, and add it to the
  // search array
  Debugger& dbg = _boss->instance()->debugger();
  AddrValue av;
  int searchCount = 0;
  for(int addr = 0; addr < kRamSize; ++addr)
  {
    if(comparisonSearch)
    {
      av.addr  = addr;
      av.value = searchVal;

      if(dbg.readRAM(av.addr) == av.value)
      {
        mySearchArray.push_back(av);
        ++searchCount;
      }
    }
    else  // match all memory locations
    {
      av.addr  = addr;
      av.value = dbg.readRAM(av.addr);
      mySearchArray.push_back(av);
      ++searchCount;
    }
  }

  // If we have some hits, enable the comparison methods
  if(searchCount)
  {
    mySearchButton->setEnabled(false);
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Show number of hits
  ostringstream buf;
  buf << "Search found " << searchCount << " result(s)";
  myResult->setLabel(buf.str());

  // Finally, show the search results in the list
  fillResultsList();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::doCompare()
{
  bool comparitiveSearch = false;
  int searchVal = 0;

  string str = myEditBox->getEditString();
  if(str.length() == 0)
  {
    myResult->setLabel("Enter an absolute or comparitive value");
    return;
  }

  // Do some pre-processing on the string
  if(str[0] == '+' || str[0] == '-')
  {
    bool negative = false;
    if(str[0] == '-')
      negative = true;

    str.erase(0, 1);  // remove the operator
    searchVal = atoi(str.c_str());
    if(negative)
      searchVal = -searchVal;

    comparitiveSearch = true;
  }
  else
    searchVal = atoi(str.c_str());

  AddrValueList tempList;

  // Now, search all memory locations specified in mySearchArray for this value
  Debugger& dbg = _boss->instance()->debugger();
  AddrValue av;
  int searchCount = 0;

  // A comparitive search searches memory for locations that have changed by
  // the specified amount, vs. for exact values
  for(unsigned int i = 0; i < mySearchArray.size(); i++)
  {
    av.addr  = mySearchArray[i].addr;
    if(comparitiveSearch)
    {
      int temp = mySearchArray[i].value + searchVal;
      if(temp < 0 || temp > 255)  // skip values which are out of range
        continue;
      av.value = temp;
    }
    else
      av.value = searchVal;

    if(dbg.readRAM(av.addr) == av.value)
    {
      tempList.push_back(av);
      ++searchCount;
    }
  }

  // Update the searchArray to the new results
  mySearchArray = tempList;

  // If we have some hits, enable the comparison methods
  if(searchCount)
  {
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Show number of hits
  ostringstream buf;
  buf << "Compare found " << searchCount << " result(s)";
  myResult->setLabel(buf.str());

  // Finally, show the search results in the list
  fillResultsList();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::doRestart()
{
  // Erase all search buffers, reset to start mode
  mySearchArray.clear();
  myCompareArray.clear();
  myEditBox->setEditString("");
  myResult->setLabel("");
  fillResultsList();

  mySearchButton->setEnabled(true);
  myCompareButton->setEnabled(false);
  myRestartButton->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::fillResultsList()
{
  StringList l;  
  char buf[1024];

  // FIXME - add to an editable two-column widget instead
  for(unsigned int i = 0; i < mySearchArray.size(); i++)
  {
    snprintf(buf, 1023, "%s :   %3d",
             Debugger::to_hex_16(kRamStart + mySearchArray[i].addr),
             mySearchArray[i].value);
    l.push_back(buf);
  }

  myResultsList->setList(l);
}
