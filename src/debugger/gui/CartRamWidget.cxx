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

#include "PopUpWidget.hxx"
#include "InputTextDialog.hxx"
#include "CartRamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartRamWidget::CartRamWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartDebugWidget& cartDebug)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont(nfont),
    myFontWidth(lfont.getMaxCharWidth()),
    myFontHeight(lfont.getFontHeight()),
    myLineHeight(lfont.getLineHeight()),
    myButtonHeight(myLineHeight + 4),
    myCart(cartDebug),
    myCurrentRamBank(0)
{ 
  int lwidth = lfont.getStringWidth("Description: "),
            fwidth = w - lwidth - 20;
  const int bwidth  = lfont.getStringWidth("Compare "),
            bheight = myLineHeight + 2;

  EditTextWidget* etw = 0;
  ostringstream buf;
  int xpos = 2, ypos = 5;
  
  // Add RAM size
  myRamSize = myCart.internalRamSize();
  new StaticTextWidget(_boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "RAM Size: ", kTextAlignLeft);
  
  buf << myRamSize << " bytes";
  if(myRamSize >= 1024)
    buf << " / " << (myRamSize/1024) << "KB";
  
  etw = new EditTextWidget(boss, nfont, xpos+lwidth, ypos,
                         fwidth, myLineHeight, buf.str());
  etw->setEditable(false);
  ypos += myLineHeight + 4;
  
  // Add Description
  const string& desc = myCart.internalRamDescription();
  const uInt16 maxlines = 6;
  StringParser bs(desc, (fwidth - kScrollBarWidth) / myFontWidth);
  const StringList& sl = bs.stringList();
  uInt32 lines = sl.size();
  if(lines < 3) lines = 3;
  if(lines > maxlines) lines = maxlines;
  
  new StaticTextWidget(_boss, _font, xpos, ypos, lwidth,
                       myFontHeight, "Description: ", kTextAlignLeft);
  myDesc = new StringListWidget(boss, nfont, xpos+lwidth, ypos,
                                fwidth, lines * myLineHeight, false);
  myDesc->setEditable(false);
  myDesc->setList(sl);
  addFocusWidget(myDesc);
  
  ypos += myDesc->getHeight() + myLineHeight + 4;   
  
  // Add RAM grid
  xpos = _font.getStringWidth("xxxx");
  int maxrow = myRamSize / 16;
  if (maxrow > 16) maxrow = 16;
  myPageSize = maxrow * 16;
  myRamGrid = new DataGridWidget(_boss, _nfont, xpos, ypos,
                                16, maxrow, 2, 8, Common::Base::F_16, true);
  myRamGrid->setTarget(this);
  addFocusWidget(myRamGrid);
  
  // Create actions buttons to the left of the RAM grid
  int bx = xpos + myRamGrid->getWidth() + 4;
  int by = ypos;
  myUndoButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                  "Undo", kUndoCmd);
  myUndoButton->setTarget(this);
  
  by += bheight + 4;
  myRevertButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                    "Revert", kRevertCmd);
  myRevertButton->setTarget(this);
  
  by += 2 * bheight + 2;
  mySearchButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                    "Search", kSearchCmd);
  mySearchButton->setTarget(this);
  
  by += bheight + 4;
  myCompareButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                     "Compare", kCmpCmd);
  myCompareButton->setTarget(this);
  
  by += bheight + 4;
  myRestartButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                     "Reset", kRestartCmd);
  myRestartButton->setTarget(this);
  
  // Labels for RAM grid
  for(int col = 0; col < 16; ++col)
  {
    new StaticTextWidget(_boss, _font, xpos + col*myRamGrid->colWidth() + 8,
                         ypos - myLineHeight,
                         myFontWidth, myFontHeight,
                         Common::Base::toString(col, Common::Base::F_16_1),
                         kTextAlignLeft);
  }
  
//  xpos = 02 + lwidth - _font.getStringWidth("xxxx  ");
  myRamStart =
  new StaticTextWidget(_boss, _font, xpos - _font.getStringWidth("xxxx"), ypos - myLineHeight,
                       _font.getStringWidth("xxxx"), myFontHeight,
                       "00xx", kTextAlignLeft);
  
//  xpos = 2 + lwidth - _font.getStringWidth("xxx  ");
  int row;
  for(row = 0; row < maxrow; ++row)
  {
    new StaticTextWidget(_boss, _font, xpos - _font.getStringWidth("x "), ypos + row*myLineHeight + 2,
                         3*myFontWidth, myFontHeight, 
                         Common::Base::toString(row*16, Common::Base::F_16_1),
                         kTextAlignLeft);
  }     
  
  // for smaller grids, make sure RAM cell detail fields are below the RESET button
  if (maxrow < 8)
    row = 8 + 1;  
  else
    row = maxrow + 1;

  ypos += myLineHeight * row; 
  // We need to define these widgets from right to left since the leftmost
  // one resizes as much as possible

  // Add Binary display of selected RAM cell
  xpos = w - 13*myFontWidth - 20;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*myFontWidth, myFontHeight,
                       "Bin:", kTextAlignLeft);
  myBinValue = new EditTextWidget(boss, nfont, xpos + 4*myFontWidth + 5,
                                  ypos-2, 9*myFontWidth, myLineHeight, "");
  myBinValue->setEditable(false);
  
  // Add Decimal display of selected RAM cell
  xpos -= 8*myFontWidth + 5 + 20;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*myFontWidth, myFontHeight,
                       "Dec:", kTextAlignLeft);
  myDecValue = new EditTextWidget(boss, nfont, xpos + 4*myFontWidth + 5, ypos-2,
                                  4*myFontWidth, myLineHeight, "");
  myDecValue->setEditable(false);
  
  // Add Label of selected RAM cell
  int xpos_r = xpos - 20;
  xpos = x + 10;
  new StaticTextWidget(boss, lfont, xpos, ypos, 6*myFontWidth, myFontHeight,
                       "Label:", kTextAlignLeft);
  xpos += 6*myFontWidth + 5;
  myLabel = new EditTextWidget(boss, nfont, xpos, ypos-2, xpos_r-xpos,
                               myLineHeight, "");
  myLabel->setEditable(false);
  
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
void CartRamWidget::loadConfig()
{
  fillGrid(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::fillGrid(bool updateOld)
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;
  uInt32 start = myCurrentRamBank * myPageSize;
  ByteArray oldRam = myCart.internalRamOld(start, myPageSize);
  ByteArray currentRam = myCart.internalRamCurrent(start, myPageSize);
  
  for(uInt32 i=0; i<myPageSize;i++)
  {
    alist.push_back(i+start);
    vlist.push_back(currentRam[i]);
    changed.push_back(currentRam[i] != oldRam[i]);
  }
  
  if(updateOld) 
  {
    myOldValueList.clear();
    myOldValueList = myCart.internalRamCurrent(start, myCart.internalRamSize());
  }
  
  myRamGrid->setNumRows(myRamSize / myPageSize);
  myRamGrid->setList(alist, vlist, changed);
  if(updateOld)
  {
    myRevertButton->setEnabled(false);
    myUndoButton->setEnabled(false);
  }
  
  // Update RAM labels
  char buf[5];
  BSPF_snprintf(buf, 5, "%04X", start);
  buf[2] = buf[3] = 'x';
  myRamStart->setLabel(buf);  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::showInputBox(int cmd)
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
string CartRamWidget::doSearch(const string& str)
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
//  CartDebug& dbg = instance().debugger().cartDebug();
//  const CartState& state = (CartState&) dbg.getState();
  
  ByteArray currentRam = myCart.internalRamCurrent(0, myCart.internalRamSize());
  
  for(uInt32 addr = 0; addr < myCart.internalRamSize(); ++addr)
  {
    int value = currentRam[addr];
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
string CartRamWidget::doCompare(const string& str)
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
  ByteArray currentRam = myCart.internalRamCurrent(0, myCart.internalRamSize());

  IntArray tempAddrList, tempValueList;
  mySearchState.clear();
  for(uInt32 i = 0; i < myCart.internalRamSize(); ++i)
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
    if(currentRam[addr] == searchVal)
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
void CartRamWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  // We simply change the values in the DataGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;
  
  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();
      
      uInt8 oldval = myCart.internalRamGetValue(addr);
      
      myCart.internalRamSetValue(addr, value);
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
      
      myLabel->setText(myCart.internalRamLabel(addr));
      myDecValue->setText(Common::Base::toString(value, Common::Base::F_10));
      myBinValue->setText(Common::Base::toString(value, Common::Base::F_2));
      break;
    }
      
    case kRevertCmd:
      for(uInt32 i = 0; i < myOldValueList.size(); ++i)
        myCart.internalRamSetValue(i, myOldValueList[i]);
      fillGrid(true);
      break;
      
    case kUndoCmd:
      myCart.internalRamSetValue(myUndoAddress, myUndoValue);
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
void CartRamWidget::doRestart()
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
void CartRamWidget::showSearchResults()
{
  // Only update the search results for the bank currently being shown
  BoolArray temp;
  uInt32 start = myCurrentRamBank * myPageSize;
  if(mySearchState.size() == 0 || start > mySearchState.size())
  {
    for(uInt32 i = 0; i < myPageSize; ++i)
      temp.push_back(false);
  }
  else
  {
    for(uInt32 i = start; i < start + myPageSize; ++i)
      temp.push_back(mySearchState[i]);
  }
  myRamGrid->setHiliteList(temp);
}

