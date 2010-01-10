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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "OSystem.hxx"
#include "RamDebug.hxx"
#include "Widget.hxx"

#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss),
    myUndoAddress(-1),
    myUndoValue(-1),
    myCurrentRamBank(0)
{
  _type = kRamWidget;

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight(),
            bwidth  = 44,//font.getStringWidth("Undo  "),
            bheight = lineHeight + 2;
  int xpos, ypos, lwidth;

  // Create a 16x8 grid holding byte values (16 x 8 = 128 RAM bytes) with labels
  // Add a scrollbar, since there may be more than 128 bytes of RAM available
  xpos = x;  ypos = y + lineHeight;  lwidth = 4 * fontWidth;
  myRamGrid = new DataGridWidget(boss, font, xpos + lwidth, ypos,
                                 16, 8, 2, 8, kBASE_16, true);
  myRamGrid->setTarget(this);
  addFocusWidget(myRamGrid);

  // Create actions buttons to the left of the RAM grid
  xpos += lwidth + myRamGrid->getWidth() + 4;
  myUndoButton = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                  "Undo", kUndoCmd);
  myUndoButton->setTarget(this);

  ypos += bheight + 4;
  myRevertButton = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                    "Rev", kRevertCmd);
  myRevertButton->setTarget(this);

  ypos += 2 * bheight + 2;
  mySearchButton = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                    "Srch", kSearchCmd);
  mySearchButton->setTarget(this);

  ypos += bheight + 4;
  myCompareButton = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                     "Cmp", kCmpCmd);
  myCompareButton->setTarget(this);

  ypos += bheight + 4;
  myRestartButton = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                     "Rset", kRestartCmd);
  myRestartButton->setTarget(this);

  // Labels for RAM grid
  xpos = x;  ypos = y + lineHeight;
  myRamStart =
    new StaticTextWidget(boss, font, xpos, ypos - lineHeight,
                         font.getStringWidth("xxxx"), fontHeight,
                         "00xx", kTextAlignLeft);

  for(int col = 0; col < 16; ++col)
  {
    new StaticTextWidget(boss, font, xpos + col*myRamGrid->colWidth() + lwidth + 8,
                         ypos - lineHeight,
                         fontWidth, fontHeight,
                         Debugger::to_hex_4(col),
                         kTextAlignLeft);
  }
  for(int row = 0; row < 8; ++row)
  {
    myRamLabels[row] =
      new StaticTextWidget(boss, font, xpos + 5, ypos + row*lineHeight + 2,
                           3*fontWidth, fontHeight, "", kTextAlignLeft);
  }

  xpos = x + 10;  ypos += 9 * lineHeight;
  new StaticTextWidget(boss, font, xpos, ypos,
                       6*fontWidth, fontHeight,
                       "Label:", kTextAlignLeft);
  xpos += 6*fontWidth + 5;
  myLabel = new EditTextWidget(boss, font, xpos, ypos-2, 17*fontWidth,
                               lineHeight, "");
  myLabel->setEditable(false);

  xpos += 17*fontWidth + 20;
  new StaticTextWidget(boss, font, xpos, ypos, 4*fontWidth, fontHeight,
                       "Dec:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myDecValue = new EditTextWidget(boss, font, xpos, ypos-2, 4*fontWidth,
                                  lineHeight, "");
  myDecValue->setEditable(false);

  xpos += 4*fontWidth + 20;
  new StaticTextWidget(boss, font, xpos, ypos, 4*fontWidth, fontHeight,
                       "Bin:", kTextAlignLeft);
  xpos += 4*fontWidth + 5;
  myBinValue = new EditTextWidget(boss, font, xpos, ypos-2, 9*fontWidth,
                                  lineHeight, "");
  myBinValue->setEditable(false);

  // Calculate real dimensions
  _w = lwidth + myRamGrid->getWidth();
  _h = ypos + lineHeight - y;

  // Inputbox which will pop up when searching RAM
  StringList labels;
  labels.push_back("Search: ");
  myInputBox = new InputTextDialog(boss, font, labels);
  myInputBox->setTarget(this);

  // Start with these buttons disabled
  myCompareButton->clearFlags(WIDGET_ENABLED);
  myRestartButton->clearFlags(WIDGET_ENABLED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::~RamWidget()
{
  delete myInputBox;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the DataGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;

  RamDebug& dbg = instance().debugger().ramDebug();
  const RamState& state = (RamState&) dbg.getState();
  switch(cmd)
  {
    case kDGItemDataChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      // Attempt the write, and revert if it didn't succeed
      uInt8 oldval = dbg.read(state.rport[addr]);
      dbg.write(state.wport[addr], value);
      uInt8 newval = dbg.read(state.rport[addr]);
      if(value != newval)
      {
        myRamGrid->setValue(addr - myCurrentRamBank*128, newval, false);
        break;
      }

      myUndoAddress = addr;
      myUndoValue = oldval;

      myDecValue->setEditString(instance().debugger().valueToString(value, kBASE_10));
      myBinValue->setEditString(instance().debugger().valueToString(value, kBASE_2));
      myRevertButton->setEnabled(true);
      myUndoButton->setEnabled(true);
      break;
    }

    case kDGSelectionChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      myLabel->setEditString(
        instance().debugger().equates().getLabel(state.rport[addr], true));
      myDecValue->setEditString(instance().debugger().valueToString(value, kBASE_10));
      myBinValue->setEditString(instance().debugger().valueToString(value, kBASE_2));
      break;
    }

    case kRevertCmd:
      for(uInt32 i = 0; i < myOldValueList.size(); ++i)
        dbg.write(state.wport[i], myOldValueList[i]);
      fillGrid(true);
      break;

    case kUndoCmd:
      dbg.write(state.wport[myUndoAddress], myUndoValue);
      myUndoButton->setEnabled(false);
      fillGrid(false);
      break;

    case kSearchCmd:
      showInputBox(kSValEntered);
      break;

    case kCmpCmd:
      showInputBox(kCValEntered);
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
        parent().removeDialog();
      break;
    }

    case kCValEntered:
    {
      const string& result = doCompare(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        parent().removeDialog();
      break;
    }

    case kSetPositionCmd:
      myCurrentRamBank = data;
      showSearchResults();
      fillGrid(false);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::loadConfig()
{
  fillGrid(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::fillGrid(bool updateOld)
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  if(updateOld) myOldValueList.clear();

  RamDebug& dbg = instance().debugger().ramDebug();

  const RamState& state    = (RamState&) dbg.getState();
  const RamState& oldstate = (RamState&) dbg.getOldState();

  // Jump to the correct 128 byte 'window' in the RAM area
  // This assumes that the RAM areas are aligned on 128 byte boundaries
  // TODO - the boundary restriction may not always apply ...
  uInt32 start = myCurrentRamBank * 128;
  assert(start+128 <= state.ram.size());

  if(updateOld) myOldValueList = state.ram;

  for(uInt32 i = start; i < start + 16*8; ++i)
  {
    alist.push_back(i);
    vlist.push_back(state.ram[i]);
    changed.push_back(state.ram[i] != oldstate.ram[i]);
  }

  myRamGrid->setNumRows(state.ram.size() / 128);
  myRamGrid->setList(alist, vlist, changed);
  if(updateOld)
  {
    myRevertButton->setEnabled(false);
    myUndoButton->setEnabled(false);
  }

  // Update RAM labels
  char buf[5];
  sprintf(buf, "%04x", state.rport[start] & 0xff00);
  buf[2] = buf[3] = 'x';
  myRamStart->setLabel(buf);
  for(uInt32 i = start, row = 0; i < start + 16*8; i += 16, ++row)
  {
    sprintf(buf, "%02x:", state.rport[i] & 0x00ff);
    myRamLabels[row]->setLabel(buf);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::showInputBox(int cmd)
{
  // Add inputbox in the middle of the RAM widget
  uInt32 x = getAbsX() + ((getWidth() - myInputBox->getWidth()) >> 1);
  uInt32 y = getAbsY() + ((getHeight() - myInputBox->getHeight()) >> 1);
  myInputBox->show(x, y);
  myInputBox->setEditString("");
  myInputBox->setTitle("");
  myInputBox->setFocus(0);
  myInputBox->setEmitSignal(cmd);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamWidget::doSearch(const string& str)
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

  int searchVal = instance().debugger().stringToValue(str);

  // Clear the search array of previous items
  mySearchAddr.clear();
  mySearchValue.clear();
  mySearchState.clear();

  // Now, search all memory locations for this value, and add it to the
  // search array
  bool hitfound = false;
  RamDebug& dbg = instance().debugger().ramDebug();
  const RamState& state = (RamState&) dbg.getState();
  for(uInt32 addr = 0; addr < state.ram.size(); ++addr)
  {
    int value = state.ram[addr];
    if(comparisonSearch && searchVal != value)
    {
      mySearchState.push_back(false);
    }
    else
    {
      mySearchAddr.push_back(addr);
      mySearchValue.push_back(value);
      mySearchState.push_back(true);
      hitfound = true;
    }
  }

  // If we have some hits, enable the comparison methods
  if(hitfound)
  {
    mySearchButton->setEnabled(false);
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Finally, show the search results in the list
  showSearchResults();

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamWidget::doCompare(const string& str)
{
  bool comparitiveSearch = false;
  int searchVal = 0, offset = 0;

  if(str.length() == 0)
    return "Enter an absolute or comparitive value";

  // Do some pre-processing on the string
  string::size_type pos = str.find_first_of("+-", 0);
  if(pos > 0 && pos != string::npos)
  {
    // Only accept '+' or '-' at the start of the string
    return "Input must be [+|-]NUM";
  }

  // A comparitive search searches memory for locations that have changed by
  // the specified amount, vs. for exact values
  if(str[0] == '+' || str[0] == '-')
  {
    comparitiveSearch = true;
    bool negative = false;
    if(str[0] == '-')
      negative = true;

    string tmp = str;
    tmp.erase(0, 1);  // remove the operator
    offset = instance().debugger().stringToValue(tmp);
    if(negative)
      offset = -offset;
  }
  else
    searchVal = instance().debugger().stringToValue(str);

  // Now, search all memory locations previously 'found' for this value
  bool hitfound = false;
  RamDebug& dbg = instance().debugger().ramDebug();
  const RamState& state = (RamState&) dbg.getState();
  IntArray tempAddrList, tempValueList;
  mySearchState.clear();
  for(uInt32 i = 0; i < state.rport.size(); ++i)
    mySearchState.push_back(false);

  for(unsigned int i = 0; i < mySearchAddr.size(); ++i)
  {
    if(comparitiveSearch)
    {
      searchVal = mySearchValue[i] + offset;
      if(searchVal < 0 || searchVal > 255)
        continue;
    }

    int addr = mySearchAddr[i];
    if(dbg.read(state.rport[addr]) == searchVal)
    {
      tempAddrList.push_back(addr);
      tempValueList.push_back(searchVal);
      mySearchState[addr] = hitfound = true;
    }
  }

  // Update the searchArray for the new addresses and data
  mySearchAddr = tempAddrList;
  mySearchValue = tempValueList;

  // If we have some hits, enable the comparison methods
  if(hitfound)
  {
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Finally, show the search results in the list
  showSearchResults();

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::doRestart()
{
  // Erase all search buffers, reset to start mode
  mySearchAddr.clear();
  mySearchValue.clear();
  mySearchState.clear();
  showSearchResults();

  mySearchButton->setEnabled(true);
  myCompareButton->setEnabled(false);
  myRestartButton->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::showSearchResults()
{
  // Only update the search results for the bank currently being shown
  BoolArray temp;
  uInt32 start = myCurrentRamBank * 128;
  if(mySearchState.size() == 0 || start > mySearchState.size())
  {
    for(uInt32 i = 0; i < 128; ++i)
      temp.push_back(false);
  }
  else
  {
    for(uInt32 i = start; i < start + 128; ++i)
      temp.push_back(mySearchState[i]);
  }
  myRamGrid->setHiliteList(temp);
}
