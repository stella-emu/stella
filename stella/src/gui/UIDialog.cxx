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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: UIDialog.cxx,v 1.15 2008-07-25 12:41:41 stephena Exp $
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
#include "StringList.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"

#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem* osystem, DialogContainer* parent,
                   const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 5;
  int xpos, ypos, tabID;
  int lwidth, pwidth = font.getStringWidth("Standard");
  WidgetArray wid;
  StringMap items;

  // Set real dimensions
//  _w = 36 * fontWidth + 10;
//  _h = 10 * (lineHeight + 4) + 10;

  // The tab widget
  xpos = ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);
  addFocusWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Launcher options
  wid.clear();
  tabID = myTab->addTab(" Launcher ");
  lwidth = font.getStringWidth("Launcher Height: ");
      
  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                           lineHeight, "Launcher Width: ",
                                           lwidth, kLWidthChanged);
  myLauncherWidthSlider->setMinValue(320);
  myLauncherWidthSlider->setMaxValue(1920);
  myLauncherWidthSlider->setStepValue(10);
  wid.push_back(myLauncherWidthSlider);
  myLauncherWidthLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myLauncherWidthSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myLauncherWidthLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  myLauncherHeightSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                            lineHeight, "Launcher Height: ",
                                            lwidth, kLHeightChanged);
  myLauncherHeightSlider->setMinValue(240);
  myLauncherHeightSlider->setMaxValue(1200);
  myLauncherHeightSlider->setStepValue(10);
  wid.push_back(myLauncherHeightSlider);
  myLauncherHeightLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myLauncherHeightSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myLauncherHeightLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Launcher font
  items.clear();
  items.push_back("Small", "small");
  items.push_back("Large", "large");
  myLauncherFontPopup =
    new PopUpWidget(myTab, font, xpos, ypos+1, pwidth, lineHeight, items,
                    "Launcher Font: ", lwidth);
  wid.push_back(myLauncherFontPopup);
  ypos += lineHeight + 4;

  // ROM launcher info/snapshot viewer
  xpos += ((_w - 40 - font.getStringWidth("ROM Info viewer")) >> 1);
  myRomViewerCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "ROM Info viewer", 0);
  wid.push_back(myRomViewerCheckbox);

  // Add message concerning usage
  xpos = vBorder; ypos += 2*(lineHeight + 4);
  lwidth = font.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(myTab, font, xpos, ypos, lwidth, fontHeight,
                       "(*) Changes require application restart",
                       kTextAlignLeft);

  // Add items for tab 0
  addToFocusList(wid, tabID);

  //////////////////////////////////////////////////////////
  // 2) Debugger options
  wid.clear();
  tabID = myTab->addTab(" Debugger ");
  lwidth = font.getStringWidth("Debugger Height: ");
  xpos = ypos = vBorder;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                           lineHeight, "Debugger Width: ",
                                           lwidth, kDWidthChanged);
  myDebuggerWidthSlider->setMinValue(1030);
  myDebuggerWidthSlider->setMaxValue(1920);
  myDebuggerWidthSlider->setStepValue(10);
  wid.push_back(myDebuggerWidthSlider);
  myDebuggerWidthLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myDebuggerWidthSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerWidthLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  myDebuggerHeightSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                            lineHeight, "Debugger Height: ",
                                            lwidth, kDHeightChanged);
  myDebuggerHeightSlider->setMinValue(690);
  myDebuggerHeightSlider->setMaxValue(1200);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  myDebuggerHeightLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myDebuggerHeightSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerHeightLabel->setFlags(WIDGET_CLEARBG);

  // Add message concerning usage
  xpos = vBorder; ypos += 2*(lineHeight + 4);
  lwidth = font.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(myTab, font, xpos, ypos, lwidth, fontHeight,
                       "(*) Changes require application restart",
                       kTextAlignLeft);

  // Add items for tab 1
  addToFocusList(wid, tabID);

  //////////////////////////////////////////////////////////
  // 3) Misc. options
  wid.clear();
  tabID = myTab->addTab(" Misc. ");
  lwidth = font.getStringWidth("Mouse wheel scroll: ");
  xpos = ypos = vBorder;

  // UI Palette
  ypos += 1;
  items.clear();
  items.push_back("Standard", "1");
  items.push_back("Classic", "2");
  myPalettePopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   items, "Interface Palette: ", lwidth);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Number of lines a mouse wheel will scroll
  myWheelLinesSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                        lineHeight, "Mouse wheel scroll: ",
                                        lwidth, kWLinesChanged);
  myWheelLinesSlider->setMinValue(1);
  myWheelLinesSlider->setMaxValue(10);
  myWheelLinesSlider->setStepValue(1);
  wid.push_back(myWheelLinesSlider);
  myWheelLinesLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myWheelLinesSlider->getWidth() + 4,
                           ypos + 1, 2*fontWidth, fontHeight, "", kTextAlignLeft);
  myWheelLinesLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Add items for tab 2
  addToFocusList(wid, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

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
  instance().settings().getSize("launcherres", w, h);
  w = BSPF_max(w, 320);
  h = BSPF_max(h, 240);
  w = BSPF_min(w, 1920);
  h = BSPF_min(h, 1200);

  myLauncherWidthSlider->setValue(w);
  myLauncherWidthLabel->setValue(w);
  myLauncherHeightSlider->setValue(h);
  myLauncherHeightLabel->setValue(h);

  // Launcher font
  const string& s = instance().settings().getString("launcherfont");
  myLauncherFontPopup->setSelected(s, "small");

  // ROM launcher info viewer
  myRomViewerCheckbox->setState(instance().settings().getBool("romviewer"));

  // Debugger size
  instance().settings().getSize("debuggerres", w, h);
  w = BSPF_max(w, 1030);
  h = BSPF_max(h, 690);
  w = BSPF_min(w, 1920);
  h = BSPF_min(h, 1200);

  myDebuggerWidthSlider->setValue(w);
  myDebuggerWidthLabel->setValue(w);
  myDebuggerHeightSlider->setValue(h);
  myDebuggerHeightLabel->setValue(h);

  // UI palette
  const string& pal = instance().settings().getString("uipalette");
  myPalettePopup->setSelected(pal, "1");

  // Mouse wheel lines
  int mw = instance().settings().getInt("mwheel");
  if(mw < 1 || mw > 10) mw = 1;
  myWheelLinesSlider->setValue(mw);
  myWheelLinesLabel->setValue(mw);

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  // Launcher size
  instance().settings().setSize("launcherres", 
    myLauncherWidthSlider->getValue(), myLauncherHeightSlider->getValue());

  // Launcher font
  instance().settings().setString("launcherfont",
    myLauncherFontPopup->getSelectedTag());

  // ROM launcher info viewer
  instance().settings().setBool("romviewer", myRomViewerCheckbox->getState());

  // Debugger size
  instance().settings().setSize("debuggerres", 
    myDebuggerWidthSlider->getValue(), myDebuggerHeightSlider->getValue());

  // UI palette
  instance().settings().setString("uipalette",
    myPalettePopup->getSelectedTag());

  // Mouse wheel lines
  int mw = myWheelLinesSlider->getValue();
  instance().settings().setInt("mwheel", mw);
  ScrollBarWidget::setWheelLines(mw);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Launcher options
    {
      int w = BSPF_min(instance().desktopWidth(), 640u);
      int h = BSPF_min(instance().desktopHeight(), 480u);
      myLauncherWidthSlider->setValue(w);
      myLauncherWidthLabel->setValue(w);
      myLauncherHeightSlider->setValue(h);
      myLauncherHeightLabel->setValue(h);
      myRomViewerCheckbox->setState(false);
      break;
    }

    case 1:  // Debugger options
      myDebuggerWidthSlider->setValue(1030);
      myDebuggerWidthLabel->setValue(1030);
      myDebuggerHeightSlider->setValue(690);
      myDebuggerHeightLabel->setValue(690);
      break;

    case 2:  // Misc. options
      myPalettePopup->setSelected("1", "1");
      myWheelLinesSlider->setValue(4);
      myWheelLinesLabel->setValue(4);
      break;

    default:
      break;
  }

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
      instance().setUIPalette();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
