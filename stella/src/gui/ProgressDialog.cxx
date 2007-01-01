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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ProgressDialog.cxx,v 1.8 2007-01-01 18:04:54 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "ProgressDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProgressDialog::ProgressDialog(GuiObject* boss, const GUI::Font& font,
                               const string& message)
    : Dialog(boss->instance(), boss->parent(), 0, 0, 16, 16),
      myMessage(NULL),
      mySlider(NULL),
      myStart(0),
      myFinish(0),
      myStep(0)
{
  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos, ypos, lwidth;

  // Calculate real dimensions
  lwidth = font.getStringWidth(message);
  _w = lwidth + 2 * fontWidth;
  _h = lineHeight * 5;
  _x = (boss->getWidth() - _w) / 2;
  _y = (boss->getHeight() - _h) / 2;

  xpos = fontWidth; ypos = lineHeight;
  myMessage = new StaticTextWidget(this, font, xpos, ypos, lwidth, fontHeight,
                                   message, kTextAlignCenter);
  myMessage->setColor(kTextColorEm);

  xpos = fontWidth; ypos += 2 * lineHeight;
  mySlider = new SliderWidget(this, font, xpos, ypos, lwidth, lineHeight, "", 0, 0);
  mySlider->setMinValue(100);
  mySlider->setMaxValue(200);
  mySlider->setValue(100);  // Prevents the slider from initially drawing
                            // across the entire screen for a split-second

  parent()->addDialog(this);
  instance()->frameBuffer().update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProgressDialog::~ProgressDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::done()
{
  parent()->removeDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setMessage(const string& message)
{
  myMessage->setLabel(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setRange(int start, int finish, int step)
{
  myStart = start;
  myFinish = finish;
  myStep = step;
  myCurrentStep = 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ProgressDialog::setProgress(int progress)
{
  // Only increase the progress bar if we have arrived at a new step
  // IE, we only increase in intervals specified by setRange()
  int p = (int) (((double)progress / myFinish) * 100 + 100);
  if(p >= myCurrentStep)
  {
    myCurrentStep += myStep;
    mySlider->setValue(p);
    instance()->frameBuffer().update();
  }
}
