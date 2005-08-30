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
// $Id: DataGridOpsWidget.cxx,v 1.1 2005-08-30 17:51:26 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "DataGridOpsWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataGridOpsWidget::DataGridOpsWidget(GuiObject* boss, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    _zeroButton(NULL),
    _invButton(NULL),
    _negButton(NULL),
    _incButton(NULL),
    _decButton(NULL),
    _shiftLeftButton(NULL),
    _shiftRightButton(NULL)
{
  const int bwidth  = _font->getMaxCharWidth() * 3,
            bheight = 16, // FIXME - magic number
            space = 6;
  int xpos, ypos;

  // Create operations buttons
  xpos = x;  ypos = y;
  _zeroButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                 "0", kDGZeroCmd, 0);

  ypos += bheight + space;
  _invButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                "Inv", kDGInvertCmd, 0);

  ypos += bheight + space;
  _incButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                "++", kDGIncCmd, 0);

  ypos += bheight + space;
  _shiftLeftButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                      "<<", kDGShiftLCmd, 0);

  // Move to next column, skip a row
  xpos = x + bwidth + space;  ypos = y + bheight + space;
  _negButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                "Neg", kDGNegateCmd, 0);

  ypos += bheight + space;
  _decButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                "--", kDGDecCmd, 0);

  ypos += bheight + space;
  _shiftRightButton = new ButtonWidget(boss, xpos, ypos, bwidth, bheight,
                                       ">>", kDGShiftRCmd, 0);

  // Calculate real dimensions
  _w = xpos + bwidth;
  _h = ypos + bheight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::setTarget(CommandReceiver* target)
{
  _zeroButton->setTarget(target);
  _invButton->setTarget(target);
  _negButton->setTarget(target);
  _incButton->setTarget(target);
  _decButton->setTarget(target);
  _shiftLeftButton->setTarget(target);
  _shiftRightButton->setTarget(target);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::setEnabled(bool e)
{
  _zeroButton->setEnabled(e);
  _invButton->setEnabled(e);
  _negButton->setEnabled(e);
  _incButton->setEnabled(e);
  _decButton->setEnabled(e);
  _shiftLeftButton->setEnabled(e);
  _shiftRightButton->setEnabled(e);
}
