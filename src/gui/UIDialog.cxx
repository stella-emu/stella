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

#include "bspf.hxx"

#include "Dialog.hxx"
#include "OSystem.hxx"
#include "ListWidget.hxx"
#include "PopUpWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "Settings.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif

#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font)
  : Dialog(osystem, parent)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  const int vBorder = 5;
  int xpos, ypos, tabID;
  int lwidth, pwidth = font.getStringWidth("Standard");
  WidgetArray wid;
  VariantList items;
  ButtonWidget* b;
  const GUI::Size& ds = instance().frameBuffer().desktopSize();

  // Set real dimensions
  _w = 37 * fontWidth + 10;
  _h = 11 * (lineHeight + 4) + 10;

  // The tab widget
  xpos = ypos = vBorder;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Launcher options
  tabID = myTab->addTab(" Launcher ");
  lwidth = font.getStringWidth("Exit to Launcher: ");

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                           lineHeight, "Launcher Width: ",
                                           lwidth, kLWidthChanged);
  myLauncherWidthSlider->setMinValue(FrameBuffer::kFBMinW);
  myLauncherWidthSlider->setMaxValue(ds.w);
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
  myLauncherHeightSlider->setMinValue(FrameBuffer::kFBMinH);
  myLauncherHeightSlider->setMaxValue(ds.h);
  myLauncherHeightSlider->setStepValue(10);
  wid.push_back(myLauncherHeightSlider);
  myLauncherHeightLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myLauncherHeightSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myLauncherHeightLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Launcher font
  pwidth = font.getStringWidth("2x (1000x760)");
  items.clear();
  VarList::push_back(items, "Small",  "small");
  VarList::push_back(items, "Medium", "medium");
  VarList::push_back(items, "Large",  "large");
  myLauncherFontPopup =
    new PopUpWidget(myTab, font, xpos, ypos+1, pwidth, lineHeight, items,
                    "Launcher Font: ", lwidth);
  wid.push_back(myLauncherFontPopup);
  ypos += lineHeight + 4;

  // ROM launcher info/snapshot viewer
  items.clear();
  VarList::push_back(items, "Off", "0");
  VarList::push_back(items, "1x (640x480) ", "1");
  VarList::push_back(items, "2x (1000x760)", "2");
  myRomViewerPopup =
    new PopUpWidget(myTab, font, xpos, ypos+1, pwidth, lineHeight, items,
                    "ROM Info viewer: ", lwidth);
  wid.push_back(myRomViewerPopup);
  ypos += lineHeight + 4;

  // Exit to Launcher
  pwidth = font.getStringWidth("If in use");
  items.clear();
  VarList::push_back(items, "If in use", "0");
  VarList::push_back(items, "Always", "1");
  myLauncherExitPopup =
    new PopUpWidget(myTab, font, xpos, ypos+1, pwidth, lineHeight, items,
                    "Exit to Launcher: ", lwidth);
  wid.push_back(myLauncherExitPopup);
  ypos += lineHeight + 4;

  // Add message concerning usage
  xpos = vBorder; ypos += 1*(lineHeight + 4);
  lwidth = ifont.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(myTab, ifont, xpos, ypos, BSPF_min(lwidth, _w-20), fontHeight,
                       "(*) Changes require application restart",
                       kTextAlignLeft);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////
  // 2) Debugger options
  wid.clear();
  tabID = myTab->addTab(" Debugger ");
#ifdef DEBUGGER_SUPPORT
  lwidth = font.getStringWidth("Debugger Height: ");
  xpos = ypos = vBorder;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos, pwidth,
                                           lineHeight, "Debugger Width: ",
                                           lwidth, kDWidthChanged);
  myDebuggerWidthSlider->setMinValue(DebuggerDialog::kSmallFontMinW);
  myDebuggerWidthSlider->setMaxValue(ds.w);
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
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  wid.push_back(myDebuggerHeightSlider);
  myDebuggerHeightLabel =
      new StaticTextWidget(myTab, font,
                           xpos + myDebuggerHeightSlider->getWidth() + 4,
                           ypos + 1, 4*fontWidth, fontHeight, "", kTextAlignLeft);
  myDebuggerHeightLabel->setFlags(WIDGET_CLEARBG);

  // Add minimum window size buttons for different fonts
  int fbwidth = font.getStringWidth("Set window size for medium font") + 20;
  xpos = (_w - fbwidth - 2*vBorder)/2;  ypos += 2*lineHeight + 4;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
      "Set window size for small font", kDSmallSize);
  wid.push_back(b);
  ypos += b->getHeight() + 4;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
      "Set window size for medium font", kDMediumSize);
  wid.push_back(b);
  ypos += b->getHeight() + 4;
  b = new ButtonWidget(myTab, font, xpos, ypos, fbwidth, buttonHeight,
      "Set window size for large font", kDLargeSize);
  wid.push_back(b);
  ypos += b->getHeight() + 12;

  // Font style (bold label vs. text, etc)
  lwidth = font.getStringWidth("Font Style: ");
  pwidth = font.getStringWidth("Bold non-labels only");
  xpos = vBorder;
  items.clear();
  VarList::push_back(items, "All Normal font", "0");
  VarList::push_back(items, "Bold labels only", "1");
  VarList::push_back(items, "Bold non-labels only", "2");
  VarList::push_back(items, "All Bold font", "3");
  myDebuggerFontStyle =
    new PopUpWidget(myTab, font, xpos, ypos+1, pwidth, lineHeight, items,
                    "Font Style: ", lwidth);
  wid.push_back(myDebuggerFontStyle);

  // Debugger is only realistically available in windowed modes 800x600 or greater
  // (and when it's actually been compiled into the app)
  bool debuggerAvailable = 
#if defined(DEBUGGER_SUPPORT) && defined(WINDOWED_SUPPORT)
    (ds.w >= 800 && ds.h >= 600);  // TODO - maybe this logic can disappear?
#else
  false;
#endif
  if(!debuggerAvailable)
  {
    myDebuggerWidthSlider->clearFlags(WIDGET_ENABLED);
    myDebuggerWidthLabel->clearFlags(WIDGET_ENABLED);
    myDebuggerHeightSlider->clearFlags(WIDGET_ENABLED);
    myDebuggerHeightLabel->clearFlags(WIDGET_ENABLED);
  }

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);
#else
  new StaticTextWidget(myTab, font, 0, 20, _w-20, fontHeight,
                       "Debugger support not included", kTextAlignCenter);
#endif

  //////////////////////////////////////////////////////////
  // 3) Misc. options
  wid.clear();
  tabID = myTab->addTab(" Misc. ");
  lwidth = font.getStringWidth("Interface Palette (*): ");
  pwidth = font.getStringWidth("Standard");
  xpos = ypos = vBorder;

  // UI Palette
  ypos += 1;
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  myPalettePopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   items, "Interface Palette (*): ", lwidth);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Delay between quick-selecting characters in ListWidget
  items.clear();
  VarList::push_back(items, "Disabled", "0");
  VarList::push_back(items, "300 ms", "300");
  VarList::push_back(items, "400 ms", "400");
  VarList::push_back(items, "500 ms", "500");
  VarList::push_back(items, "600 ms", "600");
  VarList::push_back(items, "700 ms", "700");
  VarList::push_back(items, "800 ms", "800");
  VarList::push_back(items, "900 ms", "900");
  VarList::push_back(items, "1 second", "1000");
  myListDelayPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                     items, "List quick delay (*): ", lwidth);
  wid.push_back(myListDelayPopup);
  ypos += lineHeight + 4;

  // Number of lines a mouse wheel will scroll
  items.clear();
  VarList::push_back(items, "1 line", "1");
  VarList::push_back(items, "2 lines", "2");
  VarList::push_back(items, "3 lines", "3");
  VarList::push_back(items, "4 lines", "4");
  VarList::push_back(items, "5 lines", "5");
  VarList::push_back(items, "6 lines", "6");
  VarList::push_back(items, "7 lines", "7");
  VarList::push_back(items, "8 lines", "8");
  VarList::push_back(items, "9 lines", "9");
  VarList::push_back(items, "10 lines", "10");
  myWheelLinesPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                      items, "Mouse wheel scroll: ", lwidth);
  wid.push_back(myWheelLinesPopup);
  ypos += lineHeight + 4;

  // Add message concerning usage
  xpos = vBorder; ypos += 1*(lineHeight + 4);
  lwidth = ifont.getStringWidth("(*) Requires application restart");
  new StaticTextWidget(myTab, ifont, xpos, ypos, BSPF_min(lwidth, _w-20), fontHeight,
                       "(*) Requires application restart",
                       kTextAlignLeft);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::~UIDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::loadConfig()
{
  // Launcher size
  const GUI::Size& ls = instance().settings().getSize("launcherres");
  uInt32 w = ls.w, h = ls.h;

  w = BSPF_max(w, (uInt32)FrameBuffer::kFBMinW);
  h = BSPF_max(h, (uInt32)FrameBuffer::kFBMinH);
  w = BSPF_min(w, instance().frameBuffer().desktopSize().w);
  h = BSPF_min(h, instance().frameBuffer().desktopSize().h);

  myLauncherWidthSlider->setValue(w);
  myLauncherWidthLabel->setValue(w);
  myLauncherHeightSlider->setValue(h);
  myLauncherHeightLabel->setValue(h);

  // Launcher font
  const string& font = instance().settings().getString("launcherfont");
  myLauncherFontPopup->setSelected(font, "medium");

  // ROM launcher info viewer
  const string& viewer = instance().settings().getString("romviewer");
  myRomViewerPopup->setSelected(viewer, "0");

  // Exit to launcher
  bool exitlauncher = instance().settings().getBool("exitlauncher");
  myLauncherExitPopup->setSelected(exitlauncher ? "1" : "0", "0");

#ifdef DEBUGGER_SUPPORT
  // Debugger size
  const GUI::Size& ds = instance().settings().getSize("dbg.res");
  w = ds.w, h = ds.h;
  w = BSPF_max(w, (uInt32)DebuggerDialog::kSmallFontMinW);
  h = BSPF_max(h, (uInt32)DebuggerDialog::kSmallFontMinH);
  w = BSPF_min(w, ds.w);
  h = BSPF_min(h, ds.h);

  myDebuggerWidthSlider->setValue(w);
  myDebuggerWidthLabel->setValue(w);
  myDebuggerHeightSlider->setValue(h);
  myDebuggerHeightLabel->setValue(h);

  // Debugger font style
  int style = instance().settings().getInt("dbg.fontstyle");
  myDebuggerFontStyle->setSelected(style, "0");
#endif

  // UI palette
  const string& pal = instance().settings().getString("uipalette");
  myPalettePopup->setSelected(pal, "standard");

  // Listwidget quick delay
  const string& delay = instance().settings().getString("listdelay");
  myListDelayPopup->setSelected(delay, "300");

  // Mouse wheel lines
  const string& mw = instance().settings().getString("mwheel");
  myWheelLinesPopup->setSelected(mw, "1");

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  // Launcher size
  instance().settings().setValue("launcherres", 
    GUI::Size(myLauncherWidthSlider->getValue(),
              myLauncherHeightSlider->getValue()));

  // Launcher font
  instance().settings().setValue("launcherfont",
    myLauncherFontPopup->getSelectedTag().toString());

  // ROM launcher info viewer
  instance().settings().setValue("romviewer",
    myRomViewerPopup->getSelectedTag().toString());

  // Exit to Launcher
  instance().settings().setValue("exitlauncher",
    myLauncherExitPopup->getSelectedTag().toString());

  // Debugger size
  instance().settings().setValue("dbg.res",
    GUI::Size(myDebuggerWidthSlider->getValue(),
              myDebuggerHeightSlider->getValue()));

  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
    myDebuggerFontStyle->getSelectedTag().toString());

  // UI palette
  instance().settings().setValue("uipalette",
    myPalettePopup->getSelectedTag().toString());

  // Listwidget quick delay
  instance().settings().setValue("listdelay",
    myListDelayPopup->getSelectedTag().toString());
  ListWidget::setQuickSelectDelay(myListDelayPopup->getSelectedTag().toInt());

  // Mouse wheel lines
  instance().settings().setValue("mwheel",
    myWheelLinesPopup->getSelectedTag().toString());
  ScrollBarWidget::setWheelLines(myWheelLinesPopup->getSelectedTag().toInt());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Launcher options
    {
      uInt32 w = BSPF_min(instance().frameBuffer().desktopSize().w, 1000u);
      uInt32 h = BSPF_min(instance().frameBuffer().desktopSize().h, 600u);
      myLauncherWidthSlider->setValue(w);
      myLauncherWidthLabel->setValue(w);
      myLauncherHeightSlider->setValue(h);
      myLauncherHeightLabel->setValue(h);
      myLauncherFontPopup->setSelected("medium", "");
      myRomViewerPopup->setSelected("1", "");
      myLauncherExitPopup->setSelected("0", "");
      break;
    }

    case 1:  // Debugger options
    {
#ifdef DEBUGGER_SUPPORT
      uInt32 w = BSPF_min(instance().frameBuffer().desktopSize().w, (uInt32)DebuggerDialog::kMediumFontMinW);
      uInt32 h = BSPF_min(instance().frameBuffer().desktopSize().h, (uInt32)DebuggerDialog::kMediumFontMinH);
      myDebuggerWidthSlider->setValue(w);
      myDebuggerWidthLabel->setValue(w);
      myDebuggerHeightSlider->setValue(h);
      myDebuggerHeightLabel->setValue(h);
      myDebuggerFontStyle->setSelected("0");
#endif
      break;
    }

    case 2:  // Misc. options
      myPalettePopup->setSelected("standard");
      myListDelayPopup->setSelected("300");
      myWheelLinesPopup->setSelected("4");
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

#ifdef DEBUGGER_SUPPORT
    case kDWidthChanged:
      myDebuggerWidthLabel->setValue(myDebuggerWidthSlider->getValue());
      break;

    case kDHeightChanged:
      myDebuggerHeightLabel->setValue(myDebuggerHeightSlider->getValue());
      break;

    case kDSmallSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kSmallFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kSmallFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kSmallFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kSmallFontMinH);
      break;

    case kDMediumSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kMediumFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kMediumFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kMediumFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kMediumFontMinH);
      break;

    case kDLargeSize:
      myDebuggerWidthSlider->setValue(DebuggerDialog::kLargeFontMinW);
      myDebuggerWidthLabel->setValue(DebuggerDialog::kLargeFontMinW);
      myDebuggerHeightSlider->setValue(DebuggerDialog::kLargeFontMinH);
      myDebuggerHeightLabel->setValue(DebuggerDialog::kLargeFontMinH);
      break;
#endif

    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
