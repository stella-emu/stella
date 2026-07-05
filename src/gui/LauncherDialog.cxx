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
#include "Bankswitch.hxx"
#include "BrowserDialog.hxx"
#include "ContextMenu.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "EditTextWidget.hxx"
#include "FileListWidget.hxx"
#include "LauncherFileListWidget.hxx"
#include "NavigationWidget.hxx"
#include "FSNode.hxx"
#include "OptionsDialog.hxx"
#include "GameInfoDialog.hxx"
#include "HighScoresDialog.hxx"
#include "HighScoresManager.hxx"
#include "GlobalPropsDialog.hxx"
#include "StellaSettingsDialog.hxx"
#include "WhatsNewDialog.hxx"
#include "ProgressDialog.hxx"
#include "MessageBox.hxx"
#include "ToolTip.hxx"
#include "TimerManager.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "EventHandler.hxx"
#include "StellaKeys.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "RomImageWidget.hxx"
#include "RomInfoWidget.hxx"
#include "TIAConstants.hxx"
#include "Settings.hxx"
#include "Font.hxx"
#include "Layout.hxx"
#include "StellaFont.hxx"
#include "ConsoleBFont.hxx"
#include "ConsoleMediumBFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "Stella12x24tFont.hxx"
#include "Stella14x28tFont.hxx"
#include "Stella16x32tFont.hxx"
#include "Icons.hxx"
#include "Version.hxx"
#include "MediaFactory.hxx"
#include "LauncherDialog.hxx"
#include "Random.hxx"

namespace {

// A thin draggable vertical divider.  While dragged it reports its new
// (dialog-relative) x position via the given command, so the owning dialog
// can resize the areas on either side of it.
class DividerWidget : public Widget, public CommandSender
{
  public:
    DividerWidget(GuiObject* boss, const GUI::Font& font,
                  int x, int y, int w, int h, int cmd)
      : Widget(boss, font, x, y, w, h),
        CommandSender(boss),
        myCmd{cmd}
    {
      // FLAG_TRACK_MOUSE is required for handleMouseMoved() to be delivered
      // (Dialog only forwards moves to widgets that request mouse tracking)
      _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE;
    }

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override
    {
      if(b == MouseButton::LEFT)
        myDragging = true;
    }
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override
    {
      myDragging = false;
    }
    void handleMouseMoved(int x, int y) override
    {
      if(myDragging)
        sendCommand(myCmd, _x + x, 0);  // dialog-relative cursor x
    }

  protected:
    void drawWidget(bool hilite) override
    {
      FBSurface& s = dialog().surface();
      const int x = _x + _w / 2;
      const ColorId color = (isHighlighted() || myDragging)
                            ? kWidColorHi : kShadowColor;

      s.vLine(x - 1, _y, _y + _h - 1, color);
      s.vLine(x + 1, _y, _y + _h - 1, color);
    }

  private:
    bool myDragging{false};
    int  myCmd{0};

  private:
    DividerWidget() = delete;
    DividerWidget(const DividerWidget&) = delete;
    DividerWidget(DividerWidget&&) = delete;
    DividerWidget& operator=(const DividerWidget&) = delete;
    DividerWidget& operator=(DividerWidget&&) = delete;
};

}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherDialog::LauncherDialog(OSystem& osystem, DialogContainer& parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, osystem.frameBuffer().launcherFont(), "",
           x, y, w, h),
    CommandSender(this)
{
  // Create the widgets (in focus order); layout() assigns all geometry
  addPathWidgets();
  addFilteringWidgets();

  mySelectedItem = addRomWidgets() - 2;  // Highlight 'Rom Listing'
  if(instance().settings().getBool("launcherbuttons"))
    addButtonWidgets();
  myNavigationBar->setList(myList);

  tooltip().setFont(_font);

  applyFiltering();
  setHelpAnchor("ROMInfo");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::addFilteringWidgets()
{
  if(_w < 640)
    return;

  const int fontWidth = Dialog::fontWidth();
  const int HBORDER   = Dialog::hBorder();
  const int LBL_GAP   = fontWidth;
  const int btnGap    = fontWidth / 4;
  const bool smallIcon = Dialog::lineHeight() < 26;
  const int iconGap = ((fontWidth + 1) & ~0b1) + 1;  // round up to next even

  // Decide (from the initial width) whether the "Filter" label fits and whether
  // the item counter must be shortened; layout() assigns all actual geometry
  const int iconButtonWidth =
    (smallIcon ? GUI::icon_reload_small : GUI::icon_reload_large).width() + iconGap;
  const int randomButtonWidth =
    (smallIcon ? GUI::icon_random_small : GUI::icon_random_large).width() + iconGap;
  const int bwSettings =
    iconButtonWidth + _font.getStringWidth("Options" + ELLIPSIS) + btnGap * 2 + 1;
  int lwFilter = _font.getStringWidth("Filter");
  int lwFound  = _font.getStringWidth("12345 items found");
  const int fwFilter = EditTextWidget::calcWidth(_font, "123456");  // at least 6 chars
  int wTotal = HBORDER * 2 + iconButtonWidth * 2 + randomButtonWidth + lwFilter
    + fwFilter + lwFound + bwSettings + LBL_GAP * 5 + btnGap * 3;
  if(_w < wTotal)
  {
    wTotal -= lwFound;
    myShortCount = true;
    lwFound = _font.getStringWidth("12345 items");
    wTotal += lwFound;
  }
  if(_w < wTotal)
    lwFilter = 0;

  WidgetArray wid;
  myReloadButton = new ButtonWidget(this, _font, 0, 0, 1, 1,
                                    GUI::icon_reload_small, kReloadCmd);
  myReloadButton->setToolTip("Reload listing (Ctrl+R)");
  wid.push_back(myReloadButton);

  if(lwFilter)
    myFilterLabel = new StaticTextWidget(this, _font, 0, 0, 1, Dialog::fontHeight(),
                                         "Filter");

  myPattern = new EditTextWidget(this, _font, 0, 0, 1, Dialog::lineHeight(), "");
  myPattern->setToolTip("Enter filter text to reduce file list.\n"
    "Use '*' and '?' as wildcards.");
  wid.push_back(myPattern);

  mySubDirsButton = new ButtonWidget(this, _font, 0, 0, 1, 1,
                                     GUI::icon_reload_small, kSubDirsCmd);
  mySubDirsButton->setToolTip("Toggle subdirectories (Ctrl+D)");
  wid.push_back(mySubDirsButton);

  myRomCount = new StaticTextWidget(this, _font, 0, 0, 1, Dialog::fontHeight(),
                                    "", TextAlign::Right);

  myRandomRomButton = new ButtonWidget(this, _font, 0, 0, 1, 1,
                                       GUI::icon_random_small, kLoadRndRomCmd);
#ifndef BSPF_MACOS
  myRandomRomButton->setToolTip("Load random ROM (Alt+R)");
#else
  myRandomRomButton->setToolTip("Load random ROM (Cmd+R)");
#endif
  wid.push_back(myRandomRomButton);

  mySettingsButton = new ButtonWidget(this, _font, 0, 0, 1, 1,
    GUI::icon_settings_small, iconGap, "Options" + ELLIPSIS, kOptionsCmd);
  mySettingsButton->setToolTip("Open Options dialog (Ctrl+O)");
  wid.push_back(mySettingsButton);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::addPathWidgets()
{
  WidgetArray wid;
  // The navigation bar; its geometry (and that of its children) is assigned by
  // layout()
  myNavigationBar = new NavigationWidget(this, _font, 0, 0, _w, Dialog::buttonHeight());

  // Help icon (variant/size re-picked in layout())
  myHelpButton = new ButtonWidget(this, _font, 0, 0, 1, 1,
                                  GUI::icon_help_small, kHelpCmd);
  myHelpButton->setToolTip(std::format("Click for help. ({})",
    instance().eventHandler().getMappingDesc(Event::UIHelp, EventMode::kMenuMode)));
  myHelpButton->setEnabled(true);
  wid.push_back(myHelpButton);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int LauncherDialog::addRomWidgets()
{
  const bool bottomButtons = instance().settings().getBool("launcherbuttons");
  const int fontWidth = Dialog::fontWidth();
  const int HBORDER   = Dialog::hBorder();
  const int VGAP      = Dialog::vGap();
  WidgetArray wid;

  // Estimate the list/column height the way layout() sizes the main row, so the
  // ROM info font (chosen once, here) is sized for the right area
  const int fixedRows = bottomButtons ? 3 : 2;
  const int listHeight = (_h - 2 * Dialog::vBorder())
    - fixedRows * Dialog::buttonHeight() - fixedRows * (VGAP * 2);

  // The ROM info viewer can be toggled on/off at runtime; we always create its
  // widgets and just show/hide them in layout(), so toggling needs no rebuild
  myShowRomInfo = instance().settings().getFloat("romviewer") > 0.F;

  // Determine the ROM info column width: the persisted drag width if present,
  // otherwise the configured zoom level (a default zoom when currently disabled,
  // so the widgets have a sensible size ready to be shown)
  const float savedFraction = instance().settings().getFloat("romwidth");
  int imageWidth = 0;
  if(savedFraction > 0.F)
    imageWidth = clampRomInfoWidth(
      static_cast<int>(std::round(savedFraction * (_w - HBORDER * 2))), listHeight);
  else
  {
    const float zoom = myShowRomInfo
      ? instance().settings().getFloat("romviewer") : 1.F;
    imageWidth = static_cast<int>(getRomInfoZoom(listHeight, zoom)
                                  * TIAConstants::viewableWidth);
  }
  // Remember the ROM info width as a fraction of the content width, so it scales
  // proportionally when the window is resized (see layout())
  myRomInfoFraction = imageWidth > 0
    ? static_cast<float>(imageWidth) / (_w - HBORDER * 2) : 0.F;

  // remember initial ROM directory for returning there via home button
  instance().settings().setValue("startromdir", getRomDir());
  myList = new LauncherFileListWidget(this, _font, 0, 0, 1, 1);
  myList->setEditable(false);
  myList->setListMode(FSNode::ListMode::All);
  // since we cannot know how many files there are, use a really high value here
  myList->progress().setRange(0, 50000, 5);
  myList->progress().setMessage("        Filtering files" + ELLIPSIS + "        ");
  wid.push_back(myList);

  // Create the ROM info area (if enabled); layout() shows/hides and positions it
  if(imageWidth > 0)
  {
    // Choose the ROM info font for the (estimated) available area.  The image
    // is (roughly) the column width squared, plus a label
    int imageHeight = imageWidth + RomImageWidget::labelHeight(_font);
    const Common::Size fontArea(imageWidth - fontWidth * 2,
                                listHeight - imageHeight - VGAP * 4);
    setRomInfoFont(fontArea);

    // Now we have the correct font height
    imageHeight = imageWidth + RomImageWidget::labelHeight(*myROMInfoFont);
    myRomImageWidget = new RomImageWidget(this, *myROMInfoFont, 0, 0,
                                          imageWidth, imageHeight);
    wid.push_back(myRomImageWidget);
    myRomInfoWidget = new RomInfoWidget(this, *myROMInfoFont, 0, 0, imageWidth, 1);
    // Draggable divider between the list and the ROM info column
    myDivider = new DividerWidget(this, _font, 0, 0, fontWidth, 1, kRomWidthCmd);
  }
  return addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::addButtonWidgets()
{
  WidgetArray wid;
  // Four equal-width buttons at the bottom; geometry assigned by layout()
#ifndef BSPF_MACOS
  myStartButton   = new ButtonWidget(this, _font, 0, 0, 1, 1, "Select", kLoadROMCmd);
  wid.push_back(myStartButton);
  myGoUpButton    = new ButtonWidget(this, _font, 0, 0, 1, 1, "Go Up", ListWidget::kParentDirCmd);
  wid.push_back(myGoUpButton);
  myOptionsButton = new ButtonWidget(this, _font, 0, 0, 1, 1, "Options" + ELLIPSIS, kOptionsCmd);
  wid.push_back(myOptionsButton);
  myQuitButton    = new ButtonWidget(this, _font, 0, 0, 1, 1, "Quit", kQuitCmd);
  wid.push_back(myQuitButton);
#else
  myQuitButton    = new ButtonWidget(this, _font, 0, 0, 1, 1, "Quit", kQuitCmd);
  wid.push_back(myQuitButton);
  myOptionsButton = new ButtonWidget(this, _font, 0, 0, 1, 1, "Options" + ELLIPSIS, kOptionsCmd);
  wid.push_back(myOptionsButton);
  myGoUpButton    = new ButtonWidget(this, _font, 0, 0, 1, 1, "Go Up", ListWidget::kParentDirCmd);
  wid.push_back(myGoUpButton);
  myStartButton   = new ButtonWidget(this, _font, 0, 0, 1, 1, "Select", kLoadROMCmd);
  wid.push_back(myStartButton);
#endif
  myStartButton->setToolTip("Start emulation of selected ROM\nor switch to selected directory.");
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size LauncherDialog::contentMinSize() const
{
  // The smallest (logical) size at which no widget would have to clip: the
  // point where the most constrained widget reaches its minimum usable size.
  // Since each widget shrinks 1:1 with the dialog, this is independent of the
  // current size.  (The ROM info column couples width and height non-linearly,
  // so this is derived from the current widget sizes rather than purely from
  // the font metrics.)
  const int fontWidth = Dialog::fontWidth();
  int minW = _w, minH = _h;

  if(myList)
  {
    int giveW = myList->getWidth() - MIN_LAUNCHER_CHARS * fontWidth;
    if(myPattern)
      giveW = std::min(giveW, myPattern->getWidth()
                              - EditTextWidget::calcWidth(_font, "123456"));
    if(myNavigationBar)
      giveW = std::min(giveW,
                       myNavigationBar->getWidth() - MIN_LAUNCHER_CHARS * fontWidth);
    minW = _w - std::max(giveW, 0);

    int giveH = myList->getHeight() - _font.getLineHeight() * 3;
    if(myShowRomInfo && myRomInfoWidget)
      giveH = std::min(giveH,
                       myRomInfoWidget->getHeight() - _font.getFontHeight() * 2);
    minH = _h - std::max(giveH, 0);
  }
  return Common::Size(static_cast<uInt32>(minW), static_cast<uInt32>(minH));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int LauncherDialog::clampRomInfoWidth(int imageWidth, int colHeight) const
{
  const int fontWidth  = Dialog::fontWidth();
  const int fontHeight = Dialog::fontHeight();
  const int HBORDER    = Dialog::hBorder();
  const int contentW   = _w - HBORDER * 2;

  // Horizontal limit: keep at least MIN_LAUNCHER_CHARS for the list
  const int hMax = contentW - fontWidth - MIN_LAUNCHER_CHARS * fontWidth;
  // Vertical limit: the (roughly square) image, its label and a couple of
  // info text lines must all fit in the column height.  The launcher font is
  // used as a conservative estimate (the ROM info font is never larger).
  const int vMax = colHeight - fontHeight * 4;

  const int minW = MIN_ROMINFO_CHARS * fontWidth;
  const int maxW = std::max(std::min(hMax, vMax), minW);

  return BSPF::clamp(imageWidth, minW, maxW);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::layout()
{
  // Derive the available (logical) size from the current window, clamped to the
  // minimum computed at the end of the previous layout (the widgets have no
  // usable geometry until this method runs, so the minimum can't be read up
  // front).  Most WMs honour SDL's minimum-size hint; this clamp is a fallback.
  const uInt32 scale = instance().frameBuffer().hidpiScaleFactor();
  const Common::Rect& image = instance().frameBuffer().imageRect();
  const int w = std::max(static_cast<int>(image.w() / scale),
                         static_cast<int>(myMinSize.w));
  const int h = std::max(static_cast<int>(image.h() / scale),
                         static_cast<int>(myMinSize.h));

  // Persist the launcher window size on an actual change, so it is restored
  // next time (on restart and when returning from a game)
  if(w != _w || h != _h)
    instance().settings().setValue("launcherres",
        Common::Size(static_cast<uInt32>(w), static_cast<uInt32>(h)));
  _w = w;
  _h = h;

  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::vCentered;
  using Dir = BoxLayout::Dir;

  const int HBORDER      = Dialog::hBorder();
  const int vBorder      = Dialog::vBorder();
  const int fontWidth    = Dialog::fontWidth();
  const int fontHeight   = Dialog::fontHeight();
  const int lineHeight   = Dialog::lineHeight();
  const int buttonHeight = Dialog::buttonHeight();
  const int rowGap       = Dialog::vGap() * 2;
  const int LBL_GAP      = fontWidth;
  const int BTN_GAP      = fontWidth / 4;
  const bool bottomButtons = instance().settings().getBool("launcherbuttons");

  // Icon-button sizes and variants are font-dependent, so (re)compute them here
  // rather than reading each widget's intrinsic width; this keeps a runtime
  // font change correct, not just window resizes.  The icons are also re-picked
  // in case the font height crossed the small/large threshold.
  const bool smallIcon = lineHeight < 26;
  const int iconGap = ((fontWidth + 1) & ~0b1) + 1;  // round up to next even
  const GUI::Icon& reloadIcon   = smallIcon ? GUI::icon_reload_small   : GUI::icon_reload_large;
  const GUI::Icon& randomIcon   = smallIcon ? GUI::icon_random_small   : GUI::icon_random_large;
  const GUI::Icon& settingsIcon = smallIcon ? GUI::icon_settings_small : GUI::icon_settings_large;
  const GUI::Icon& helpIcon     = smallIcon ? GUI::icon_help_small     : GUI::icon_help_large;
  const int iconButtonWidth   = reloadIcon.width() + iconGap;
  const int randomButtonWidth = randomIcon.width() + iconGap;
  const int helpButtonWidth   = helpIcon.width() + iconGap;
  const int bwSettings = iconButtonWidth
    + _font.getStringWidth("Options" + ELLIPSIS) + BTN_GAP * 2 + 1;

  myReloadButton->setIcon(reloadIcon);
  myRandomRomButton->setIcon(randomIcon);
  mySettingsButton->setIcon(settingsIcon);
  if(myHelpButton)
    myHelpButton->setIcon(helpIcon);
  // The subdirs button shows the current on/off state in the matching variant
  const bool subdirs = instance().settings().getBool("launchersubdirs");
  mySubDirsButton->setIcon(subdirs
    ? (smallIcon ? GUI::icon_subdirs_small_on  : GUI::icon_subdirs_large_on)
    : (smallIcon ? GUI::icon_subdirs_small_off : GUI::icon_subdirs_large_off));

  // Path / navigation row: the bar fills the width, the help button anchors
  // to the right
  auto pathRow = std::make_unique<BoxLayout>(Dir::Horizontal, BTN_GAP, HBORDER, 0);
  pathRow->addStretch(widgetItem(myNavigationBar, MIN_LAUNCHER_CHARS * fontWidth));
  if(myHelpButton)
    pathRow->addFixed(widgetItem(myHelpButton), helpButtonWidth);

  // Filtering row: the filter field absorbs the slack; everything else is
  // fixed and packs around it
  const int lwFound = _font.getStringWidth(myShortCount ? "12345 items" : "12345 items found");
  auto filterRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, HBORDER, 0);
  filterRow->addFixed(widgetItem(myReloadButton), iconButtonWidth);
  filterRow->addSpace(LBL_GAP * 2);
  if(myFilterLabel)
  {
    filterRow->addFixed(vCentered(myFilterLabel, fontHeight),
                        _font.getStringWidth(myFilterLabel->getLabel()));
    filterRow->addSpace(LBL_GAP);
  }
  filterRow->addStretch(widgetItem(myPattern, EditTextWidget::calcWidth(_font, "123456")));
  filterRow->addSpace(BTN_GAP);
  filterRow->addFixed(widgetItem(mySubDirsButton), iconButtonWidth);
  filterRow->addSpace(BTN_GAP + LBL_GAP);
  filterRow->addFixed(vCentered(myRomCount, fontHeight), lwFound);
  filterRow->addSpace(LBL_GAP * 2);
  filterRow->addFixed(widgetItem(myRandomRomButton), randomButtonWidth);
  filterRow->addSpace(BTN_GAP);
  filterRow->addFixed(widgetItem(mySettingsButton), bwSettings);

  // Main row: the ROM list, plus an optional ROM info column (divider + image
  // over info text).  The column width is a fraction of the content width so
  // it scales with the window, clamped to keep both list and image usable.
  const int contentW = _w - HBORDER * 2;
  const int fixedRowCount = bottomButtons ? 3 : 2;
  const int gapCount      = bottomButtons ? 3 : 2;
  const int mainH = (_h - vBorder * 2) - fixedRowCount * buttonHeight
                  - gapCount * rowGap;

  const bool showRom = myShowRomInfo && myRomImageWidget && myRomInfoWidget;
  const int imageWidth = showRom
    ? clampRomInfoWidth(static_cast<int>(std::round(myRomInfoFraction * contentW)),
                        mainH)
    : 0;

  auto mainRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, HBORDER, 0);
  mainRow->addStretch(widgetItem(myList, MIN_LAUNCHER_CHARS * fontWidth, lineHeight * 3));
  if(showRom && imageWidth > 0)
  {
    mainRow->addFixed(widgetItem(myDivider), fontWidth);

    const int imageHeight = imageWidth + RomImageWidget::labelHeight(*myROMInfoFont);
    auto romCol = std::make_unique<BoxLayout>(Dir::Vertical);
    romCol->addFixed(widgetItem(myRomImageWidget), imageHeight);
    romCol->addSpace(myROMInfoFont->getFontHeight() / 2);
    romCol->addStretch(widgetItem(myRomInfoWidget, MIN_ROMINFO_CHARS * fontWidth));
    mainRow->addFixed(std::move(romCol), imageWidth);

    myRomImageWidget->clearFlags(Widget::FLAG_INVISIBLE);
    myRomInfoWidget->clearFlags(Widget::FLAG_INVISIBLE);
    myDivider->clearFlags(Widget::FLAG_INVISIBLE);
  }
  else if(myRomImageWidget && myRomInfoWidget)
  {
    // Viewer disabled: hide its widgets and move them off-screen so they
    // receive no mouse events (findWidgetInChain() is bounds-only and does not
    // skip invisible widgets); the full-width list then handles clicks there
    myRomImageWidget->setPos(_w, 0);
    myRomImageWidget->setFlags(Widget::FLAG_INVISIBLE);
    myRomInfoWidget->setPos(_w, 0);
    myRomInfoWidget->setFlags(Widget::FLAG_INVISIBLE);
    if(myDivider)
    {
      myDivider->setPos(_w, 0);
      myDivider->setFlags(Widget::FLAG_INVISIBLE);
    }
  }

  // Bottom button row (optional): four equal-width buttons
  unique_ptr<BoxLayout> buttonRow;
  if(bottomButtons && myStartButton && myGoUpButton && myOptionsButton && myQuitButton)
  {
    buttonRow = std::make_unique<BoxLayout>(Dir::Horizontal, Dialog::buttonGap(),
                                            HBORDER, 0);
#ifndef BSPF_MACOS
    buttonRow->addStretch(widgetItem(myStartButton));
    buttonRow->addStretch(widgetItem(myGoUpButton));
    buttonRow->addStretch(widgetItem(myOptionsButton));
    buttonRow->addStretch(widgetItem(myQuitButton));
#else
    buttonRow->addStretch(widgetItem(myQuitButton));
    buttonRow->addStretch(widgetItem(myOptionsButton));
    buttonRow->addStretch(widgetItem(myGoUpButton));
    buttonRow->addStretch(widgetItem(myStartButton));
#endif
  }

  // Assemble the vertical stack and apply it to the whole dialog
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, 0, vBorder);
  root->addFixed(std::move(pathRow), buttonHeight);
  root->addSpace(rowGap);
  root->addFixed(std::move(filterRow), buttonHeight);
  root->addSpace(rowGap);
  root->addStretch(std::move(mainRow));
  if(buttonRow)
  {
    root->addSpace(rowGap);
    root->addFixed(std::move(buttonRow), buttonHeight);
  }

  root->doLayout(0, 0, _w, _h);

  // With the widgets laid out, compute the minimum content size (from their
  // sizes) to clamp the next layout, and give the window manager its hint
  myMinSize = contentMinSize();
  instance().frameBuffer().setWindowMinSize(myMinSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& LauncherDialog::selectedRom() const
{
  return currentNode().getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& LauncherDialog::selectedRomMD5()
{
  if(currentNode().isDirectory() || !Bankswitch::isValidRomName(currentNode()))
    return EmptyString();

  // Attempt to conserve memory
  if(myMD5List.size() > 500)
    myMD5List.clear();

  // Lookup MD5, and if not present, cache it
  const auto [it, _] = myMD5List.try_emplace(
    currentNode().getPath(), "");
  if(it->second.empty())
    it->second = OSystem::getROMMD5(currentNode());
  return it->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FSNode& LauncherDialog::currentNode() const
{
  return myList->selected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FSNode& LauncherDialog::currentDir() const
{
  return myList->currentDir();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::reload()
{
  const bool extensions = instance().settings().getBool("launcherextensions");

  myMD5List.clear();
  myList->setShowFileExtensions(extensions);
  myList->reload();
  myPendingReload = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleQuit()
{
  // saveConfig() will be done in quit()
  close();
  instance().eventHandler().quit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::quit()
{
  saveConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::tick()
{
  if(myPendingReload && myReloadTime < TimerManager::getTicks() / 1000)
    reload();

  if(myPendingRomInfo && myRomInfoTime < TimerManager::getTicks() / 1000)
    loadPendingRomInfo();

  Dialog::tick();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadConfig()
{
  // Should we use a temporary directory specified on the commandline, or the
  // default one specified by the settings?
  Settings& settings = instance().settings();
  const string& romdir = getRomDir();
  const string& version = settings.getString("stella.version");

  // Show "What's New" message when a new version of Stella is run for the first time
  if(version < STELLA_VERSION)
  {
    openWhatsNew();
    settings.setValue("stella.version", STELLA_VERSION);
  }

  toggleSubDirs(false);
  myList->setShowFileExtensions(settings.getBool("launcherextensions"));
  // Favorites
  myList->loadFavorites();

  // Assume that if the list is empty, this is the first time that loadConfig()
  // has been called (and we should reload the list)
  if(myList->getList().empty())
  {
    FSNode node(romdir.empty() ? "~" : romdir);
    if(!myList->isDirectory(node))
      node = FSNode("~");

    myList->setInitialDirectory(node, settings.getString("lastrom"));
    updateUI();
  }
  Dialog::setFocus(getFocusList()[mySelectedItem]);

  if(myRomImageWidget && myRomInfoWidget)
  {
    myRomImageWidget->reloadProperties(currentNode());
    myRomInfoWidget->reloadProperties(currentNode());
  }

  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // always reset this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::saveConfig()
{
  Settings& settings = instance().settings();

  if(settings.getBool("followlauncher"))
    settings.setValue("romdir", myList->currentDir().getShortPath());

  // Favorites
  myList->saveFavorites();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::updateUI()
{
  // Only enable the 'up' button if there's a parent directory
  if(myGoUpButton)
    myGoUpButton->setEnabled(myList->currentDir().hasParent());
  // Only enable the navigation buttons if function is available
  myNavigationBar->updateUI();

  // Indicate how many files were found
  myRomCount->setLabel(std::format("{} {}",
    myList->getList().size() - (currentDir().hasParent() ? 1 : 0),
    myShortCount ? "items" : "items found"));

  loadRomInfo();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string LauncherDialog::getRomDir()
{
  const Settings& settings = instance().settings();
  const string& tmpromdir = settings.getString("tmpromdir");

  return !tmpromdir.empty() ? tmpromdir : settings.getString("romdir");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherDialog::matchWithWildcardsIgnoreCase(
    string_view str, string_view pattern)
{
  string in{str};
  string pat{pattern};

  BSPF::toUpperCase(in);
  BSPF::toUpperCase(pat);

  return BSPF::matchWithWildcards(in, pat);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::applyFiltering()
{
  myList->setNameFilter(
    [&](const FSNode& node) {
      myList->incProgress();
      if(!node.isDirectory())
      {
        // Only show valid ROMs
        string ext;
        if(!Bankswitch::isValidRomName(node, ext) ||
           BSPF::compareIgnoreCase(ext, "zip") == 0) // exclude ZIPs without any valid ROMs
          return false;

        // Skip over files that don't match the pattern in the 'pattern' textbox
        if(myPattern && !myPattern->getText().empty() &&
           !matchWithWildcardsIgnoreCase(node.getName(), myPattern->getText()))
          return false;
      }
      return true;
    }
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float LauncherDialog::getRomInfoZoom(int listHeight, float zoom) const
{
  // The ROM info area is some multiple of the minimum TIA image size
  if(zoom > 0.F)
  {
    const GUI::Font& smallFont = instance().frameBuffer().smallFont();
    const int fontWidth = Dialog::fontWidth(),
              HBORDER   = Dialog::hBorder();

    // upper zoom limit - at least 24 launchers chars/line and 7 + 4 ROM info lines
    if((_w - (HBORDER * 2 + fontWidth + 30) - zoom * TIAConstants::viewableWidth)
       / fontWidth < MIN_LAUNCHER_CHARS)
    {
      zoom = static_cast<float>(_w - (HBORDER * 2 + fontWidth + 30) - MIN_LAUNCHER_CHARS * fontWidth)
        / TIAConstants::viewableWidth;
    }
    if((listHeight - 12 - zoom * TIAConstants::viewableHeight) <
       MIN_ROMINFO_ROWS * smallFont.getLineHeight() +
       MIN_ROMINFO_LINES * smallFont.getFontHeight())
    {
      zoom = static_cast<float>(listHeight - 12 -
                   MIN_ROMINFO_ROWS * smallFont.getLineHeight() -
                   MIN_ROMINFO_LINES * smallFont.getFontHeight())
        / TIAConstants::viewableHeight;
    }

    // lower zoom limit - at least 30 ROM info chars/line
    if((zoom * TIAConstants::viewableWidth)
       / smallFont.getMaxCharWidth() < MIN_ROMINFO_CHARS + 6)
    {
      zoom = static_cast<float>(MIN_ROMINFO_CHARS * smallFont.getMaxCharWidth() + 6)
        / TIAConstants::viewableWidth;
    }
  }
  return zoom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setRomInfoFont(const Common::Size& area)
{
  // TODO: Perhaps offer a setting to override the font used?

  const FontDesc FONTS[7] = {
    GUI::stella16x32tDesc, GUI::stella14x28tDesc, GUI::stella12x24tDesc,
    GUI::stellaLargeDesc, GUI::stellaMediumDesc,
    GUI::consoleMediumBDesc, GUI::consoleBDesc
  };

  // Try to pick a font that works best, based on the available area
  for(const auto& font: FONTS)
  {
    // only use fonts <= launcher fonts
    if(Dialog::fontHeight() >= font.height)
    {
      if(std::cmp_greater_equal(area.h,
            MIN_ROMINFO_ROWS * font.height + 2 + MIN_ROMINFO_LINES * font.height)
         && std::cmp_greater_equal(area.w, MIN_ROMINFO_CHARS * font.maxwidth))
      {
        myROMInfoFont = std::make_unique<GUI::Font>(font);
        return;
      }
    }
  }
  myROMInfoFont = std::make_unique<GUI::Font>(GUI::stellaDesc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::setRomInfoEnabled(bool enable)
{
  if(enable == myShowRomInfo)
    return;

  myShowRomInfo = enable;

  // Re-flow so the list and ROM info column take their new widths, and the
  // viewer widgets are shown/hidden
  layout();

  if(enable)
    loadRomInfo();                  // load image/info for the current selection
  else if(myRomImageWidget)
    myRomImageWidget->clearProperties();  // stop rendering the image surface
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRomInfo()
{
  if(!myShowRomInfo || !myRomImageWidget || !myRomInfoWidget)
    return;

  // Update ROM info UI item, delayed
  myRomInfoTime = TimerManager::getTicks() / 1000 + myRomImageWidget->pendingLoadTime();
  myPendingRomInfo = true;

  const string& md5 = selectedRomMD5();
  Properties properties;
  if(!md5.empty())
  {
    // Make sure to load a per-ROM properties entry, if one exists
    instance().propSet().loadPerROM(currentNode(), md5);

    // And now get the properties for this ROM
    instance().propSet().getMD5(md5, properties);
  }
  else
  {
    const Bankswitch::Type type = Bankswitch::typeFromExtension(currentNode());
    if(type == Bankswitch::Type::AUTO)
    {
      myRomImageWidget->clearProperties();
      myRomInfoWidget->clearProperties();
      return;
    }
    properties.set(PropType::Cart_Name, currentNode().getBaseName());
    properties.set(PropType::Cart_Type, Bankswitch::typeToName(type));
  }
  myRomImageWidget->setProperties(currentNode(), properties, false);
  myRomInfoWidget->setProperties(currentNode(), properties, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadPendingRomInfo()
{
  myPendingRomInfo = false;

  if(!myRomImageWidget || !myRomInfoWidget)
    return;

  const string& md5 = selectedRomMD5();
  Properties properties;
  if(!md5.empty())
  {
    // Make sure to load a per-ROM properties entry, if one exists
    instance().propSet().loadPerROM(currentNode(), md5);

    // And now get the properties for this ROM
    instance().propSet().getMD5(md5, properties);
  }
  else
  {
    const Bankswitch::Type type = Bankswitch::typeFromExtension(currentNode());
    if(type == Bankswitch::Type::AUTO)
      return;
    properties.set(PropType::Cart_Name, currentNode().getBaseName());
    properties.set(PropType::Cart_Type, Bankswitch::typeToName(type));
  }
  myRomImageWidget->setProperties(currentNode(), properties);
  myRomInfoWidget->setProperties(currentNode(), properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleFavoritesChanged()
{
  if(instance().settings().getBool("favorites"))
  {
    myList->loadFavorites();
  }
  else
  {
    if(myList->inVirtualDir())
      myList->selectParent();
    myList->saveFavorites(true);
    myList->clearFavorites();
  }
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleContextMenu()
{
  const string& cmd = contextMenu().getSelectedTag().toString();

  if(cmd == "favorite")
    myList->toggleUserFavorite();
  else if(cmd == "remove")
  {
    myList->removeFavorite();
    reload();
  }
  else if(cmd == "removefavorites")
    removeAllFavorites();
  else if(cmd == "removepopular")
    removeAllPopular();
  else if(cmd == "removerecent")
    removeAllRecent();
  else if(cmd == "properties")
    openGameProperties();
  else if(cmd == "override")
    openGlobalProps();
  else if(cmd == "extensions")
    toggleExtensions();
  else if(cmd == "sorting")
    toggleSorting();
  else if(cmd == "subdirs")
    sendCommand(kSubDirsCmd, 0, 0);
  else if(cmd == "homedir")
    sendCommand(FileListWidget::kHomeDirCmd, 0, 0);
  else if(cmd == "highscores")
    openHighScores();
  else if(cmd == "reload")
    reload();
  else if(cmd == "options")
    openSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ContextMenu& LauncherDialog::contextMenu()
{
  if(myContextMenu == nullptr)
    // Create (empty) context menu for ROM list options
    myContextMenu = std::make_unique<ContextMenu>(this, _font);

  return *myContextMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Grab the key before passing it to the actual dialog and check for
  // context menu keys
  bool handled = false;

  if(!(myPattern && myPattern->isHighlighted()
      && instance().eventHandler().checkEventForKey(EventMode::kEditMode, key, mod)))
  {
    if(StellaModTest::isControl(mod))
    {
      handled = true;
      switch(key)
      {
        case StellaKey::D:
          sendCommand(kSubDirsCmd, 0, 0);
          break;

        case StellaKey::E:
          toggleExtensions();
          break;

        case StellaKey::F:
          myList->toggleUserFavorite();
          break;

        case StellaKey::G:
          openGameProperties();
          break;

        case StellaKey::H:
          if(instance().highScores().enabled())
            openHighScores();
          break;

        case StellaKey::O:
          openSettings();
          break;

        case StellaKey::P:
          openGlobalProps();
          break;

        case StellaKey::R:
          reload();
          break;

        case StellaKey::S:
          toggleSorting();
          break;

        case StellaKey::X:
          myList->removeFavorite();
          reload();
          break;

        default:
          handled = false;
          break;
      }
    }
    else if(StellaModTest::isAlt(mod) && key == StellaKey::R)
    {
      loadRandomRom();
      handled = true;
    }
  }
  if(!handled)
    // Required because BrowserDialog does not want raw input
    if(repeated || !myList->handleKeyDown(key, mod))
      Dialog::handleKeyDown(key, mod, repeated);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyDown(int stick, int button, bool longPress)
{
  myEventHandled = false;
  myList->setFlags(Widget::FLAG_WANTS_RAWDATA);   // allow handling long button press
  Dialog::handleJoyDown(stick, button, longPress);
  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // revert flag afterwards!
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleJoyUp(int stick, int button)
{
  // open power-up options and settings for 2nd and 4th button if not mapped otherwise
  const Event::Type e = instance().eventHandler().eventForJoyButton(EventMode::kMenuMode, stick, button);

  if(myList->isHighlighted() && button == 1 && (e == Event::UIOK || e == Event::NoType))
    openGlobalProps();
  if(myList->isHighlighted() && button == 3 && (e == Event::UITabPrev || e == Event::NoType))
    openSettings();
  else if(!myEventHandled)
    Dialog::handleJoyUp(stick, button);

  myList->clearFlags(Widget::FLAG_WANTS_RAWDATA); // stop allowing to handle long button press
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type LauncherDialog::getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button)
{
  Event::Type event = instance().eventHandler().eventForJoyAxis(EventMode::kMenuMode, stick, axis, adir, button);

  // map axis events for launcher
  switch(event)
  {
    case Event::UITabPrev:
      event = Event::UIPgUp;
      break;

    case Event::UITabNext:
      event = Event::UIPgDown;
      break;

    default:
      break;
  }
  return event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  // Grab right mouse button for context menu, send left to base class
  if(b == MouseButton::RIGHT
    && x + getAbsX() >= myList->getLeft() && x + getAbsX() <= myList->getRight()
    && y + getAbsY() >= myList->getTop() && y + getAbsY() <= myList->getBottom())
  {
    openContextMenu(x, y);
  }
  else
    Dialog::handleMouseUp(x, y, b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch(cmd)
  {
    case kRomWidthCmd:
    {
      // The divider was dragged: 'data' is the dialog-relative cursor x.
      // The ROM info column spans from there to the right border.
      const int HBORDER   = Dialog::hBorder();
      const int contentW  = _w - HBORDER * 2;
      const int imageWidth = clampRomInfoWidth((_w - HBORDER) - data,
                                               myList->getHeight());

      myRomInfoFraction = static_cast<float>(imageWidth) / contentW;
      instance().settings().setValue("romwidth", myRomInfoFraction);

      layout();
      break;
    }

    case kSubDirsCmd:
      toggleSubDirs();
      break;

    case kLoadROMCmd:
      if(myList->isDirectory(myList->selected()))
      {
        if(myList->selected().getName() == "..")
          myList->selectParent();
        else
          myList->selectDirectory();
        break;
      }
      [[fallthrough]];
    case FileListWidget::ItemActivated:
      loadRom();
      break;

    case kLoadRndRomCmd:
      loadRandomRom();
      break;

    case ListWidget::kParentDirCmd:
      myList->selectParent();
      break;

    case kOptionsCmd:
      openSettings();
      break;

    case kReloadCmd:
      reload();
      break;

    case FileListWidget::ItemChanged:
      updateUI();
      break;

    case ListWidget::kLongButtonPressCmd:
      if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
        openContextMenu();
      myEventHandled = true;
      break;

    case EditableWidget::kChangedCmd:
    case EditableWidget::kAcceptCmd:
    {
      const bool subDirs = instance().settings().getBool("launchersubdirs");

      myList->setIncludeSubDirs(subDirs);
      if(subDirs && cmd == EditableWidget::kChangedCmd)
      {
        // delay (potentially slow) subdirectories reloads until user stops typing
        myReloadTime = TimerManager::getTicks() / 1000 +
          LauncherFileListWidget::getQuickSelectDelay();
        myPendingReload = true;
      }
      else
        reload();
      break;
    }

    case kQuitCmd:
      handleQuit();
      break;

    case kRomDirChosenCmd:
    {
      string_view romDir = instance().settings().getString("romdir");

      if(myList->currentDir().getPath() != romDir)
      {
        FSNode node(romDir);

        if(!myList->isDirectory(node))
          node = FSNode("~");

        myList->setInitialDirectory(node);
      }
      if(romDir != instance().settings().getString("startromdir"))
      {
        instance().settings().setValue("startromdir", romDir);
        reload();
      }
      break;
    }

    case kFavChangedCmd:
      handleFavoritesChanged();
      break;

    case kRmAllFav:
      myList->removeAllUserFavorites();
      reload();
      break;

    case kRmAllPop:
      myList->removeAllPopular();
      reload();
      break;

    case kRmAllRec:
      myList->removeAllRecent();
      reload();
      break;

    case kExtChangedCmd:
      reload();
      break;

    case kRomViewerChangedCmd:
      setRomInfoEnabled(instance().settings().getFloat("romviewer") > 0.F);
      break;

    case kFontChangedCmd:
      // The launcher font was changed at runtime.  Swap it in place (every
      // widget references the same Font object), then refresh the cached
      // font-derived state and re-flow — no restart required.
      instance().frameBuffer().changeLauncherFont(
          instance().settings().getString("launcherfont"));
      refreshFont();
      break;

    case ContextMenu::kItemSelectedCmd:
      handleContextMenu();
      break;

    case RomInfoWidget::kClickedCmd:
    {
      const string& url = myRomInfoWidget->getUrl();

      if(!url.empty())
        MediaFactory::openURL(url);
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRom()
{
  // Assumes that the ROM will be loaded successfully, has to be done
  //  before saving the config.
  myList->updateFavorites();
  saveConfig();

  const string& result = instance().createConsole(currentNode(), selectedRomMD5());
  if(result.empty())
  {
    instance().settings().setValue("lastrom", myList->getSelectedString());

    // If romdir has never been set, set it now based on the selected rom
    if(instance().settings().getString("romdir").empty())
      instance().settings().setValue("romdir", currentNode().getParent().getShortPath());
  }
  else
    instance().frameBuffer().showTextMessage(result, MessagePosition::MiddleCenter, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openContextMenu(int x, int y)
{
  // Dynamically create context menu for ROM list options

  bool addCancel = false;
  if(x < 0 || y < 0)
  {
    // Long pressed button, determine position from currently selected list item
    x = myList->getLeft() + myList->getWidth() / 2;
    y = myList->getTop() + (myList->getSelected() - myList->currentPos() + 1) * _font.getLineHeight();
    addCancel = true;
  }

  struct ContextItem {
    string label;
    string shortcut;
    string key;
    explicit ContextItem(string_view _label, string_view _shortcut,
                         string_view _key)
      : label{_label}, shortcut{_shortcut}, key{_key} {}
    // For items that have no keyboard shortcut
    ContextItem(string_view _label, string_view _key)
      : label{_label}, key{_key} {}
  };
  using ContextList = std::vector<ContextItem>;
  ContextList items;
  const bool useFavorites = instance().settings().getBool("favorites");

  if(useFavorites)
  {
    if(!currentNode().isDirectory())
    {
      if(LauncherFileListWidget::isUserDir(currentNode().getName()))
        items.emplace_back("Remove all from favorites", "removefavorites");
      if(LauncherFileListWidget::isPopularDir(currentNode().getName()))
        items.emplace_back("Remove all from most popular", "removepopular");
      if(LauncherFileListWidget::isRecentDir(currentNode().getName()))
        items.emplace_back("Remove all from recently played", "removerecent");
      if(myList->inRecentDir())
        items.emplace_back("Remove from recently played", "Ctrl+X", "remove");
      if(myList->inPopularDir())
        items.emplace_back("Remove from most popular", "Ctrl+X", "remove");
    }
    if((currentNode().isDirectory() && currentNode().getName() != "..")
      || Bankswitch::isValidRomName(currentNode()))
      items.emplace_back(myList->isUserFavorite(myList->selected().getPath())
        ? "Remove from favorites"
        : "Add to favorites", "Ctrl+F", "favorite");
  }
  if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
  {
    items.emplace_back("Game properties" + ELLIPSIS, "Ctrl+G", "properties");
    items.emplace_back("Power-on options" + ELLIPSIS, "Ctrl+P", "override");
    if(instance().highScores().enabled())
      items.emplace_back("High scores" + ELLIPSIS, "Ctrl+H", "highscores");
  }
  items.emplace_back(instance().settings().getBool("launcherextensions")
    ? "Disable file extensions"
    : "Enable file extensions", "Ctrl+E", "extensions");
  if(useFavorites && myList->inVirtualDir())
    items.emplace_back(instance().settings().getBool("altsorting")
      ? "Normal sorting"
      : "Alternative sorting", "Ctrl+S", "sorting");
  if(addCancel)
    items.emplace_back("Cancel", ""); // closes the context menu and does nothing

  // Format items for menu, aligning all shortcuts to the right
  VariantList varItems;
  size_t maxLen = 0;
  for(auto& item: items)
    maxLen = std::max(maxLen, item.label.length());

  for(auto& item: items)
    VarList::push_back(varItems, " " + item.label.append(maxLen - item.label.length(), ' ')
      + "  " + item.shortcut + " ", item.key);
  contextMenu().addItems(varItems);

  // Add menu at current x,y mouse location
  contextMenu().show(x + getAbsX(), y + getAbsY(), surface().dstRect(), 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::loadRandomRom()
{
  const Random rand;
  int tries = 100; // limit to 100 tries, in case the directory contains no ROMs

  do {
    myList->setSelected(rand.next() % myList->getList().size());
  } while(myList->isDirectory(myList->selected()) && --tries);
  if(tries)
    loadRom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openSettings()
{
  saveConfig();

  // Create an options dialog, similar to the in-game one
  if(instance().settings().getBool("basic_settings"))
    myDialog = std::make_unique<StellaSettingsDialog>(instance(), parent(),
                                                      AppMode::launcher);
  else
    myDialog = std::make_unique<OptionsDialog>(instance(), parent(), this,
                                               AppMode::launcher);
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openGameProperties()
{
  if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
  {
    // Create game properties dialog
    myDialog = std::make_unique<GameInfoDialog>(instance(), parent(),
      instance().frameBuffer().font(), this);
    myDialog->open();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openGlobalProps()
{
  if(!currentNode().isDirectory() && Bankswitch::isValidRomName(currentNode()))
  {
    // Create global props dialog, which is used to temporarily override
    // ROM properties
    myDialog = std::make_unique<GlobalPropsDialog>(this,
                                              instance().frameBuffer().font());
    myDialog->open();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openHighScores()
{
  // Create an high scores dialog, similar to the in-game one
  myDialog = std::make_unique<HighScoresDialog>(instance(), parent(), _w, _h,
                                           AppMode::launcher);
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::openWhatsNew()
{
  myDialog = std::make_unique<WhatsNewDialog>(instance(), parent());
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::toggleSubDirs(bool toggle)
{
  bool subdirs = instance().settings().getBool("launchersubdirs");

  if(toggle)
  {
    subdirs = !subdirs;
    instance().settings().setValue("launchersubdirs", subdirs);
  }

  if(mySubDirsButton)
  {
    const bool smallIcon = Dialog::lineHeight() < 26;
    const GUI::Icon& subdirsIcon = subdirs
      ? smallIcon ? GUI::icon_subdirs_small_on : GUI::icon_subdirs_large_on
      : smallIcon ? GUI::icon_subdirs_small_off : GUI::icon_subdirs_large_off;

    mySubDirsButton->setIcon(subdirsIcon);
  }
  myList->setIncludeSubDirs(subdirs);
  if(toggle)
    reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::toggleExtensions()
{
  const bool extensions = !instance().settings().getBool("launcherextensions");

  instance().settings().setValue("launcherextensions", extensions);
  myList->setShowFileExtensions(extensions);
  reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::toggleSorting()
{
  if(myList->inVirtualDir())
  {
    // Toggle between normal and alternative sorting of virtual directories
    const bool altSorting = !instance().settings().getBool("altsorting");

    instance().settings().setValue("altsorting", altSorting);
    reload();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::removeAllFavorites()
{
  StringList msg;

  msg.emplace_back("This will remove ALL ROMs from");
  msg.emplace_back("your 'Favorites' list!");
  msg.emplace_back("");
  msg.emplace_back("Are you sure?");
  myConfirmMsg = std::make_unique<GUI::MessageBox>
    (this, _font, msg, _w, _h, kRmAllFav,
      "Yes", "No", "Remove all Favorites", false);
  myConfirmMsg->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::removeAll(string_view name)
{
  StringList msg;

  msg.emplace_back("This will remove ALL ROMs from");
  msg.emplace_back(std::format("your '{}' list!", name));
  msg.emplace_back("");
  msg.emplace_back("Are you sure?");

  myConfirmMsg = std::make_unique<GUI::MessageBox>(
    this, _font, msg, _w, _h, kRmAllPop,
    "Yes", "No", std::format("Remove all {}", name), false);
  myConfirmMsg->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::removeAllPopular()
{
  removeAll("Most Popular");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherDialog::removeAllRecent()
{
  removeAll("Recently Played");
}
