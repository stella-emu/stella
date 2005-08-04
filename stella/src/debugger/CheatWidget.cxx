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
// $Id: CheatWidget.cxx,v 1.2 2005-08-04 16:31:23 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiUtils.hxx"
#include "GuiObject.hxx"
#include "RamDebug.hxx"
#include "Widget.hxx"
#include "EditNumWidget.hxx"
#include "AddrValueWidget.hxx"
#include "InputTextDialog.hxx"

#include "CheatWidget.hxx"

enum {
  kSearchCmd   = 'CSEA',
  kCmpCmd      = 'CCMP',
  kRestartCmd  = 'CRST',
  kSValEntered = 'CSVE',
  kCValEntered = 'CCVE'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::CheatWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss),
    myInputBox(NULL)
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
  myEditBox->setFont(instance()->consoleFont());
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
  xpos = 200;  ypos = border/2;
  new StaticTextWidget(boss, xpos + 5, ypos, 75, kLineHeight,
                       "Address    Value", kTextAlignLeft);
  ypos += kLineHeight;

  myResultsList = new AddrValueWidget(boss, xpos, ypos, 100, 75, 0xff);
  myResultsList->setFont(instance()->consoleFont());
  myResultsList->setTarget(this);

  myInputBox = new InputTextDialog(boss, instance()->consoleFont());
  myInputBox->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatWidget::~CheatWidget()
{
  mySearchArray.clear();
  myCompareArray.clear();

  delete myInputBox;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kSearchCmd:
      myInputBox->setEditString("");
      myInputBox->setTitle("");
      myInputBox->setEmitSignal(kSValEntered);
      parent()->addDialog(myInputBox);
      break;

    case kCmpCmd:
      myInputBox->setEditString("");
      myInputBox->setTitle("");
      myInputBox->setEmitSignal(kCValEntered);
      parent()->addDialog(myInputBox);
      break;

    case kRestartCmd:
      doRestart();
      break;

    case kSValEntered:
    {
      const string& result = doSearch(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        parent()->removeDialog();
      break;
    }

    case kCValEntered:
    {
      const string& result = doCompare(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        parent()->removeDialog();
      break;
    }

    case kAVItemDataChangedCmd:
      int addr  = myResultsList->getSelectedAddr() - kRamStart;
      int value = myResultsList->getSelectedValue();
      instance()->debugger().ramDebug().write(addr, value);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string CheatWidget::doSearch(const string& str)
{
  bool comparisonSearch = true;

  if(str.length() == 0)
  {
    // An empty field means return all memory locations
    comparisonSearch = false;
  }
  else if(str.find_first_of("+-", 0) != string::npos)
  {
    // Don't accept these characters here, only in compare
    return "Invalid input +|-";
  }

  int searchVal = instance()->debugger().stringToValue(str);

  // Clear the search array of previous items
  mySearchArray.clear();

  // Now, search all memory locations for this value, and add it to the
  // search array
  RamDebug& dbg = instance()->debugger().ramDebug();
  AddrValue av;
  int searchCount = 0;
  for(int addr = 0; addr < kRamSize; ++addr)
  {
    if(comparisonSearch)
    {
      av.addr  = addr;
      av.value = searchVal;

      if(dbg.read(av.addr) == av.value)
      {
        mySearchArray.push_back(av);
        ++searchCount;
      }
    }
    else  // match all memory locations
    {
      av.addr  = addr;
      av.value = dbg.read(av.addr);
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

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string CheatWidget::doCompare(const string& str)
{
  bool comparitiveSearch = false;
  int searchVal = 0;

  if(str.length() == 0)
    return "Enter an absolute or comparitive value";

  // Do some pre-processing on the string
  string::size_type pos = str.find_first_of("+-", 0);
  if(pos > 0 && pos != string::npos)
  {
    // Only accept '+' or '-' at the start of the string
    return "Input must be [+|-]NUM";
  }

  if(str[0] == '+' || str[0] == '-')
  {
    bool negative = false;
    if(str[0] == '-')
      negative = true;

    string tmp = str;
    tmp.erase(0, 1);  // remove the operator
    searchVal = instance()->debugger().stringToValue(tmp);
    if(negative)
      searchVal = -searchVal;

    comparitiveSearch = true;
  }
  else
    searchVal = instance()->debugger().stringToValue(str);

  AddrValueList tempList;

  // Now, search all memory locations specified in mySearchArray for this value
  RamDebug& dbg = instance()->debugger().ramDebug();
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

    if(dbg.read(av.addr) == av.value)
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

  return "";
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
  AddrList alist;
  ValueList vlist;

  for(unsigned int i = 0; i < mySearchArray.size(); i++)
  {
    alist.push_back(kRamStart + mySearchArray[i].addr);
    vlist.push_back(mySearchArray[i].value);
  }

  myResultsList->setList(alist, vlist);
}
