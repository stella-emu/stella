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
#include "TabPaneWidget.hxx"
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
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;
  VariantList items;
  const Common::Size& ds = instance().frameBuffer().desktopSize();

  // Widgets are only created here (at placeholder geometry); layout() sizes the
  // dialog and positions everything from the current font, so it reflows on
  // font change.  The tab widget's bar geometry is (re)computed in layout() via
  // TabWidget::updateTabSizes(), so a placeholder size is fine here.
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, font, 0, 0, 1, 1);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) Look & Feel options
  wid.clear();
  int tabID = myTab->addTab(" Look & Feel ");
  auto* lookPane = new TabPaneWidget(myTab, font);
  myTab->setPaneWidget(tabID, lookPane);

  // UI Palette
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  VarList::push_back(items, "Dark", "dark");
  myPalette1Popup = new PopUpWidget(lookPane, font, 0, 0,
                                    items, "Light theme", 0);
  myPalette1Popup->setToolTip("Primary/light theme.", Event::ToggleUIPalette, EventMode::kMenuMode);
  wid.push_back(myPalette1Popup);

  myPalette2Popup = new PopUpWidget(lookPane, font, 0, 0,
                                    items, "Dark theme", 0);
  myPalette2Popup->setToolTip("Alternative/dark theme.", Event::ToggleUIPalette, EventMode::kMenuMode);
  wid.push_back(myPalette2Popup);

  myAutoPalette = new CheckboxWidget(lookPane, font, 0, 0, "Auto theme");
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
  myDialogFontPopup = new PopUpWidget(lookPane, font, 0, 0,
                                      items, "Dialogs font", 0, kDialogFont);
  wid.push_back(myDialogFontPopup);

  // Enable HiDPI mode
  myHidpiWidget = new CheckboxWidget(lookPane, font, 0, 0, "HiDPI mode (*)");
  myHidpiWidget->setToolTip("Scale the UI by a factor of two when enabled.");
  wid.push_back(myHidpiWidget);

  // Dialog position
  items.clear();
  VarList::push_back(items, "Centered", 0);
  VarList::push_back(items, "Left top", 1);
  VarList::push_back(items, "Right top", 2);
  VarList::push_back(items, "Right bottom", 3);
  VarList::push_back(items, "Left bottom", 4);
  myPositionPopup = new PopUpWidget(lookPane, font, 0, 0,
                                    items, "Dialogs position", 0);
  wid.push_back(myPositionPopup);

  // Center window (in windowed mode)
  myCenter = new CheckboxWidget(lookPane, _font, 0, 0, "Center windows");
  myCenter->setToolTip("Check to center all windows, else remember last position.");
  wid.push_back(myCenter);

  // Delay between quick-selecting characters in ListWidget.  The sliders' tracks
  // span the pop-ups' boxes and arrows beside them, but the pop-ups have not been
  // given their shared width yet, so layout() sets the real track width
  const int swidth = 1;
  myListDelaySlider = new SliderWidget(lookPane, font, 0, 0, swidth,
                                      "List input delay", 0, kListDelay,
                                      font.getStringWidth("1 second"));
  myListDelaySlider->setMinValue(0);
  myListDelaySlider->setMaxValue(1000);
  myListDelaySlider->setStepValue(50);
  myListDelaySlider->setTickmarkIntervals(5);
  myListDelaySlider->setToolTip("Set delay between key presses in file lists"
                                " before a search string resets.");
  wid.push_back(myListDelaySlider);

  // Number of lines a mouse wheel will scroll
  myWheelLinesSlider = new SliderWidget(lookPane, font, 0, 0, swidth,
                                      "Mouse wheel scroll", 0, kMouseWheel,
                                       font.getStringWidth("10 lines"));
  myWheelLinesSlider->setMinValue(1);
  myWheelLinesSlider->setMaxValue(10);
  myWheelLinesSlider->setTickmarkIntervals(3);
  wid.push_back(myWheelLinesSlider);

  // Mouse double click speed
  myDoubleClickSlider = new SliderWidget(lookPane, font, 0, 0, swidth,
                                         "Double-click speed", 0, 0,
                                         font.getStringWidth("900 ms"), " ms");
  myDoubleClickSlider->setMinValue(100);
  myDoubleClickSlider->setMaxValue(900);
  myDoubleClickSlider->setStepValue(50);
  myDoubleClickSlider->setTickmarkIntervals(8);
  wid.push_back(myDoubleClickSlider);

  // Initial delay before controller input will start repeating
  myControllerDelaySlider = new SliderWidget(lookPane, font, 0, 0, swidth,
                                             "Controller repeat delay", 0, kControllerDelay,
                                             font.getStringWidth("1 second"));
  myControllerDelaySlider->setMinValue(200);
  myControllerDelaySlider->setMaxValue(1000);
  myControllerDelaySlider->setStepValue(100);
  myControllerDelaySlider->setTickmarkIntervals(4);
  wid.push_back(myControllerDelaySlider);

  // Controller repeat rate
  myControllerRateSlider = new SliderWidget(lookPane, font, 0, 0, swidth,
                                            "Controller repeat rate", 0, 0,
                                            font.getStringWidth("30 repeats/s"), " repeats/s");
  myControllerRateSlider->setMinValue(2);
  myControllerRateSlider->setMaxValue(30);
  myControllerRateSlider->setStepValue(1);
  myControllerRateSlider->setTickmarkIntervals(14);
  wid.push_back(myControllerRateSlider);

  myLookFeelInfo = new StaticTextWidget(lookPane, ifont, 0, 0,
                       "(*) Change requires an application restart");

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  lookPane->setHelpAnchor("UserInterface");

  // Describe the layout once; the pane runs it on every resize.  Three of the
  // controls have a checkbox beside them, and those checkboxes share a column —
  // so the tab is a grid, and the rows without one simply span across it
  lookPane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::GridLayout;
    using GUI::anchoredItem;
    using GUI::alignedItem;
    using GUI::HAlign;
    using GUI::VAlign;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap();

    // The pop-ups and sliders draw their own labels, so they share one label
    // column and their value boxes and tracks line up down the tab
    GUI::alignLabels({{myPalette1Popup}, {myPalette2Popup}, {myDialogFontPopup},
                      {myPositionPopup}, {myListDelaySlider}, {myWheelLinesSlider},
                      {myDoubleClickSlider}, {myControllerDelaySlider},
                      {myControllerRateSlider}});
    // The pop-ups size their own boxes to their items; one shared width keeps
    // them flush, and the sliders' tracks then span box and arrow alike
    GUI::alignPopUps({myPalette1Popup, myPalette2Popup, myDialogFontPopup,
                      myPositionPopup});
    const int swidth = myPalette1Popup->getWidth() - myPalette1Popup->labelWidth();
    for(auto* slider: {myListDelaySlider, myWheelLinesSlider, myDoubleClickSlider,
                       myControllerDelaySlider, myControllerRateSlider})
      slider->setTrackWidth(swidth);

    enum Col: int { MAIN, EXTRA, COLS };
    enum Row: int {
      THEME1, THEME2, FONT, POSITION, GAP, LIST, WHEEL, CLICK, DELAY, RATE, ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS, fontWidth * 5, VGAP);
    grid->columnAuto(MAIN).columnStretch(EXTRA);
    for(int r = 0; r < ROWS; ++r)
      grid->rowAuto(r);
    grid->rowFixed(GAP, VGAP);

    grid->place(MAIN, THEME1,   anchoredItem(myPalette1Popup));
    grid->place(MAIN, THEME2,   anchoredItem(myPalette2Popup));
    grid->place(MAIN, FONT,     anchoredItem(myDialogFontPopup));
    grid->place(MAIN, POSITION, anchoredItem(myPositionPopup));

    // The auto-theme box governs both theme rows, so it is centered across them
    grid->place(EXTRA, THEME1,
                alignedItem(myAutoPalette, HAlign::Left, VAlign::Center), 1, 2);
    grid->place(EXTRA, FONT,     anchoredItem(myHidpiWidget));
    grid->place(EXTRA, POSITION, anchoredItem(myCenter));

    // The sliders have nothing beside them, so they take the whole row
    grid->place(MAIN, LIST,  anchoredItem(myListDelaySlider), COLS - MAIN);
    grid->place(MAIN, WHEEL, anchoredItem(myWheelLinesSlider), COLS - MAIN);
    grid->place(MAIN, CLICK, anchoredItem(myDoubleClickSlider), COLS - MAIN);
    grid->place(MAIN, DELAY, anchoredItem(myControllerDelaySlider), COLS - MAIN);
    grid->place(MAIN, RATE,  anchoredItem(myControllerRateSlider), COLS - MAIN);

    col.addAuto(std::move(grid));
    // Info message along the bottom of the tab
    col.addStretchSpace();
    col.addAuto(anchoredItem(myLookFeelInfo));
  });

  //////////////////////////////////////////////////////////
  // 2) Launcher options
  wid.clear();
  tabID = myTab->addTab(" Launcher ");
  auto* launchPane = new TabPaneWidget(myTab, font);
  myTab->setPaneWidget(tabID, launchPane);

  // ROM path
  myRomButton = new ButtonWidget(launchPane, font, 0, 0,
      "ROM path" + ELLIPSIS, kChooseRomDirCmd);
  wid.push_back(myRomButton);
  myRomPath = new EditTextWidget(launchPane, font, 0, 0, 1, "");
  wid.push_back(myRomPath);

  myFollowLauncherWidget = new CheckboxWidget(launchPane, font, 0, 0, "Follow Launcher path");
  myFollowLauncherWidget->setToolTip("The ROM path is updated during Launcher navigation.");
  wid.push_back(myFollowLauncherWidget);

  // Launcher font
  items.clear();
  VarList::push_back(items, "Small", "small");            //  8x13
  VarList::push_back(items, "Low Medium", "low_medium");  //  9x15
  VarList::push_back(items, "Medium", "medium");          //  9x18
  VarList::push_back(items, "Large (10pt)", "large");     // 10x20
  VarList::push_back(items, "Large (12pt)", "large12");   // 12x24
  VarList::push_back(items, "Large (14pt)", "large14");   // 14x28
  VarList::push_back(items, "Large (16pt)", "large16");   // 16x32
  myLauncherFontPopup =
    new PopUpWidget(launchPane, font, 0, 0, items, "Launcher font", 0);
  wid.push_back(myLauncherFontPopup);

  // Launcher width and height
  myLauncherWidthSlider = new SliderWidget(launchPane, font, 0, 0, "Launcher width",
                                           0, 0, 6 * fontWidth, "px");
  myLauncherWidthSlider->setMaxValue(ds.w);
  myLauncherWidthSlider->setStepValue(10);
  wid.push_back(myLauncherWidthSlider);

  myLauncherHeightSlider = new SliderWidget(launchPane, font, 0, 0, "Launcher height",
                                            0, 0, 6 * fontWidth, "px");
  myLauncherHeightSlider->setMaxValue(ds.h);
  myLauncherHeightSlider->setStepValue(10);
  wid.push_back(myLauncherHeightSlider);

  // Track favorites
  myFavoritesWidget = new CheckboxWidget(launchPane, _font, 0, 0, "Track favorites");
  myFavoritesWidget->setToolTip("Check to enable favorites tracking and display.");
  wid.push_back(myFavoritesWidget);

  // Display launcher extensions
  myLauncherExtensionsWidget = new CheckboxWidget(launchPane, _font, 0, 0, "Display file extensions");
  wid.push_back(myLauncherExtensionsWidget);

  // Display bottom buttons
  myLauncherButtonsWidget = new CheckboxWidget(launchPane, _font, 0, 0, "Display bottom buttons");
  myLauncherButtonsWidget->setToolTip("Check to enable bottom command buttons.");
  wid.push_back(myLauncherButtonsWidget);

  // ROM launcher info/snapshot viewer
  myRomViewerSize = new SliderWidget(launchPane, font, 0, 0, "ROM info width",
                                     0, kRomViewer, 6 * fontWidth, "%");
  myRomViewerSize->setMinValue(0);
  myRomViewerSize->setMaxValue(100);
  myRomViewerSize->setStepValue(2);
  // set tickmarks every ~20%
  myRomViewerSize->setTickmarkIntervals((myRomViewerSize->getMaxValue() - myRomViewerSize->getMinValue()) / 20);
  wid.push_back(myRomViewerSize);

  // Snapshot path (load files)
  myOpenBrowserButton = new ButtonWidget(launchPane, font, 0, 0,
                                         "Image path" + ELLIPSIS, kChooseSnapLoadDirCmd);
  myOpenBrowserButton->setToolTip("Select path for images used in Launcher.");
  wid.push_back(myOpenBrowserButton);

  mySnapLoadPath = new EditTextWidget(launchPane, font, 0, 0, 1, "");
  wid.push_back(mySnapLoadPath);

  // Exit to Launcher
  myLauncherExitWidget = new CheckboxWidget(launchPane, font, 0, 0, "Always exit to Launcher");
  wid.push_back(myLauncherExitWidget);

  myLauncherInfo = new StaticTextWidget(launchPane, ifont, 0, 0,
                       "(*) Changes may require an application restart");

  // Add items for tab 1
  addToFocusList(wid, myTab, tabID);

  launchPane->setHelpAnchor("ROMInfo");

  // Describe the layout once; the pane runs it on every resize.  The launcher
  // options each have a checkbox beside them, and those checkboxes share a
  // column flush with the tab's right edge — which is what the grid's stretching
  // left column produces, without anyone measuring the widest of them
  launchPane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::GridLayout;
    using GUI::anchoredItem;
    using GUI::alignedItem;
    using GUI::widgetItem;
    using GUI::HAlign;
    using GUI::VAlign;
    using Dir = BoxLayout::Dir;

    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();

    // The launcher's self-labeling controls share one label column
    GUI::alignLabels({{myLauncherFontPopup}, {myLauncherWidthSlider},
                      {myLauncherHeightSlider}, {myRomViewerSize}});
    const int labelW = myLauncherFontPopup->labelWidth();

    // A path row: its browse button, then the path filling the rest
    const auto pathRow = [&](ButtonWidget* button, EditTextWidget* path,
                             int indent, int gap) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      if(indent > 0)
        row->addSpace(indent);
      row->addAuto(anchoredItem(button));
      row->addSpace(gap);
      row->addStretch(alignedItem(path, HAlign::Fill, VAlign::Center,
                                  EditTextWidget::calcWidth(_font, 30)));
      return row;
    };

    enum Col: int { MAIN, EXTRA, COLS };
    enum Row: int {
      ROM_PATH, GAP1, FOLLOW, FONT, WIDTH, HEIGHT, GAP2, VIEWER, IMAGE_PATH,
      GAP3, EXIT, ROWS
    };
    auto grid = std::make_unique<GridLayout>(COLS, ROWS, fontWidth, VGAP);
    grid->columnStretch(MAIN).columnAuto(EXTRA);
    for(int r = 0; r < ROWS; ++r)
      grid->rowAuto(r);
    grid->rowFixed(GAP1, VGAP).rowFixed(GAP2, VGAP * 2).rowFixed(GAP3, VGAP * 2);

    // The ROM path spans the width; the "follow launcher" box that governs it
    // sits in the checkbox column on the row below
    grid->place(MAIN,  ROM_PATH, pathRow(myRomButton, myRomPath, 0, fontWidth),
                COLS - MAIN);
    grid->place(EXTRA, FOLLOW, anchoredItem(myFollowLauncherWidget));

    grid->place(MAIN,  FONT,   anchoredItem(myLauncherFontPopup));
    grid->place(EXTRA, FONT,   anchoredItem(myFavoritesWidget));
    grid->place(MAIN,  WIDTH,  anchoredItem(myLauncherWidthSlider));
    grid->place(EXTRA, WIDTH,  anchoredItem(myLauncherExtensionsWidget));
    grid->place(MAIN,  HEIGHT, anchoredItem(myLauncherHeightSlider));
    grid->place(EXTRA, HEIGHT, anchoredItem(myLauncherButtonsWidget));

    // The image path lines up with the value boxes of the controls above it
    grid->place(MAIN, VIEWER, anchoredItem(myRomViewerSize), COLS - MAIN);
    grid->place(MAIN, IMAGE_PATH,
                pathRow(myOpenBrowserButton, mySnapLoadPath, INDENT,
                        labelW - INDENT - myOpenBrowserButton->getWidth()),
                COLS - MAIN);
    grid->place(MAIN, EXIT, anchoredItem(myLauncherExitWidget), COLS - MAIN);

    col.addAuto(std::move(grid));
    // Info message along the bottom of the tab
    col.addStretchSpace();
    col.addAuto(anchoredItem(myLauncherInfo));
  });

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
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UIDialog::layout()
{
  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            VGAP         = Dialog::vGap();
  constexpr int xpos = 2;

  // Both dimensions come from what the tabs ask for: nothing here counts rows or
  // columns.  Each tab lays itself out via the tab widget, so there is no
  // per-tab code here either
  const Common::Size tabSize = myTab->naturalSize();

  myTab->setPos(xpos, VGAP + _th);
  myTab->setWidth(static_cast<int>(tabSize.w));
  myTab->setHeight(static_cast<int>(tabSize.h));

  _w = myTab->getWidth() + 2 * xpos;
  _h = _th + VGAP + myTab->getHeight() + VBORDER + buttonHeight + VBORDER;

  // Recompute the tab-bar geometry for the current font/width
  myTab->updateTabSizes();

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
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
