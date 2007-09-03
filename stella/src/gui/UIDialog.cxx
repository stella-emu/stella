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
// $Id: UIDialog.cxx,v 1.8 2007-09-03 18:37:24 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "bspf.hxx"

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "PopUpWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Settings.hxx"
#include "Widget.hxx"

#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem* osystem, DialogContainer* parent,
                   const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Debugger Height (*): "),
      pwidth = font.getStringWidth("Standard");
  WidgetArray wid;

  xpos = 10;  ypos = 10;

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(this, font, xpos, ypos, pwidth,
                                           lineHeight, "Launcher Width (*): ",
                                           lwidth, kLWidthChanged);
  myLauncherWidthSlider->setMinValue(320);
  myLauncherWidthSlider->setMaxValue(800);
  myLauncherWidthSlider->setStepValue(10);
  wid.push_back(myLauncherWidthSlider);
  myLauncherWidthLabel =
      new StaticTextWidget(this, font,
                           xpos + myLauncherWidthSlider->getWidth() + 4,
                           ypos + 1, 20, fontHeight, "", kTextAlignLeft);
  myLauncherWidthLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  myLauncherHeightSlider = new SliderWidget(this, font, xpos, ypos, pwidth,
                                            lineHeight, "Launcher Height (*): ",
                                            lwidth, kLHeightChanged);
  myLauncherHeightSlider->setMinValue(240);
  myLauncherHeightSlider->setMaxValue(600);
  myLauncherHeightSlider->setStepValue(10);
  wid.push_back(myLauncherHeightSlider);
  myLauncherHeightLabel =
      new StaticTextWidget(this, font,
                           xpos + myLauncherHeightSlider->getWidth() + 4,
                           ypos + 1, 20, fontHeight, "", kTextAlignLeft);
  myLauncherHeightLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(this, font, xpos, ypos, pwidth,
                                           lineHeight, "Debugger Width (*): ",
                                           lwidth, kDWidthChanged);
  myDebuggerWidthSlider->setMinValue(1030);
  myDebuggerWidthSlider->setMaxValue(1600);
  myDebuggerWidthSlider->setStepValue(10);
  wid.push_back(myDebuggerWidthSlider);
  myDebuggerWidthLabel =
      new StaticTextWidget(this, font,
                           xpos + myDebuggerWidthSlider->getWidth() + 4,
                           ypos + 1, 20, fontHeight, "", kTextAlignLeft);
  myDebuggerWidthLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  myDebuggerHeightSlider = new SliderWidget(this, font, xpos, ypos, pwidth,
                                            lineHeight, "Debugger Height (*): ",
                                            lwidth, kDHeightChanged);
  myDebuggerHeightSlider->setMinValue(690);
  myDebuggerHeightSlider->setMaxValue(1200);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  myDebuggerHeightLabel =
      new StaticTextWidget(this, font,
                           xpos + myDebuggerHeightSlider->getWidth() + 4,
                           ypos + 1, 20, fontHeight, "", kTextAlignLeft);
  myDebuggerHeightLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Number of lines a mouse wheel will scroll
  myWheelLinesSlider = new SliderWidget(this, font, xpos, ypos, pwidth,
                                        lineHeight, "Mouse wheel scroll: ",
                                        lwidth, kWLinesChanged);
  myWheelLinesSlider->setMinValue(1);
  myWheelLinesSlider->setMaxValue(10);
  myWheelLinesSlider->setStepValue(1);
  wid.push_back(myWheelLinesSlider);
  myWheelLinesLabel =
      new StaticTextWidget(this, font,
                           xpos + myWheelLinesSlider->getWidth() + 4,
                           ypos + 1, 20, fontHeight, "", kTextAlignLeft);
  myWheelLinesLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // UI Palette
  ypos += 1;
  myPalettePopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                   "Interface Palette: ", lwidth);
  myPalettePopup->appendEntry("Standard", 1);
  myPalettePopup->appendEntry("Classic", 2);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Add message concerning usage
  lwidth = font.getStringWidth("(*) Requires application restart");
  new StaticTextWidget(this, font, 10, _h - 38, lwidth, fontHeight,
                       "(*) Requires application restart",
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

#ifndef DEBUGGER_SUPPORT
  myDebuggerWidthSlider->clearFlags(WIDGET_ENABLED);
  myDebuggerWidthLabel->clearFlags(WIDGET_ENABLED);
  myDebuggerHeightSlider->clearFlags(WIDGET_ENABLED);
  myDebuggerHeightLabel->clearFlags(WIDGET_ENABLED);
#endif

#ifdef _WIN32_WCE
  myLauncherPopup->clearFlags(WIDGET_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::~UIDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::loadConfig()
{
  int w, h;

  // Launcher size
  instance()->settings().getSize("launcherres", w, h);
  if(w < 320) w = 320;
  if(w > 800) w = 800;
  if(h < 240) h = 240;
  if(h > 600) h = 600;

  myLauncherWidthSlider->setValue(w);
  myLauncherWidthLabel->setValue(w);
  myLauncherHeightSlider->setValue(h);
  myLauncherHeightLabel->setValue(h);

  // Debugger size
  instance()->settings().getSize("debuggerres", w, h);
  if(w < 1030) w = 1030;
  if(w > 1600) w = 1600;
  if(h < 690)  h = 690;
  if(h > 1200) h = 1200;

  myDebuggerWidthSlider->setValue(w);
  myDebuggerWidthLabel->setValue(w);
  myDebuggerHeightSlider->setValue(h);
  myDebuggerHeightLabel->setValue(h);

  // Mouse wheel lines
  int mw = instance()->settings().getInt("mwheel");
  if(mw < 1 || mw > 10) mw = 1;
  myWheelLinesSlider->setValue(mw);
  myWheelLinesLabel->setValue(mw);

  // UI palette
  int i = instance()->settings().getInt("uipalette");
  if(i < 1 || i > 2) i = 1;
  myPalettePopup->setSelectedTag(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  // Launcher size
  instance()->settings().setSize("launcherres", 
    myLauncherWidthSlider->getValue(), myLauncherHeightSlider->getValue());

  // Debugger size
  instance()->settings().setSize("debuggerres", 
    myDebuggerWidthSlider->getValue(), myDebuggerHeightSlider->getValue());

  // Mouse wheel lines
  int mw = myWheelLinesSlider->getValue();
  instance()->settings().setInt("mwheel", mw);
  ScrollBarWidget::setWheelLines(mw);

  // UI palette
  instance()->settings().setInt("uipalette",
    myPalettePopup->getSelectedTag());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
  int w = BSPF_min(instance()->desktopWidth(), (const uInt32) 400);
  int h = BSPF_min(instance()->desktopHeight(), (const uInt32) 300);
  myLauncherWidthSlider->setValue(w);
  myLauncherWidthLabel->setValue(w);
  myLauncherHeightSlider->setValue(h);
  myLauncherHeightLabel->setValue(h);

  myDebuggerWidthSlider->setValue(1030);
  myDebuggerWidthLabel->setValue(1030);
  myDebuggerHeightSlider->setValue(690);
  myDebuggerHeightLabel->setValue(690);

  myWheelLinesSlider->setValue(4);
  myWheelLinesLabel->setValue(4);

  myPalettePopup->setSelectedTag(1);

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kLWidthChanged:
      myLauncherWidthLabel->setValue(myLauncherWidthSlider->getValue());
      break;

    case kLHeightChanged:
      myLauncherHeightLabel->setValue(myLauncherHeightSlider->getValue());
      break;

    case kDWidthChanged:
      myDebuggerWidthLabel->setValue(myDebuggerWidthSlider->getValue());
      break;

    case kDHeightChanged:
      myDebuggerHeightLabel->setValue(myDebuggerHeightSlider->getValue());
      break;

    case kWLinesChanged:
      myWheelLinesLabel->setValue(myWheelLinesSlider->getValue());
      break;

    case kOKCmd:
      saveConfig();
      close();
      instance()->setUIPalette();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
