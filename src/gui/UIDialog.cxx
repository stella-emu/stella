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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "BrowserDialog.hxx"
#include "Dialog.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "ListWidget.hxx"
#include "PopUpWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "EditTextWidget.hxx"
#include "Settings.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif
#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font)
  : Dialog(osystem, parent, font, "User interface settings"),
    myFont(font)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Image path" + ELLIPSIS) + 20 + 1,
            buttonHeight = font.getLineHeight() + 4;

  const int VBORDER = 8;
  const int HBORDER = 10;
  const int INDENT = 16;
  int xpos, ypos, tabID;
  int lwidth, pwidth;
  WidgetArray wid;
  VariantList items;
  const GUI::Size& ds = instance().frameBuffer().desktopSize();

  // Set real dimensions
  _w = (39+15) * fontWidth + 10 * 2;
  _h = (10+1) * (lineHeight + 4) + VBORDER + _th;

  // The tab widget
  myTab = new TabWidget(this, font, 2, 4 + _th, _w - 2*2, _h - _th - buttonHeight - 20);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Misc. options
  wid.clear();
  tabID = myTab->addTab(" Look & Feel ");
  lwidth = font.getStringWidth("Mouse wheel scroll ");
  pwidth = font.getStringWidth("Standard");
  xpos = HBORDER;  ypos = VBORDER;

  // UI Palette
  ypos += 1;
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  myPalettePopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   items, "Theme     ", lwidth);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4 * 4;

  // Delay between quick-selecting characters in ListWidget
  int swidth = myPalettePopup->getWidth() - lwidth;
  myListDelayPopup = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                      "List input delay   ", 0, kListDelay,
                                      font.getStringWidth("1 second"));
  myListDelayPopup->setMinValue(0);
  myListDelayPopup->setMaxValue(1000);
  myListDelayPopup->setStepValue(50);
  myListDelayPopup->setTickmarkInterval(5);
  wid.push_back(myListDelayPopup);
  ypos += lineHeight + 4;

  // Number of lines a mouse wheel will scroll
  myWheelLinesPopup = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                      "Mouse wheel scroll ", 0, kMouseWheel,
                                       font.getStringWidth("10 lines"));
  myWheelLinesPopup->setMinValue(1);
  myWheelLinesPopup->setMaxValue(10);
  myWheelLinesPopup->setTickmarkInterval(3);
  wid.push_back(myWheelLinesPopup);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////
  // 2) Launcher options
  wid.clear();
  tabID = myTab->addTab(" Launcher ");
  lwidth = font.getStringWidth("Launcher height ");
  xpos = HBORDER;  ypos = VBORDER;

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(myTab, font, xpos, ypos, "Launcher width ",
                                           lwidth, kLauncherSize, 6 * fontWidth, "px");
  myLauncherWidthSlider->setMinValue(FrameBuffer::kFBMinW);
  myLauncherWidthSlider->setMaxValue(ds.w);
  myLauncherWidthSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myLauncherWidthSlider->setTickmarkInterval((ds.w - FrameBuffer::kFBMinW + 50) / 100);
  wid.push_back(myLauncherWidthSlider);
  ypos += lineHeight + 4;

  myLauncherHeightSlider = new SliderWidget(myTab, font, xpos, ypos, "Launcher height ",
                                            lwidth, kLauncherSize, 6 * fontWidth, "px");
  myLauncherHeightSlider->setMinValue(FrameBuffer::kFBMinH);
  myLauncherHeightSlider->setMaxValue(ds.h);
  myLauncherHeightSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myLauncherHeightSlider->setTickmarkInterval((ds.h - FrameBuffer::kFBMinH + 50) / 100);
  wid.push_back(myLauncherHeightSlider);
  ypos += lineHeight + 4;

  // Launcher font
  pwidth = font.getStringWidth("2x (1000x760)");
  items.clear();
  VarList::push_back(items, "Small", "small");
  VarList::push_back(items, "Medium", "medium");
  VarList::push_back(items, "Large", "large");
  myLauncherFontPopup =
    new PopUpWidget(myTab, font, xpos, ypos + 1, pwidth, lineHeight, items,
                    "Launcher font ", lwidth);
  wid.push_back(myLauncherFontPopup);
  ypos += lineHeight +  4 * 4;

  // ROM launcher info/snapshot viewer
  items.clear();
  VarList::push_back(items, "Off", "0");
  VarList::push_back(items, "1x (640x480) ", "1");
  VarList::push_back(items, "2x (1000x760)", "2");
  myRomViewerPopup =
    new PopUpWidget(myTab, font, xpos, ypos + 1, pwidth, lineHeight, items,
                    "ROM info viewer ", lwidth, kRomViewer);
  wid.push_back(myRomViewerPopup);
  ypos += lineHeight + 4;

  // Snapshot path (load files)
  xpos = HBORDER + INDENT;
  myOpenBrowserButton = new ButtonWidget(myTab, font, xpos, ypos, buttonWidth, buttonHeight,
                                         "Image path" + ELLIPSIS, kChooseSnapLoadDirCmd);
  wid.push_back(myOpenBrowserButton);
  //ypos += lineHeight + 4;
  xpos = myOpenBrowserButton->getRight() + 8;

  mySnapLoadPath = new EditTextWidget(myTab, font, xpos, ypos + 1,
                                      _w - xpos - HBORDER, lineHeight, "");
  wid.push_back(mySnapLoadPath);
  ypos += lineHeight + 4 * 5;

  // Exit to Launcher
  xpos = HBORDER;
  myLauncherExitWidget = new CheckboxWidget(myTab, font, xpos + 1, ypos, "Always exit to Launcher");
  wid.push_back(myLauncherExitWidget);

  // Add message concerning usage
  xpos = HBORDER;
  ypos = myTab->getHeight() - 5 - fontHeight - ifont.getFontHeight() - 10;
  lwidth = ifont.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(myTab, ifont, xpos, ypos, std::min(lwidth, _w - 20), fontHeight,
                       "(*) Changes require application restart",
                       TextAlign::Left);

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
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

  w = std::max(w, uInt32(FrameBuffer::kFBMinW));
  h = std::max(h, uInt32(FrameBuffer::kFBMinH));
  w = std::min(w, instance().frameBuffer().desktopSize().w);
  h = std::min(h, instance().frameBuffer().desktopSize().h);

  myLauncherWidthSlider->setValue(w);
  myLauncherHeightSlider->setValue(h);

  // Launcher font
  const string& font = instance().settings().getString("launcherfont");
  myLauncherFontPopup->setSelected(font, "medium");

  // ROM launcher info viewer
  const string& viewer = instance().settings().getString("romviewer");
  myRomViewerPopup->setSelected(viewer, "0");

  // ROM launcher info viewer image path
  mySnapLoadPath->setText(instance().settings().getString("snaploaddir"));

  // Exit to launcher
  bool exitlauncher = instance().settings().getBool("exitlauncher");
  myLauncherExitWidget->setState(exitlauncher);

  // UI palette
  const string& pal = instance().settings().getString("uipalette");
  myPalettePopup->setSelected(pal, "standard");

  // Listwidget quick delay
  int delay = instance().settings().getInt("listdelay");
  myListDelayPopup->setValue(delay);

  // Mouse wheel lines
  int mw = instance().settings().getInt("mwheel");
  myWheelLinesPopup->setValue(mw);

  handleRomViewer();

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

  // ROM launcher info viewer image path
  instance().settings().setValue("snaploaddir", mySnapLoadPath->getText());

  // Exit to Launcher
  instance().settings().setValue("exitlauncher", myLauncherExitWidget->getState());

  // UI palette
  instance().settings().setValue("uipalette",
    myPalettePopup->getSelectedTag().toString());
  instance().frameBuffer().setUIPalette();

  // Listwidget quick delay
  instance().settings().setValue("listdelay", myListDelayPopup->getValue());
  ListWidget::setQuickSelectDelay(myListDelayPopup->getValue());

  // Mouse wheel lines
  instance().settings().setValue("mwheel", myWheelLinesPopup->getValue());
  ScrollBarWidget::setWheelLines(myWheelLinesPopup->getValue());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Misc. options
      myPalettePopup->setSelected("standard");
      myListDelayPopup->setValue(300);
      myWheelLinesPopup->setValue(4);
      break;
    case 1:  // Launcher options
    {
      uInt32 w = std::min(instance().frameBuffer().desktopSize().w, 900u);
      uInt32 h = std::min(instance().frameBuffer().desktopSize().h, 600u);
      myLauncherWidthSlider->setValue(w);
      myLauncherHeightSlider->setValue(h);
      myLauncherFontPopup->setSelected("medium", "");
      myRomViewerPopup->setSelected("1", "");
      mySnapLoadPath->setText(instance().defaultLoadDir());
      myLauncherExitWidget->setState(false);
      break;
    }
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kListDelay:
      if(myListDelayPopup->getValue() == 0)
      {
        myListDelayPopup->setValueLabel("Off");
        myListDelayPopup->setValueUnit("");
      }
      else if(myListDelayPopup->getValue() == 1000)
      {
        myListDelayPopup->setValueLabel("1");
        myListDelayPopup->setValueUnit(" second");
      }
      else
      {
        myListDelayPopup->setValueUnit(" ms");
      }
      break;
    case kMouseWheel:
      if(myWheelLinesPopup->getValue() == 1)
        myWheelLinesPopup->setValueUnit(" line");
      else
        myWheelLinesPopup->setValueUnit(" lines");
      break;

    case kLauncherSize:
    case kRomViewer:
      handleRomViewer();
      break;

    case kChooseSnapLoadDirCmd:
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      createBrowser("Select snapshot load directory");
      myBrowser->show(mySnapLoadPath->getText(),
                      BrowserDialog::Directories, kSnapLoadDirChosenCmd);
      break;

    case kSnapLoadDirChosenCmd:
      mySnapLoadPath->setText(myBrowser->getResult().getShortPath());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleRomViewer()
{
  //string size = myRomViewerPopup->getSelectedName();
  bool enable = myRomViewerPopup->getSelectedName() != "Off";
  VariantList items;

  myOpenBrowserButton->setEnabled(enable);
  mySnapLoadPath->setEnabled(enable);

  /*items.clear();
  VarList::push_back(items, "Off", "0");
  VarList::push_back(items, "1x (640x480) ", "1");
  if(myLauncherWidthSlider->getValue() >= 1000 &&
     myLauncherHeightSlider->getValue() >= 760)
  {
    VarList::push_back(items, "2x (1000x760)", "2");
  }
  myRomViewerPopup->addItems(items);

  //myRomViewerPopup->setSelected("1x(640x480) ");*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::createBrowser(const string& title)
{
  uInt32 w = 0, h = 0;
  getDynamicBounds(w, h);

  // Create file browser dialog
  if(!myBrowser || uInt32(myBrowser->getWidth()) != w ||
     uInt32(myBrowser->getHeight()) != h)
    myBrowser = make_unique<BrowserDialog>(this, myFont, w, h, title);
  else
    myBrowser->setTitle(title);
}
