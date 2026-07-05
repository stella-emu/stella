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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "DialogContainer.hxx"
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
#include "Layout.hxx"
#include "Font.hxx"
#include "StellaMediumFont.hxx"
#include "LauncherDialog.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif
#include "UIDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UIDialog::UIDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font, GuiObject* boss)
  : Dialog(osystem, parent, font, "User interface settings"),
    CommandSender(boss),
    myIsGlobal{boss != nullptr}
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight();
  WidgetArray wid;
  VariantList items;
  const Common::Size& ds = instance().frameBuffer().desktopSize();

  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font, so it reflows on
  // font change.  The tab widget's bar geometry is (re)computed in layout() via
  // TabWidget::updateTabSizes(), so a placeholder size is fine here.
  myTab = new TabWidget(this, font, 0, 0, 1, 1);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Look & Feel options
  wid.clear();
  int tabID = myTab->addTab(" Look & Feel ");
  const int lwidth = font.getStringWidth("Controller repeat delay ");
  const int pwidth = font.getStringWidth("Right bottom");

  // UI Palette
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  VarList::push_back(items, "Dark", "dark");
  myPalette1Popup = new PopUpWidget(myTab, font, 0, 0, pwidth, lineHeight,
                                    items, "Light theme", lwidth);
  myPalette1Popup->setToolTip("Primary/light theme.", Event::ToggleUIPalette, EventMode::kMenuMode);
  wid.push_back(myPalette1Popup);

  myPalette2Popup = new PopUpWidget(myTab, font, 0, 0, pwidth, lineHeight,
                                    items, "Dark theme ", lwidth);
  myPalette2Popup->setToolTip("Alternative/dark theme.", Event::ToggleUIPalette, EventMode::kMenuMode);
  wid.push_back(myPalette2Popup);

  myAutoPalette = new CheckboxWidget(myTab, font, 0, 0, "Auto theme");
  myAutoPalette->setToolTip("Enable for automatic switching between light and dark themes in sync with OS.");
  wid.push_back(myAutoPalette);

  // Dialog font
  items.clear();
  VarList::push_back(items, "Small", "small");            //  8x13
  VarList::push_back(items, "Low Medium", "low_medium");  //  9x15
  VarList::push_back(items, "Medium", "medium");          //  9x18
  VarList::push_back(items, "Large (10pt)", "large");     // 10x20
  VarList::push_back(items, "Large (12pt)", "large12");   // 12x24
  VarList::push_back(items, "Large (14pt)", "large14");   // 14x28
  VarList::push_back(items, "Large (16pt)", "large16");   // 16x32
  myDialogFontPopup = new PopUpWidget(myTab, font, 0, 0, pwidth, lineHeight,
                                      items, "Dialogs font", lwidth, kDialogFont);
  wid.push_back(myDialogFontPopup);

  // Enable HiDPI mode
  myHidpiWidget = new CheckboxWidget(myTab, font, 0, 0, "HiDPI mode (*)");
  myHidpiWidget->setToolTip("Scale the UI by a factor of two when enabled.");
  wid.push_back(myHidpiWidget);

  // Dialog position
  items.clear();
  VarList::push_back(items, "Centered", 0);
  VarList::push_back(items, "Left top", 1);
  VarList::push_back(items, "Right top", 2);
  VarList::push_back(items, "Right bottom", 3);
  VarList::push_back(items, "Left bottom", 4);
  myPositionPopup = new PopUpWidget(myTab, font, 0, 0, pwidth, lineHeight,
                                    items, "Dialogs position", lwidth);
  wid.push_back(myPositionPopup);

  // Center window (in windowed mode)
  myCenter = new CheckboxWidget(myTab, _font, 0, 0, "Center windows");
  myCenter->setToolTip("Check to center all windows, else remember last position.");
  wid.push_back(myCenter);

  // Delay between quick-selecting characters in ListWidget
  const int swidth = myPalette1Popup->getWidth() - lwidth;
  myListDelaySlider = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                      "List input delay        ", 0, kListDelay,
                                      font.getStringWidth("1 second"));
  myListDelaySlider->setMinValue(0);
  myListDelaySlider->setMaxValue(1000);
  myListDelaySlider->setStepValue(50);
  myListDelaySlider->setTickmarkIntervals(5);
  myListDelaySlider->setToolTip("Set delay between key presses in file lists"
                                " before a search string resets.");
  wid.push_back(myListDelaySlider);

  // Number of lines a mouse wheel will scroll
  myWheelLinesSlider = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                      "Mouse wheel scroll      ", 0, kMouseWheel,
                                       font.getStringWidth("10 lines"));
  myWheelLinesSlider->setMinValue(1);
  myWheelLinesSlider->setMaxValue(10);
  myWheelLinesSlider->setTickmarkIntervals(3);
  wid.push_back(myWheelLinesSlider);

  // Mouse double click speed
  myDoubleClickSlider = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                         "Double-click speed      ", 0, 0,
                                         font.getStringWidth("900 ms"), " ms");
  myDoubleClickSlider->setMinValue(100);
  myDoubleClickSlider->setMaxValue(900);
  myDoubleClickSlider->setStepValue(50);
  myDoubleClickSlider->setTickmarkIntervals(8);
  wid.push_back(myDoubleClickSlider);

  // Initial delay before controller input will start repeating
  myControllerDelaySlider = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                             "Controller repeat delay ", 0, kControllerDelay,
                                             font.getStringWidth("1 second"));
  myControllerDelaySlider->setMinValue(200);
  myControllerDelaySlider->setMaxValue(1000);
  myControllerDelaySlider->setStepValue(100);
  myControllerDelaySlider->setTickmarkIntervals(4);
  wid.push_back(myControllerDelaySlider);

  // Controller repeat rate
  myControllerRateSlider = new SliderWidget(myTab, font, 0, 0, swidth, lineHeight,
                                            "Controller repeat rate  ", 0, 0,
                                            font.getStringWidth("30 repeats/s"), " repeats/s");
  myControllerRateSlider->setMinValue(2);
  myControllerRateSlider->setMaxValue(30);
  myControllerRateSlider->setStepValue(1);
  myControllerRateSlider->setTickmarkIntervals(14);
  wid.push_back(myControllerRateSlider);

  // Info message (positioned along the bottom of the tab in layout())
  myLookFeelInfo = new StaticTextWidget(myTab, ifont, 0, 0, 1, ifont.getFontHeight(),
                       "(*) Change requires an application restart");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("UserInterface");

  //////////////////////////////////////////////////////////
  // 2) Launcher options
  wid.clear();
  tabID = myTab->addTab(" Launcher ");
  const int lwidthL = font.getStringWidth("Launcher height ");

  // ROM path
  int bwidth = font.getStringWidth("ROM path" + ELLIPSIS) + 20 + 1;
  myRomButton = new ButtonWidget(myTab, font, 0, 0, bwidth,
      buttonHeight, "ROM path" + ELLIPSIS, kChooseRomDirCmd);
  wid.push_back(myRomButton);
  myRomPath = new EditTextWidget(myTab, font, 0, 0, lineHeight, lineHeight, "");
  wid.push_back(myRomPath);

  myFollowLauncherWidget = new CheckboxWidget(myTab, font, 0, 0, "Follow Launcher path");
  myFollowLauncherWidget->setToolTip("The ROM path is updated during Launcher navigation.");
  wid.push_back(myFollowLauncherWidget);

  // Launcher font
  const int pwidthL = font.getStringWidth("2x (1000x760)");
  items.clear();
  VarList::push_back(items, "Small", "small");            //  8x13
  VarList::push_back(items, "Low Medium", "low_medium");  //  9x15
  VarList::push_back(items, "Medium", "medium");          //  9x18
  VarList::push_back(items, "Large (10pt)", "large");     // 10x20
  VarList::push_back(items, "Large (12pt)", "large12");   // 12x24
  VarList::push_back(items, "Large (14pt)", "large14");   // 14x28
  VarList::push_back(items, "Large (16pt)", "large16");   // 16x32
  myLauncherFontPopup =
    new PopUpWidget(myTab, font, 0, 0, pwidthL, lineHeight, items,
      "Launcher font ", lwidthL);
  wid.push_back(myLauncherFontPopup);

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(myTab, font, 0, 0, "Launcher width ",
                                           lwidthL, 0, 6 * fontWidth, "px");
  myLauncherWidthSlider->setMaxValue(ds.w);
  myLauncherWidthSlider->setStepValue(10);
  wid.push_back(myLauncherWidthSlider);

  myLauncherHeightSlider = new SliderWidget(myTab, font, 0, 0, "Launcher height ",
                                            lwidthL, 0, 6 * fontWidth, "px");
  myLauncherHeightSlider->setMaxValue(ds.h);
  myLauncherHeightSlider->setStepValue(10);
  wid.push_back(myLauncherHeightSlider);

  // Track favorites
  myFavoritesWidget = new CheckboxWidget(myTab, _font, 0, 0, "Track favorites");
  myFavoritesWidget->setToolTip("Check to enable favorites tracking and display.");
  wid.push_back(myFavoritesWidget);

  // Display launcher extensions
  myLauncherExtensionsWidget = new CheckboxWidget(myTab, _font, 0, 0, "Display file extensions");
  wid.push_back(myLauncherExtensionsWidget);

  // Display bottom buttons
  myLauncherButtonsWidget = new CheckboxWidget(myTab, _font, 0, 0, "Display bottom buttons");
  myLauncherButtonsWidget->setToolTip("Check to enable bottom command buttons.");
  wid.push_back(myLauncherButtonsWidget);

  // ROM launcher info/snapshot viewer
  myRomViewerSize = new SliderWidget(myTab, font, 0, 0, "ROM info width  ",
                                     lwidthL, kRomViewer, 6 * fontWidth, "%  ");
  myRomViewerSize->setMinValue(0);
  myRomViewerSize->setMaxValue(100);
  myRomViewerSize->setStepValue(2);
  // set tickmarks every ~20%
  myRomViewerSize->setTickmarkIntervals((myRomViewerSize->getMaxValue() - myRomViewerSize->getMinValue()) / 20);
  wid.push_back(myRomViewerSize);

  // Snapshot path (load files)
  bwidth = font.getStringWidth("Image path" + ELLIPSIS) + fontWidth * 2 + 1;
  myOpenBrowserButton = new ButtonWidget(myTab, font, 0, 0, bwidth, buttonHeight,
                                         "Image path" + ELLIPSIS, kChooseSnapLoadDirCmd);
  myOpenBrowserButton->setToolTip("Select path for images used in Launcher.");
  wid.push_back(myOpenBrowserButton);

  mySnapLoadPath = new EditTextWidget(myTab, font, 0, 0, lineHeight, lineHeight, "");
  wid.push_back(mySnapLoadPath);

  // Exit to Launcher
  myLauncherExitWidget = new CheckboxWidget(myTab, font, 0, 0, "Always exit to Launcher");
  wid.push_back(myLauncherExitWidget);

  // Info message (positioned along the bottom of the tab in layout())
  myLauncherInfo = new StaticTextWidget(myTab, ifont, 0, 0, 1, ifont.getFontHeight(),
                       "(*) Changes may require an application restart");

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("ROMInfo");

  //////////////////////////////////////////////////////////
  // All ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    myRomButton->clearFlags(Widget::FLAG_ENABLED);
    myRomPath->setEditable(false);
  }

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  setHelpAnchor("UserInterface");

#ifndef WINDOWED_SUPPORT
  myCenter->clearFlags(Widget::FLAG_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::layout()
{
  const int lineHeight   = Dialog::lineHeight(),
            buttonHeight = Dialog::buttonHeight(),
            fontWidth    = Dialog::fontWidth(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Size the (fixed) dialog from the current font so it reflows on font change
  _w = 64 * fontWidth + HBORDER * 2;
  _h = _th + VGAP * 3 + lineHeight + 10 * (lineHeight + VGAP) + VGAP * 2
       + buttonHeight + VBORDER * 3;

  // Size the tab widget below the title bar, then recompute its tab-bar
  // geometry for the current font/width; its contents are positioned in child
  // (tab-relative) coordinates by the per-tab helpers below
  myTab->setPos(2, VGAP + _th);
  myTab->setWidth(_w - 2 * 2);
  myTab->setHeight(_h - _th - VGAP - buttonHeight - VBORDER * 2);
  myTab->updateTabSizes();

  layoutLookAndFeelTab();
  layoutLauncherTab();

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::layoutLookAndFeelTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap();

  // The left column is a uniform vertical run of self-labeling controls, so it
  // maps cleanly onto a vertical BoxLayout (each control keeps its own size).
  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  col->addSpace(1);
  col->addFixed(anchoredItem(myPalette1Popup), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myPalette2Popup), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myDialogFontPopup), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myPositionPopup), lineHeight);
  col->addSpace(VGAP * 3);
  col->addFixed(anchoredItem(myListDelaySlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myWheelLinesSlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myDoubleClickSlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myControllerDelaySlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myControllerRateSlider), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // The right-hand checkboxes align to specific left-column rows, so they are
  // positioned directly once those rows are resolved
  const int xpos2 = myPalette1Popup->getRight() + fontWidth * 5;
  myAutoPalette->setPos(xpos2,
      myPalette1Popup->getBottom() - CheckboxWidget::boxSize(_font) / 2 - 1);
  myHidpiWidget->setPos(xpos2, myDialogFontPopup->getTop() + 1);
  myCenter->setPos(xpos2, myPositionPopup->getTop() + 1);

  // Info message along the bottom of the tab
  myLookFeelInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - ifont.getFontHeight() - VGAP - VBORDER);
  myLookFeelInfo->setWidth(std::min(ifont.getStringWidth(myLookFeelInfo->getLabel()),
                                    _w - HBORDER * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::layoutLauncherTab()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::vCentered;
  using Dir = BoxLayout::Dir;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int lineHeight   = Dialog::lineHeight(),
            fontHeight   = Dialog::fontHeight(),
            fontWidth    = Dialog::fontWidth(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap(),
            INDENT       = Dialog::indent();
  const int lwidth = _font.getStringWidth("Launcher height ");

  // Left column: the two path rows (browse button + filling edit) bracket a
  // vertical run of self-labeling launcher and ROM-info-viewer controls.  The
  // browse buttons keep their (taller) natural height and overflow their row.
  auto col = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);

  auto romRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  romRow->addFixed(anchoredItem(myRomButton), myRomButton->getWidth());
  romRow->addSpace(fontWidth);
  romRow->addStretch(vCentered(myRomPath, myRomPath->getHeight()));
  col->addFixed(std::move(romRow), lineHeight);

  col->addSpace(VGAP * 2);
  col->addSpace(lineHeight + VGAP);  // row shared with the right "follow launcher" box
  col->addFixed(anchoredItem(myLauncherFontPopup), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myLauncherWidthSlider), lineHeight);
  col->addSpace(VGAP);
  col->addFixed(anchoredItem(myLauncherHeightSlider), lineHeight);
  col->addSpace(VGAP * 4);
  col->addFixed(anchoredItem(myRomViewerSize), lineHeight);
  col->addSpace(VGAP);

  auto imgRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  imgRow->addSpace(INDENT);
  imgRow->addFixed(anchoredItem(myOpenBrowserButton), myOpenBrowserButton->getWidth());
  imgRow->addSpace(lwidth - INDENT - myOpenBrowserButton->getWidth());
  imgRow->addStretch(vCentered(mySnapLoadPath, mySnapLoadPath->getHeight()));
  col->addFixed(std::move(imgRow), lineHeight);

  col->addSpace(VGAP * 4);
  col->addFixed(anchoredItem(myLauncherExitWidget), lineHeight);
  col->doLayout(0, 0, myTab->getWidth(), myTab->getHeight());

  // Right-hand checkbox column, right-aligned to the widest label and vertically
  // aligned to specific (now-resolved) left-column rows
  const int xpos2 = _w - HBORDER - _font.getStringWidth("Display file extensions")
                    - CheckboxWidget::prefixSize(_font) - 1;
  myFollowLauncherWidget->setPos(xpos2,
      myLauncherFontPopup->getTop() - lineHeight - VGAP);

  int rypos = myLauncherFontPopup->getTop();
  myFavoritesWidget->setPos(xpos2, rypos + 1);
  rypos += lineHeight + VGAP;
  myLauncherExtensionsWidget->setPos(xpos2, rypos + 1);
  rypos += lineHeight + VGAP;
  myLauncherButtonsWidget->setPos(xpos2, rypos + 1);

  // Info message along the bottom of the tab
  myLauncherInfo->setPos(HBORDER,
      myTab->getHeight() - fontHeight - ifont.getFontHeight() - VGAP - VBORDER);
  myLauncherInfo->setWidth(std::min(ifont.getStringWidth(myLauncherInfo->getLabel()),
                                    _w - HBORDER * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // ROM path
  myRomPath->setText(settings.getString("romdir"));

  // Launcher size
  const Common::Size& ls = settings.getSize("launcherres");
  const Common::Size& ds = instance().frameBuffer().desktopSize(BufferType::Launcher);
  uInt32 w = ls.w, h = ls.h;

  w = std::max(w, FBMinimum::Width);
  h = std::max(h, FBMinimum::Height);
  w = std::min(w, ds.w);
  h = std::min(h, ds.h);

  myLauncherWidthSlider->setValue(w);
  myLauncherHeightSlider->setValue(h);

  // Follow Launcher path
  myFollowLauncherWidget->setState(settings.getBool("followlauncher"));

  // Launcher font
  const string& launcherFont = settings.getString("launcherfont");
  myLauncherFontPopup->setSelected(launcherFont, "medium");

  myFavoritesWidget->setState(settings.getBool("favorites"));
  myLauncherExtensionsWidget->setState(settings.getBool("launcherextensions"));
  myLauncherButtonsWidget->setState(settings.getBool("launcherbuttons"));

  // ROM launcher info viewer
  const float zoom = instance().settings().getFloat("romviewer");
  const int percentage = zoom * TIAConstants::viewableWidth * 100 / w;
  myRomViewerSize->setValue(percentage);

  // ROM launcher info viewer image path
  mySnapLoadPath->setText(settings.getString("snaploaddir"));

  // Exit to launcher
  const bool exitlauncher = settings.getBool("exitlauncher");
  myLauncherExitWidget->setState(exitlauncher);

  // UI palette
  const string& pal1 = settings.getString("uipalette");
  myPalette1Popup->setSelected(pal1, "standard");
  const string& pal2 = settings.getString("uipalette2");
  myPalette2Popup->setSelected(pal2, "dark");
  myAutoPalette->setState(settings.getBool("autouipalette"));

  // Dialog font
  const string& dialogFont = settings.getString("dialogfont");
  myDialogFontPopup->setSelected(dialogFont, "medium");

  // Enable HiDPI mode
  if(!instance().frameBuffer().hidpiAllowed())
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

  // Center window
  myCenter->setState(settings.getBool("center"));

  // Listwidget quick delay
  const int delay = settings.getInt("listdelay");
  myListDelaySlider->setValue(delay);

  // Mouse wheel lines
  const int mw = settings.getInt("mwheel");
  myWheelLinesSlider->setValue(mw);

  // Mouse double click
  const int md = settings.getInt("mdouble");
  myDoubleClickSlider->setValue(md);

  // Controller input delay
  const int cs = settings.getInt("ctrldelay");
  myControllerDelaySlider->setValue(cs);

  // Controller input rate
  const int cr = settings.getInt("ctrlrate");
  myControllerRateSlider->setValue(cr);

  handleLauncherSize();
  handleRomViewer();

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // ROM path
  settings.setValue("romdir", myRomPath->getText());

  // Follow Launcher path
  settings.setValue("followlauncher", myFollowLauncherWidget->getState());

  // Launcher size
  settings.setValue("launcherres",
    Common::Size(myLauncherWidthSlider->getValue(),
              myLauncherHeightSlider->getValue()));

  // Launcher font
  settings.setValue("launcherfont",
                    myLauncherFontPopup->getSelectedTag().toString());

  // Track favorites
  settings.setValue("favorites", myFavoritesWidget->getState());
  // Display launcher extensions
  settings.setValue("launcherextensions", myLauncherExtensionsWidget->getState());
  // Display bottom buttons
  settings.setValue("launcherbuttons", myLauncherButtonsWidget->getState());

  // ROM launcher info viewer
  const int w = myLauncherWidthSlider->getValue();
  const float zoom = myRomViewerSize->getValue() * w / 100.F / TIAConstants::viewableWidth;
  settings.setValue("romviewer", zoom);

  // ROM launcher info viewer image path
  settings.setValue("snaploaddir", mySnapLoadPath->getText());

  // Exit to Launcher
  settings.setValue("exitlauncher", myLauncherExitWidget->getState());

  // UI palette
  settings.setValue("uipalette",
                    myPalette1Popup->getSelectedTag().toString());
  settings.setValue("uipalette2",
                    myPalette2Popup->getSelectedTag().toString());
  settings.setValue("autouipalette", myAutoPalette->getState());

  instance().frameBuffer().updateTheme();
  instance().frameBuffer().setUIPalette();
  instance().frameBuffer().update(FrameBuffer::UpdateMode::REDRAW);

  // Dialog font
  settings.setValue("dialogfont",
                    myDialogFontPopup->getSelectedTag().toString());

  // Enable HiDPI mode
  settings.setValue("hidpi", myHidpiWidget->getState());

  // Dialog position
  settings.setValue("dialogpos", myPositionPopup->getSelectedTag().toString());

  // Center window
  settings.setValue("center", myCenter->getState());

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
      myPalette1Popup->setSelected("standard");
      myPalette2Popup->setSelected("dark");
      myAutoPalette->setState(false);
      myDialogFontPopup->setSelected("medium", "");
      myHidpiWidget->setState(false);
      myPositionPopup->setSelected("0");
      myCenter->setState(false);
      myListDelaySlider->setValue(300);
      myWheelLinesSlider->setValue(4);
      myDoubleClickSlider->setValue(500);
      myControllerDelaySlider->setValue(400);
      myControllerRateSlider->setValue(20);
      break;
    case 1:  // Launcher options
    {
      const FSNode node("~");
      const Common::Size& size =
        instance().frameBuffer().desktopSize(BufferType::Launcher);

      myRomPath->setText(node.getShortPath());
      const uInt32 w = std::min<uInt32>(size.w, 900);
      const uInt32 h = std::min<uInt32>(size.h, 600);
      myLauncherWidthSlider->setValue(w);
      myLauncherHeightSlider->setValue(h);
      myLauncherFontPopup->setSelected("medium", "");
      myFavoritesWidget->setState(true);
      myLauncherExtensionsWidget->setState(false);
      myLauncherButtonsWidget->setState(false);
      myRomViewerSize->setValue(35);
      mySnapLoadPath->setText(instance().userDir().getShortPath());
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
    {
      const bool informPath = myIsGlobal &&
        (myRomPath->getText() != instance().settings().getString("romdir")
         || myRomPath->getText() != instance().settings().getString("startromdir"));
      const bool informFav = myIsGlobal &&
        myFavoritesWidget->getState() != instance().settings().getBool("favorites");
      const bool informExt = myIsGlobal &&
        myLauncherExtensionsWidget->getState() != instance().settings().getBool("launcherextensions");
      const bool informRomViewer = myIsGlobal &&
        (myRomViewerSize->getValue() > 0) != (instance().settings().getFloat("romviewer") > 0.F);
      const bool informFont = myIsGlobal &&
        myLauncherFontPopup->getSelectedTag().toString() != instance().settings().getString("launcherfont");
      // The dialog font applies to every open dialog (not just the launcher),
      // so it is not gated on myIsGlobal
      const bool informDialogFont =
        myDialogFontPopup->getSelectedTag().toString() != instance().settings().getString("dialogfont");
      saveConfig();
      close();
      if(informPath) // Let the boss know romdir has changed
        sendCommand(LauncherDialog::kRomDirChosenCmd, 0, 0);
      if(informFav) // Let the boss know the favaorites tracking has changed
        sendCommand(LauncherDialog::kFavChangedCmd, 0, 0);
      if(informExt) // Let the boss know the file extension display setting has changed
        sendCommand(LauncherDialog::kExtChangedCmd, 0, 0);
      if(informRomViewer) // Let the boss know the ROM info viewer was toggled
        sendCommand(LauncherDialog::kRomViewerChangedCmd, 0, 0);
      if(informFont) // Let the boss know the launcher font changed
        sendCommand(LauncherDialog::kFontChangedCmd, 0, 0);
      if(informDialogFont)
      {
        // Change the dialog font in place, then re-font every open dialog.  A
        // dialog that no longer fits the window is detected + reported when it
        // (re)opens via Dialog::open() (see Dialog::exceedsScreen)
        instance().frameBuffer().changeDialogFont(
            instance().settings().getString("dialogfont"));
        parent().refreshFont();
      }
      break;
    }
    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kDialogFont:
      handleLauncherSize();
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
      BrowserDialog::show(this, _font, "Select ROM Directory",
                          myRomPath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) myRomPath->setText(node.getShortPath());
                          });
      break;

    case kRomViewer:
      handleRomViewer();
      break;

    case kChooseSnapLoadDirCmd:
      BrowserDialog::show(this, _font, "Select ROM Info Viewer Image Directory",
                          mySnapLoadPath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) mySnapLoadPath->setText(node.getShortPath());
                          });
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleLauncherSize()
{
  // Determine minimal launcher sizebased on the default font
  //  So what fits with default font should fit for any font.
  const FontDesc& fd = FrameBuffer::getFontDesc(
      myDialogFontPopup->getSelectedTag().toString());
  const int w = std::max(FBMinimum::Width, FBMinimum::Width *
      fd.maxwidth / GUI::stellaMediumDesc.maxwidth);
  const int h = std::max(FBMinimum::Height, FBMinimum::Height *
      fd.height / GUI::stellaMediumDesc.height);
  const Common::Size& ds =
      instance().frameBuffer().desktopSize(BufferType::Launcher);

  myLauncherWidthSlider->setMinValue(w);
  if(myLauncherWidthSlider->getValue() < myLauncherWidthSlider->getMinValue())
    myLauncherWidthSlider->setValue(w);
  // one tickmark every ~100 pixel
  myLauncherWidthSlider->setTickmarkIntervals((ds.w - w + 67) / 100);

  myLauncherHeightSlider->setMinValue(h);
  if(myLauncherHeightSlider->getValue() < myLauncherHeightSlider->getMinValue())
    myLauncherHeightSlider->setValue(h);
  // one tickmark every ~100 pixel
  myLauncherHeightSlider->setTickmarkIntervals((ds.h - h + 67) / 100);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::handleRomViewer()
{
  const int size = myRomViewerSize->getValue();
  const bool enable = size > myRomViewerSize->getMinValue();

  if(enable)
  {
    myRomViewerSize->setValueLabel(size);
    myRomViewerSize->setValueUnit("%");
  }
  else
  {
    myRomViewerSize->setValueLabel("Off");
    myRomViewerSize->setValueUnit("");
  }
  myOpenBrowserButton->setEnabled(enable);
  mySnapLoadPath->setEnabled(enable);
}
