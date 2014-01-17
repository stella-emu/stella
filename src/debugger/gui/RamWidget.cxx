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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "OSystem.hxx"
#include "CartDebug.hxx"
#include "Widget.hxx"

#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss),
    myUndoAddress(-1),
    myUndoValue(-1),
    myCurrentRamBank(0)
{
  const int fontWidth  = lfont.getMaxCharWidth(),
            fontHeight = lfont.getFontHeight(),
            lineHeight = lfont.getLineHeight(),
            bwidth  = lfont.getStringWidth("Compare "),
            bheight = lineHeight + 2;
  int xpos, ypos, lwidth;

  // Create a 16x8 grid holding byte values (16 x 8 = 128 RAM bytes) with labels
  // Add a scrollbar, since there may be more than 128 bytes of RAM available
  xpos = x;  ypos = y + lineHeight;  lwidth = 4 * fontWidth;
  myRamGrid = new DataGridWidget(boss, nfont, xpos + lwidth, ypos,
                                 16, 8, 2, 8, Common::Base::F_16, true);
  myRamGrid->setTarget(this);
  addFocusWidget(myRamGrid);

  // Create actions buttons to the left of the RAM grid
  xpos += lwidth + myRamGrid->getWidth() + 4;
  myUndoButton = new ButtonWidget(boss, lfont, xpos, ypos, bwidth, bheight,
                                  "Undo", kUndoCmd);
  myUndoButton->setTarget(this);

  ypos += bheight + 4;
  myRevertButton = new ButtonWidget(boss, lfont, xpos, ypos, bwidth, bheight,
                                    "Revert", kRevertCmd);
  myRevertButton->setTarget(this);

  ypos += 2 * bheight + 2;
  mySearchButton = new ButtonWidget(boss, lfont, xpos, ypos, bwidth, bheight,
                                    "Search", kSearchCmd);
  mySearchButton->setTarget(this);

  ypos += bheight + 4;
  myCompareButton = new ButtonWidget(boss, lfont, xpos, ypos, bwidth, bheight,
                                     "Compare", kCmpCmd);
  myCompareButton->setTarget(this);

  ypos += bheight + 4;
  myRestartButton = new ButtonWidget(boss, lfont, xpos, ypos, bwidth, bheight,
                                     "Reset", kRestartCmd);
  myRestartButton->setTarget(this);

  // Remember position of right side of buttons
  int xpos_r = xpos + bwidth ;

  // Labels for RAM grid
  xpos = x;  ypos = y + lineHeight;
  myRamStart =
    new StaticTextWidget(boss, lfont, xpos, ypos - lineHeight,
                         lfont.getStringWidth("xxxx"), fontHeight,
                         "00xx", kTextAlignLeft);

  for(int col = 0; col < 16; ++col)
  {
    new StaticTextWidget(boss, lfont, xpos + col*myRamGrid->colWidth() + lwidth + 8,
                         ypos - lineHeight,
                         fontWidth, fontHeight,
                         Common::Base::toString(col, Common::Base::F_16_1),
                         kTextAlignLeft);
  }
  for(int row = 0; row < 8; ++row)
  {
    myRamLabels[row] =
      new StaticTextWidget(boss, lfont, xpos + 8, ypos + row*lineHeight + 2,
                           3*fontWidth, fontHeight, "", kTextAlignLeft);
  }

  ypos += 9 * lineHeight;

  // We need to define these widgets from right to left since the leftmost
  // one resizes as much as possible
  xpos = xpos_r - 13*fontWidth - 5;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*fontWidth, fontHeight,
                       "Bin:", kTextAlignLeft);
  myBinValue = new EditTextWidget(boss, nfont, xpos + 4*fontWidth + 5,
                                  ypos-2, 9*fontWidth, lineHeight, "");
  myBinValue->setEditable(false);

  xpos -= 8*fontWidth + 5 + 20;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*fontWidth, fontHeight,
                       "Dec:", kTextAlignLeft);
  myDecValue = new EditTextWidget(boss, nfont, xpos + 4*fontWidth + 5, ypos-2,
                                  4*fontWidth, lineHeight, "");
  myDecValue->setEditable(false);

  xpos_r = xpos - 20;
  xpos = x + 10;
  new StaticTextWidget(boss, lfont, xpos, ypos, 6*fontWidth, fontHeight,
                       "Label:", kTextAlignLeft);
  xpos += 6*fontWidth + 5;
  myLabel = new EditTextWidget(boss, nfont, xpos, ypos-2, xpos_r-xpos,
                               lineHeight, "");
  myLabel->setEditable(false);

  // Calculate real dimensions
  _w = lwidth + myRamGrid->getWidth();
  _h = ypos + lineHeight - y;

  // Inputbox which will pop up when searching RAM
  StringList labels;
  labels.push_back("Search: ");
  myInputBox = new InputTextDialog(boss, lfont, nfont, labels);
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

  CartDebug& dbg = instance().debugger().cartDebug();
  const CartState& state = (CartState&) dbg.getState();
  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      // Attempt the write, and revert if it didn't succeed
      uInt8 oldval = dbg.peek(state.rport[addr]);
      dbg.poke(state.wport[addr], value);
      uInt8 newval = dbg.peek(state.rport[addr]);
      if(value != newval)
      {
        myRamGrid->setValue(addr - myCurrentRamBank*128, newval, false);
        break;
      }

      myUndoAddress = addr;
      myUndoValue = oldval;

      myDecValue->setText(Common::Base::toString(value, Common::Base::F_10));
      myBinValue->setText(Common::Base::toString(value, Common::Base::F_2));
      myRevertButton->setEnabled(true);
      myUndoButton->setEnabled(true);
      break;
    }

    case DataGridWidget::kSelectionChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();

      myLabel->setText(dbg.getLabel(state.rport[addr], true));
      myDecValue->setText(Common::Base::toString(value, Common::Base::F_10));
      myBinValue->setText(Common::Base::toString(value, Common::Base::F_2));
      break;
    }

    case kRevertCmd:
      for(uInt32 i = 0; i < myOldValueList.size(); ++i)
        dbg.poke(state.wport[i], myOldValueList[i]);
      fillGrid(true);
      break;

    case kUndoCmd:
      dbg.poke(state.wport[myUndoAddress], myUndoValue);
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
        myInputBox->close();
      break;
    }

    case kCValEntered:
    {
      const string& result = doCompare(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        myInputBox->close();
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
void RamWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myRamGrid->setOpsWidget(w);
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

  CartDebug& dbg = instance().debugger().cartDebug();

  const CartState& state    = (CartState&) dbg.getState();
  const CartState& oldstate = (CartState&) dbg.getOldState();

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
  BSPF_snprintf(buf, 5, "%04X", state.rport[start] & 0xff00);
  buf[2] = buf[3] = 'x';
  myRamStart->setLabel(buf);
  for(uInt32 i = start, row = 0; i < start + 16*8; i += 16, ++row)
  {
    BSPF_snprintf(buf, 3, "%02X", state.rport[i] & 0x00ff);
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
  myInputBox->setText("");
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
  CartDebug& dbg = instance().debugger().cartDebug();
  const CartState& state = (CartState&) dbg.getState();
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
  CartDebug& dbg = instance().debugger().cartDebug();
  const CartState& state = (CartState&) dbg.getState();
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
    if(dbg.peek(state.rport[addr]) == searchVal)
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
