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
#include "FileListWidget.hxx"
#include "PopUpWidget.hxx"
#include "ScrollBarWidget.hxx"
#include "EditTextWidget.hxx"
#include "Settings.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "LauncherDialog.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif
#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font, GuiObject* boss, int max_w, int max_h)
  : Dialog(osystem, parent, font, "User interface settings"),
    CommandSender(boss),
    myFont(font),
    myIsGlobal(boss != nullptr)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonHeight = font.getLineHeight() + 4;

  const int VBORDER = 8;
  const int HBORDER = 10;
  const int INDENT = 16;
  const int V_GAP = 4;
  int xpos, ypos, tabID;
  int lwidth, pwidth, bwidth;
  WidgetArray wid;
  VariantList items;
  const Common::Size& ds = instance().frameBuffer().desktopSize();

  // Set real dimensions
  setSize(64 * fontWidth + HBORDER * 2, 11 * (lineHeight + V_GAP) + V_GAP * 9 + VBORDER + _th,
          max_w, max_h);

  // The tab widget
  myTab = new TabWidget(this, font, 2, 4 + _th, _w - 2*2, _h - _th - buttonHeight - 20);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Misc. options
  wid.clear();
  tabID = myTab->addTab(" Look & Feel ");
  lwidth = font.getStringWidth("Controller repeat delay ");
  pwidth = font.getStringWidth("Right bottom");
  xpos = HBORDER;  ypos = VBORDER;

  // UI Palette
  ypos += 1;
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  myPalettePopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                   items, "Theme      ", lwidth);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + V_GAP;

  // Dialog position
  items.clear();
  VarList::push_back(items, "Centered", 0);
  VarList::push_back(items, "Left top", 1);
  VarList::push_back(items, "Right top", 2);
  VarList::push_back(items, "Right bottom", 3);
  VarList::push_back(items, "Left bottom", 4);
  myPositionPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                    items, "Dialogs position ", lwidth);
  wid.push_back(myPositionPopup);
  ypos += lineHeight + V_GAP;

  // Enable HiDPI mode
  myHidpiWidget = new CheckboxWidget(myTab, font, xpos, ypos, "HiDPI mode (*)");
  wid.push_back(myHidpiWidget);
  ypos += lineHeight + V_GAP * 4;

  // Delay between quick-selecting characters in ListWidget
  int swidth = myPalettePopup->getWidth() - lwidth;
  myListDelaySlider = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                      "List input delay        ", 0, kListDelay,
                                      font.getStringWidth("1 second"));
  myListDelaySlider->setMinValue(0);
  myListDelaySlider->setMaxValue(1000);
  myListDelaySlider->setStepValue(50);
  myListDelaySlider->setTickmarkIntervals(5);
  wid.push_back(myListDelaySlider);
  ypos += lineHeight + V_GAP;

  // Number of lines a mouse wheel will scroll
  myWheelLinesSlider = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                      "Mouse wheel scroll      ", 0, kMouseWheel,
                                       font.getStringWidth("10 lines"));
  myWheelLinesSlider->setMinValue(1);
  myWheelLinesSlider->setMaxValue(10);
  myWheelLinesSlider->setTickmarkIntervals(3);
  wid.push_back(myWheelLinesSlider);
  ypos += lineHeight + V_GAP;

  // Mouse double click speed
  myDoubleClickSlider = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                         "Double-click speed      ", 0, 0,
                                         font.getStringWidth("900 ms"), " ms");
  myDoubleClickSlider->setMinValue(100);
  myDoubleClickSlider->setMaxValue(900);
  myDoubleClickSlider->setStepValue(50);
  myDoubleClickSlider->setTickmarkIntervals(8);
  wid.push_back(myDoubleClickSlider);
  ypos += lineHeight + V_GAP;

  // Initial delay before controller input will start repeating
  myControllerDelaySlider = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                             "Controller repeat delay ", 0, kControllerDelay,
                                             font.getStringWidth("1 second"));
  myControllerDelaySlider->setMinValue(200);
  myControllerDelaySlider->setMaxValue(1000);
  myControllerDelaySlider->setStepValue(100);
  myControllerDelaySlider->setTickmarkIntervals(4);
  wid.push_back(myControllerDelaySlider);
  ypos += lineHeight + V_GAP;

  // Controller repeat rate
  myControllerRateSlider = new SliderWidget(myTab, font, xpos, ypos, swidth, lineHeight,
                                            "Controller repeat rate  ", 0, 0,
                                            font.getStringWidth("30 repeats/s"), " repeats/s");
  myControllerRateSlider->setMinValue(2);
  myControllerRateSlider->setMaxValue(30);
  myControllerRateSlider->setStepValue(1);
  myControllerRateSlider->setTickmarkIntervals(14);
  wid.push_back(myControllerRateSlider);

  // Add message concerning usage
  ypos = myTab->getHeight() - 5 - fontHeight - ifont.getFontHeight() - 10;
  lwidth = ifont.getStringWidth("(*) Change requires application restart");
  new StaticTextWidget(myTab, ifont, xpos, ypos, std::min(lwidth, _w - 20), fontHeight,
    "(*) Change requires application restart");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////
  // 2) Launcher options
  wid.clear();
  tabID = myTab->addTab(" Launcher ");
  lwidth = font.getStringWidth("Launcher height ");
  xpos = HBORDER;  ypos = VBORDER;

  // ROM path
  bwidth = font.getStringWidth("ROM path" + ELLIPSIS) + 20 + 1;
  ButtonWidget* romButton =
    new ButtonWidget(myTab, font, xpos, ypos, bwidth, buttonHeight,
                     "ROM path" + ELLIPSIS, kChooseRomDirCmd);
  wid.push_back(romButton);
  xpos = romButton->getRight() + 8;
  myRomPath = new EditTextWidget(myTab, font, xpos, ypos + 1,
                                 _w - xpos - HBORDER - 2, lineHeight, "");
  wid.push_back(myRomPath);
  xpos = HBORDER;
  ypos += lineHeight + V_GAP * 4;

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(myTab, font, xpos, ypos, "Launcher width ",
                                           lwidth, kLauncherSize, 6 * fontWidth, "px");
  myLauncherWidthSlider->setMinValue(FBMinimum::Width);
  myLauncherWidthSlider->setMaxValue(ds.w);
  myLauncherWidthSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myLauncherWidthSlider->setTickmarkIntervals((ds.w - FBMinimum::Width + 50) / 100);
  wid.push_back(myLauncherWidthSlider);
  ypos += lineHeight + V_GAP;

  myLauncherHeightSlider = new SliderWidget(myTab, font, xpos, ypos, "Launcher height ",
                                            lwidth, kLauncherSize, 6 * fontWidth, "px");
  myLauncherHeightSlider->setMinValue(FBMinimum::Height);
  myLauncherHeightSlider->setMaxValue(ds.h);
  myLauncherHeightSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myLauncherHeightSlider->setTickmarkIntervals((ds.h - FBMinimum::Height + 50) / 100);
  wid.push_back(myLauncherHeightSlider);
  ypos += lineHeight + V_GAP;

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
  ypos += lineHeight + V_GAP * 4;

  // ROM launcher info/snapshot viewer
  items.clear();
  VarList::push_back(items, "Off", "0");
  VarList::push_back(items, "1x (640x480) ", "1");
  VarList::push_back(items, "2x (1000x760)", "2");
  myRomViewerPopup =
    new PopUpWidget(myTab, font, xpos, ypos + 1, pwidth, lineHeight, items,
                    "ROM info viewer ", lwidth, kRomViewer);
  wid.push_back(myRomViewerPopup);
  ypos += lineHeight + V_GAP;

  // Snapshot path (load files)
  xpos = HBORDER + INDENT;
  bwidth = font.getStringWidth("Image path" + ELLIPSIS) + 20 + 1;
  myOpenBrowserButton = new ButtonWidget(myTab, font, xpos, ypos, bwidth, buttonHeight,
                                         "Image path" + ELLIPSIS, kChooseSnapLoadDirCmd);
  wid.push_back(myOpenBrowserButton);

  mySnapLoadPath = new EditTextWidget(myTab, font, HBORDER + lwidth, ypos + 1,
                                      _w - lwidth - HBORDER * 2 - 2, lineHeight, "");
  wid.push_back(mySnapLoadPath);
  ypos += lineHeight + V_GAP * 4;

  // Exit to Launcher
  xpos = HBORDER;
  myLauncherExitWidget = new CheckboxWidget(myTab, font, xpos + 1, ypos, "Always exit to Launcher");
  wid.push_back(myLauncherExitWidget);

  // Add message concerning usage
  xpos = HBORDER;
  ypos = myTab->getHeight() - 5 - fontHeight - ifont.getFontHeight() - 10;
  lwidth = ifont.getStringWidth("(*) Changes require application restart");
  new StaticTextWidget(myTab, ifont, xpos, ypos, std::min(lwidth, _w - 20), fontHeight,
                       "(*) Changes require application restart");

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  // All ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    romButton->clearFlags(Widget::FLAG_ENABLED);
    myRomPath->setEditable(false);
  }

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
  const Settings& settings = instance().settings();

  // ROM path
  myRomPath->setText(settings.getString("romdir"));

  // Launcher size
  const Common::Size& ls = settings.getSize("launcherres");
  uInt32 w = ls.w, h = ls.h;

  w = std::max(w, FBMinimum::Width);
  h = std::max(h, FBMinimum::Height);
  w = std::min(w, instance().frameBuffer().desktopSize().w);
  h = std::min(h, instance().frameBuffer().desktopSize().h);

  myLauncherWidthSlider->setValue(w);
  myLauncherHeightSlider->setValue(h);

  // Launcher font
  const string& font = settings.getString("launcherfont");
  myLauncherFontPopup->setSelected(font, "medium");

  // ROM launcher info viewer
  const string& viewer = settings.getString("romviewer");
  myRomViewerPopup->setSelected(viewer, "0");

  // ROM launcher info viewer image path
  mySnapLoadPath->setText(settings.getString("snaploaddir"));

  // Exit to launcher
  bool exitlauncher = settings.getBool("exitlauncher");
  myLauncherExitWidget->setState(exitlauncher);

  // UI palette
  const string& pal = settings.getString("uipalette");
  myPalettePopup->setSelected(pal, "standard");

  // Enable HiDPI mode
  if (!instance().frameBuffer().hidpiAllowed())
  {
    myHidpiWidget->setState(false);
    myHidpiWidget->setEnabled(false);
  }
  else
  {
    myHidpiWidget->setState(settings.getBool("hidpi"));
  }

  // Dialog position
  myPositionPopup->setSelected(settings.getString("dialogpos"), "0");

  // Listwidget quick delay
  int delay = settings.getInt("listdelay");
  myListDelaySlider->setValue(delay);

  // Mouse wheel lines
  int mw = settings.getInt("mwheel");
  myWheelLinesSlider->setValue(mw);

  // Mouse double click
  int md = settings.getInt("mdouble");
  myDoubleClickSlider->setValue(md);

  // Controller input delay
  int cs = settings.getInt("ctrldelay");
  myControllerDelaySlider->setValue(cs);

  // Controller input rate
  int cr = settings.getInt("ctrlrate");
  myControllerRateSlider->setValue(cr);

  handleRomViewer();

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // ROM path
  settings.setValue("romdir", myRomPath->getText());

  // Launcher size
  settings.setValue("launcherres",
    Common::Size(myLauncherWidthSlider->getValue(),
              myLauncherHeightSlider->getValue()));

  // Launcher font
  settings.setValue("launcherfont",
    myLauncherFontPopup->getSelectedTag().toString());

  // ROM launcher info viewer
  settings.setValue("romviewer",
    myRomViewerPopup->getSelectedTag().toString());

  // ROM launcher info viewer image path
  settings.setValue("snaploaddir", mySnapLoadPath->getText());

  // Exit to Launcher
  settings.setValue("exitlauncher", myLauncherExitWidget->getState());

  // UI palette
  settings.setValue("uipalette",
    myPalettePopup->getSelectedTag().toString());
  instance().frameBuffer().setUIPalette();

  // Enable HiDPI mode
  settings.setValue("hidpi", myHidpiWidget->getState());

  // Dialog position
  settings.setValue("dialogpos", myPositionPopup->getSelectedTag().toString());

  // Listwidget quick delay
  settings.setValue("listdelay", myListDelaySlider->getValue());
  FileListWidget::setQuickSelectDelay(myListDelaySlider->getValue());

  // Mouse wheel lines
  settings.setValue("mwheel", myWheelLinesSlider->getValue());
  ScrollBarWidget::setWheelLines(myWheelLinesSlider->getValue());

  // Mouse double click
  settings.setValue("mdouble", myDoubleClickSlider->getValue());
  DialogContainer::setDoubleClickDelay(myDoubleClickSlider->getValue());

  // Controller input delay
  settings.setValue("ctrldelay", myControllerDelaySlider->getValue());
  DialogContainer::setControllerDelay(myControllerDelaySlider->getValue());

  // Controller input rate
  settings.setValue("ctrlrate", myControllerRateSlider->getValue());
  DialogContainer::setControllerRate(myControllerRateSlider->getValue());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Misc. options
      myPalettePopup->setSelected("standard");
      myHidpiWidget->setState(false);
      myPositionPopup->setSelected("0");
      myListDelaySlider->setValue(300);
      myWheelLinesSlider->setValue(4);
      myDoubleClickSlider->setValue(500);
      myControllerDelaySlider->setValue(400);
      myControllerRateSlider->setValue(20);
      break;
    case 1:  // Launcher options
    {
      FilesystemNode node("~");
      myRomPath->setText(node.getShortPath());
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
      if(myIsGlobal)  // Let the boss know romdir has changed
        sendCommand(LauncherDialog::kRomDirChosenCmd, 0, 0);
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kListDelay:
      if(myListDelaySlider->getValue() == 0)
      {
        myListDelaySlider->setValueLabel("Off");
        myListDelaySlider->setValueUnit("");
      }
      else if(myListDelaySlider->getValue() == 1000)
      {
        myListDelaySlider->setValueLabel("1");
        myListDelaySlider->setValueUnit(" second");
      }
      else
      {
        myListDelaySlider->setValueUnit(" ms");
      }
      break;

    case kMouseWheel:
      if(myWheelLinesSlider->getValue() == 1)
        myWheelLinesSlider->setValueUnit(" line");
      else
        myWheelLinesSlider->setValueUnit(" lines");
      break;

    case kControllerDelay:
      if(myControllerDelaySlider->getValue() == 1000)
      {
        myControllerDelaySlider->setValueLabel("1");
        myControllerDelaySlider->setValueUnit(" second");
      }
      else
      {
        myControllerDelaySlider->setValueUnit(" ms");
      }
      break;

    case kChooseRomDirCmd:
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      createBrowser("Select ROM directory");
      myBrowser->show(myRomPath->getText(),
                      BrowserDialog::Directories, LauncherDialog::kRomDirChosenCmd);
      break;

    case LauncherDialog::kRomDirChosenCmd:
      myRomPath->setText(myBrowser->getResult().getShortPath());
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
  int size = myRomViewerPopup->getSelected();
  bool enable = myRomViewerPopup->getSelectedName() != "Off";
  VariantList items;

  myOpenBrowserButton->setEnabled(enable);
  mySnapLoadPath->setEnabled(enable);

  items.clear();
  VarList::push_back(items, "Off", "0");
  VarList::push_back(items, "1x (640x480) ", "1");
  if(myLauncherWidthSlider->getValue() >= 1000 &&
     myLauncherHeightSlider->getValue() >= 760)
  {
    VarList::push_back(items, "2x (1000x760)", "2");
  }
  else if (size == 2)
  {
    myRomViewerPopup->setSelected(1);
  }

  myRomViewerPopup->addItems(items);
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
