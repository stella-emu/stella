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

#include <cmath>

#include "OSystem.hxx"
#include "Logger.hxx"
#include "Console.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "Debugger.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "Dialog.hxx"
#include "ToolTip.hxx"
#include "Props.hxx"
#include "FSNode.hxx"
#include "DebuggerParser.hxx"
#include "ContextMenu.hxx"
#include "ScrollBarWidget.hxx"
#include "ScrollBarHWidget.hxx"
#ifdef IMAGE_SUPPORT
  #include "PNGLibrary.hxx"
#endif
#include "MemViewWidget.hxx"
#include "MemViewWindowDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewWidget::MemViewWidget(GuiObject* boss, const GUI::Font& font,
                              uInt16 bankSize, uInt16 bankCount,
                              uInt16 initialBankHeight,
                              bool isZoomable,
                              const MemViewWidget::ColorTab& readColorTab,
                              const MemViewWidget::ColorTab& writeColorTab,
                              const MemViewWidget::ColorTab& pcColorTab,
                              uInt32 dataDefaultColor, uInt32 dataFadedColor,
                              string_view typeText, uInt32 baseAddress
)
  : Widget(boss, font),
    CommandSender(boss),
    myInitialBankHeight{initialBankHeight},
    myIsZoomable{isZoomable},
    myIsDisplayable{isDisplayable(bankSize, bankCount, initialBankHeight, !isZoomable)},
    myParams(myIsDisplayable ? bankSize : DEFAULT_BANK_SIZE, myIsDisplayable ? bankCount : 1,
             baseAddress, boss->instance().console().cartridge(), 0, 0),
    myDataLayer(dialog(), myParams, dataDefaultColor, dataFadedColor),
    myReadLayer(dialog(), myParams, readColorTab),
    myWriteLayer(dialog(), myParams, writeColorTab),
    myPcLayer(dialog(), myParams, pcColorTab),
    myPcMarker{dialog(), myParams, MemViewWindowDialog::PC_COLOR_HIGH | 0xFF000000},
    myMouseMarker{dialog(), myParams, FBSurface::getColorRgb(kWidColorHi)},
    myTypeText{typeText}
{
  _flags = Widget::Flag::Enabled | Widget::Flag::ClearBG |
           Widget::Flag::RetainFocus | Widget::Flag::TrackMouse;
  _bgcolor = _bgcolorhi = kDlgColor;

  addFocusWidget(this);

  Logger::debug(std::format("[{}] New MemViewWidget ({} x {})",
    myParams.myDataSize, bankCount, bankSize));

  if(!isDisplayable())
    Logger::error(std::format("MemViewWidget: Unsupported data size: {} ({} x {})",
      myParams.myDataSize, bankCount, bankSize));

  // The data arrangement (horizontal banks x vertical banks and bank height) is unknown until
  // we will be assigned a dimension by setArea() and findBestLayout() will determine the layout

  // Maybe zero (auto-determine bank height by findBestLayout()):
  myParams.myBankHeight = initialBankHeight;

  // Set default parameters (will be overwritten)
  myParams.setAccessDataParams(myParams.myDataSize, 0);
  myParams.setDecayRate(50);

  myDataLayer.setVisibility(true);
  myReadLayer.setVisibility(true);
  myWriteLayer.setVisibility(true);
  myPcLayer.setVisibility(true);

  // Sibling widgets (same boss), not layout children: positioned in
  // setArea() below. Only a zoomable region scrolls
  if(myIsZoomable)
  {
    myVScrollBar = new ScrollBarWidget(boss, font);
    myVScrollBar->setTarget(this);
    myHScrollBar = new ScrollBarHWidget(boss, font);
    myHScrollBar->setTarget(this);
  }

  // Create context menu for commands
  myMenu = std::make_unique<ContextMenu>(this, font, getContextMenuItems());

  // Resize data array
  myCurrentData.resize(myParams.myDataSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::isDisplayable(uInt16 bankSize, uInt16 bankCount,
  uInt16 bankHeight, bool fixedBankHeight)
{
  // A single bank only has to divide into rows; several banks must suit every
  // selectable bank height
  return ((bankSize * bankCount) > 0)
         &&
         (
           (
             (bankCount == 1)
             &&
             (
               fixedBankHeight
               ||
               ((bankSize % MemViewWindowDialog::MAX_BANK_HEIGHT) == 0)
             )
           )
           ||
           (
             fixedBankHeight
             &&
             (bankHeight != 0)
             &&
             ((bankSize % bankHeight) == 0)
           )
           ||
           (
             !fixedBankHeight
             &&
             ((bankSize % MemViewWindowDialog::MAX_BANK_HEIGHT) == 0)
           )
         );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::loadConfig()
{
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int MemViewWidget::getHFrameSize(const GUI::Font& font, bool isZoomable)
{
  return getHBorderSize() + (isZoomable ? ScrollBarWidget::scrollBarWidth(font) : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int MemViewWidget::getVFrameSize(const GUI::Font& font, bool isZoomable)
{
  return getVBorderSize() + (isZoomable ? ScrollBarHWidget::scrollBarHeight(font) : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setArea(int x, int y, int w, int h)
{
#if 0
  Logger::debug(std::format("[{}] Set area {} x {}", myParams.myDataSize, w, h));
#endif

  // _w/_h exclude the scrollbars' own space, so this widget's background-clear
  // (Widget::draw(), via ClearBG) never wipes the scrollbars' pixels
  const int hFrameSize = getHFrameSize(_font, myIsZoomable);
  const int vFrameSize = getVFrameSize(_font, myIsZoomable);
  Widget::setArea(x, y,
    w - hFrameSize + getHBorderSize(), h - vFrameSize + getVBorderSize());

  // A live drag can transiently report an area smaller than the border
  // itself; floor at 1 so this never asks FBSurface for a non-positive size
  myParams.mySurfaceWidth = std::max(1, w - hFrameSize);
  myParams.mySurfaceHeight = std::max(1, h - vFrameSize);

  // Live-resizable, so re-fit to the assigned area on every resize, at the
  // same fixed params naturalSize() used to arrive at that area
  auto [size, layoutParams] = calcNeededSize(myParams.mySurfaceWidth, myParams.mySurfaceHeight,
    myIsZoomable, myParams.myBankSize, myParams.myBankCount, myInitialBankHeight, myIsDisplayable,
    myParams.mySingleRow, myParams.mySeparators
  );

  myParams.setLayoutParameters(layoutParams, myParams.mySingleRow, myParams.mySeparators);

  // mySurfacePosX/Y need dialog().surface(), not valid yet here; set in render()
  myParams.layoutRecalc();

  reallocateLayerSurfaces();

  // The vertical bar spans the full height; the horizontal one sits below
  // the view, up to the vertical bar
  if(myVScrollBar)
  {
    myVScrollBar->setPos(_x + _w, _y);
    myVScrollBar->setHeight(h);
    myHScrollBar->setPos(_x, _y + _h);
    myHScrollBar->setWidth(_w);

    // The visible fraction of the content has changed with the area
    recalcScrollBars();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// A non-zoomable region shrink-fits within a ceiling (myMaxCandidateW/H)
Common::Size MemViewWidget::naturalSize() const
{
  if(myIsZoomable)
    return Widget::naturalSize();

  // Just calculate the size for simple RAM views
  int dummyZoomLevel;
  Common::Size size = calcSizeAndZoom(
    myMaxCandidateW, myMaxCandidateH,
    myParams.myBankWidth, myParams.myBankHeight,
    1, 1,
    false, dummyZoomLevel
  );

  return Common::Size(I32(size.w) + 2 * BORDER,
                      I32(size.h) + 2 * BORDER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size MemViewWidget::minSize(int maxWidth) const
{
  // naturalSize() shrink-fits to the current candidate area, so it chases
  // the window down to a 1px-per-bit sliver and cannot be the floor here.
  // Use a fixed minimum zoom level instead
  if(!myIsZoomable)
  {
    const int minWidth = myParams.myBankWidth * 8 * RAM_MIN_ZOOM;
    const int minHeight = myParams.myBankHeight * RAM_MIN_ZOOM;
    return Common::Size(minWidth + 2 * BORDER, minHeight + 2 * BORDER);
  }

  // Width asks for the whole bank arrangement at the highest zoom, up to
  // ROM_MIN_ZOOM, that fits within maxWidth, so it shows without scrolling
  // and without empty margins.  If even zoom 1 is too wide, take maxWidth and
  // let the horizontal scrollbar cover the rest.  Height is free to be
  // squeezed, since the vertical scrollbar covers it
  const int separatorWidth = (myParams.mySeparators && (myParams.myHBanks >= 2))
    ? (myParams.myHBanks - 1) * MemViewParams::SEPARATOR_WIDTH : 0;
  const int frameWidth = 2 * BORDER + ScrollBarWidget::scrollBarWidth(_font);
  const auto arrangementWidth = [&](int zoom) {
    return myParams.myBankWidth * 8 * myParams.myHBanks * zoom + separatorWidth + frameWidth;
  };

  int width = maxWidth;
  for(int zoom = ROM_MIN_ZOOM; zoom >= 1; --zoom)
  {
    if(arrangementWidth(zoom) <= maxWidth)
    {
      width = arrangementWidth(zoom);
      break;
    }
  }

  return Common::Size(std::max(1, width),
                      _font.getLineHeight() * 3 + 2 * BORDER
                      + ScrollBarHWidget::scrollBarHeight(_font));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::reallocateLayerSurfaces()
{
  // This window is live-resizable, so force a fresh allocation at the new
  // size whenever the widget's own area changes
  if(myDataLayer.mySurface)
  {
    FrameBuffer::deallocateSurface(dialog().window(), myDataLayer.mySurface);
    myDataLayer.mySurface = nullptr;
  }
  if(myReadLayer.mySurface)
  {
    FrameBuffer::deallocateSurface(dialog().window(), myReadLayer.mySurface);
    myReadLayer.mySurface = nullptr;
  }
  if(myWriteLayer.mySurface)
  {
    FrameBuffer::deallocateSurface(dialog().window(), myWriteLayer.mySurface);
    myWriteLayer.mySurface = nullptr;
  }
  if(myPcLayer.mySurface)
  {
    FrameBuffer::deallocateSurface(dialog().window(), myPcLayer.mySurface);
    myPcLayer.mySurface = nullptr;
  }

  // Every layer must redraw into its fresh surface, the data layer included
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::zoom(int level)
{
  if (!myIsZoomable || (myParams.myZoomLevel == level))
    return;

  // Zoom towards or away from mouse position

  // Limit to positions within the shown data (if mouse is over border)
  int x = myParams.limitPosX(myClickX);
  int y = myParams.limitPosY(myClickY);

  // Bring to current pos within data
  x += myParams.myOffsetX;
  y += myParams.myOffsetY;

  // Multiply with change in zoom level
  const double zoomFactor = DBL(level) / DBL(myParams.myZoomLevel);
  x = I32(round(DBL(x) * zoomFactor));
  y = I32(round(DBL(y) * zoomFactor));

  // Apply new zoom
  myParams.myZoomLevel = level;

  // Calculate new correct sizes
  myParams.layoutPreCalc();

  // Set new offsets
  myParams.myOffsetX = myParams.myLeftBorderWidth ? 0 : (x - myClickX);
  myParams.myOffsetY = myParams.myTopBorderHeight ? 0 : (y - myClickY);

  // Calc the rest
  myParams.layoutPostCalc();
  recalcScrollBars();
  updateMarker();
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::lockedSingleRow() const
{
  // This is currently the same as having a single bank because all
  // other configurations could change myVBanks when resizing the window
  return lockedSingleBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::lockedSingleBank() const
{
  return (myParams.myBankCount == 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size MemViewWidget::calcSizeAndZoom(int availableWidth, int availableHeight,
                                            int bankWidth, int bankHeight,
                                            int hBanks, int vBanks,
                                            bool separators, int& minZoomLevel)
{
  const int totalSeparatorWidth = (separators && (hBanks >= 2)) ? (hBanks - 1) * MemViewParams::SEPARATOR_WIDTH : 0;
  const int totalSeparatorHeight = (separators && (vBanks >= 2)) ? (vBanks - 1) * MemViewParams::SEPARATOR_HEIGHT : 0;
  const int minHeight = bankHeight * vBanks;
  const int minWidth = bankWidth * 8 * hBanks;

  // Automatically determine minimum zoom level
  const int hZoom = (availableWidth - totalSeparatorWidth) / minWidth;
  const int vZoom = (availableHeight - totalSeparatorHeight) / minHeight;
  minZoomLevel = std::max(std::min(hZoom, vZoom), 1);

  return Common::Size(
    minWidth * minZoomLevel + totalSeparatorWidth,
    minHeight * minZoomLevel + totalSeparatorHeight
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Best hBanks x vBanks arrangement and zoom for the given area, forced to
// one row or searched
std::tuple<Common::Size, MemViewParams::LayoutParams> MemViewWidget::findBestLayout(
  const uInt16& bankSize, const uInt16& bankCount,
  const int& innerSurfaceW, const int& innerSurfaceH,
  const bool& isZoomable,
  uInt16 bankHeight,
  bool singleRow, bool separators
)
{
  if (bankCount == 1)
    singleRow = true;

  const double destAspect = DBL(innerSurfaceW) / DBL(innerSurfaceH);
  const int firstVBanks = 1;
  const int lastVBanks = singleRow ? 1 : bankCount;
  double bestAspectDiff = 0.0;
  bool fits = false;
  Common::Size bestSize;
  MemViewParams::LayoutParams layoutParams;

  // Determine best format to display the data based on the available area size
  uInt16 firstHeight = 64;
  uInt16 lastHeight = 512;
  if (!isZoomable || (bankHeight != 0))
    firstHeight = lastHeight = bankHeight;

  for (uInt16 testHeight = firstHeight; testHeight <= lastHeight; testHeight *= 2)
  {
    const uInt16 testWidth = bankSize / testHeight;
    const uInt16 bankRatio = testHeight / testWidth;
    const bool preferred = (bankRatio >= 16) && (bankRatio <= 32);

    for (int testVBanks = firstVBanks; testVBanks <= lastVBanks; testVBanks++)
    {
      if (bankCount % testVBanks)
        continue;

      const int testHBanks = bankCount / testVBanks;

      int testZoomLevel = 0;
      Common::Size testSize = MemViewWidget::calcSizeAndZoom(
        innerSurfaceW,
        innerSurfaceH,
        testWidth,
        testHeight,
        testHBanks,
        testVBanks,
        separators,
        testZoomLevel
      );

      bool testFits =
        (testSize.w <= U32(innerSurfaceW))
        &&
        (testSize.h <= U32(innerSurfaceH))
      ;
      // Calculate aspect ratio without separators to give the preferred ratio a fair chance
      const double netWidth = DBL(testWidth * testHBanks * 8);
      const double netHeight = DBL(testHeight * testVBanks);
      double testDiff = fabs((netWidth / netHeight) - destAspect);

      // Make the difference for our preferred bank aspect slightly better to prefer these
      // when there is very little difference between the arrangements
      if (preferred)
        testDiff *= 0.75;

      if (
        (layoutParams.minZoomLevel == 0)
        ||
        (!fits && testFits)
        ||
        (
          (!fits || testFits)
          &&
          (
            (testZoomLevel > layoutParams.minZoomLevel)
            ||
            (
              (testZoomLevel == layoutParams.minZoomLevel)
              &&
              (testDiff < bestAspectDiff)
            )
          )
        )
      )
      {
        // Take new best option
        bestSize = testSize; 
        layoutParams.bankHeight = testHeight;
        layoutParams.bankWidth = testWidth;
        layoutParams.vBanks = testVBanks;
        layoutParams.hBanks = testHBanks;
        layoutParams.minZoomLevel = testZoomLevel;
        bestAspectDiff = testDiff;
        fits = testFits;
      }
    }
  }

  // At least one condition must be met
  assert(fits || isZoomable);

  if (layoutParams.vBanks == 1)
    singleRow = true;

  return {bestSize, layoutParams};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::tuple<Common::Size, MemViewParams::LayoutParams> MemViewWidget::calcNeededSize(
  int w, int h, bool isZoomable, uInt16 bankSize, uInt16 bankCount, uInt16 initialBankHeight,
  bool& success, bool singleRow, bool separators
)
{
  uInt32 dataSize = U32(bankSize * bankCount);

  // Evaluate bank height
  uInt16 bankHeight = U16(std::min(dataSize, U32(initialBankHeight)));
  const bool fixedBankHeight = !isZoomable;

  // Check if bank is displayable
  success = isDisplayable(bankSize, bankCount, initialBankHeight, fixedBankHeight);

  if (!success)
  {
    Logger::error(std::format("MemViewWidget: Unsupported data size: {} ({} x {})",
      bankCount * bankSize, bankCount, bankSize));
    // Setup some emergency values to prevent crashing
    bankSize = DEFAULT_BANK_SIZE;
    bankCount = 1;
    dataSize = DEFAULT_BANK_SIZE;
  }

  // Find the initial best layout to use (how are the banks arranged)
  // and return size and layout parameters
  auto [size, layoutParams] = findBestLayout(bankSize, bankCount, w, h,
    isZoomable, bankHeight, singleRow, separators
  );

  return {size, layoutParams};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Never called for a non-zoomable region (RAM): its layout is fixed at
// construction and the settings panel never touches it
void MemViewWidget::setLayoutParameters(bool singleRow, bool separators)
{
  if(!isSetup())
    return;

  // Calculate new arrangement and set to params
  auto [size, layoutParams] = findBestLayout(
    myParams.myBankSize, myParams.myBankCount, myParams.mySurfaceWidth, myParams.mySurfaceHeight,
    myIsZoomable, myInitialBankHeight, singleRow, separators
  );
  myParams.setLayoutParameters(layoutParams, singleRow, separators);

  // Set original data again to let the corresponding layer copy the data to it's new layout
  myDataLayer.updateData(myCurrentData);

  // Update heatmap layout to new arrangement when paused or in debugger
  if(instance().eventHandler().state() != EventHandlerState::EMULATION)
    heatmapsToFields(true);

  recalcScrollBars();
  updateMarker();
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setVisualParameters(bool showData, bool showPc, bool showReads,
                                        bool showWrites, bool inverted, bool byteFade)
{
  if(!isSetup())
    return;

  // Switch on/off visibility of each layer
  myDataLayer.setVisibility(showData);
  myPcLayer.setVisibility(showPc);
  myReadLayer.setVisibility(showReads);
  myWriteLayer.setVisibility(showWrites);

  // Set parameters specific to the data layer
  myDataLayer.setVisualParameters(
    inverted,
    // No byte fade for single column data (e.g. RAM)
    (myParams.myDataSize <= myParams.myBankHeight) ? false : byteFade
  );

  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setDecayRate(int percentage)
{
  if(!isSetup())
    return;

  myParams.setDecayRate(percentage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::clearHeatmaps()
{
  if (!isSetup())
    return;

  myReadLayer.clearHeatmap();
  myWriteLayer.clearHeatmap();
  myPcLayer.clearHeatmap();
  heatmapsToFields(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Uses public setters, since the scrollbars keep those fields private
void MemViewWidget::recalcScrollBars()
{
  if(!myIsZoomable)
    return;

  myVScrollBar->setNumEntries(myParams.myTotalHeight);
  myVScrollBar->setEntriesPerPage(myParams.myCurrentHeight);
  myVScrollBar->setCurrentPos(myParams.myOffsetY);
  myVScrollBar->recalc();

  myHScrollBar->setNumEntries(myParams.myTotalWidth);
  myHScrollBar->setEntriesPerPage(myParams.myCurrentWidth);
  myHScrollBar->setCurrentPos(myParams.myOffsetX);
  myHScrollBar->recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::writeBankHeightConfig() const
{
  Settings& settings = instance().settings();

  switch (myParams.myBankSize)
  {
    case 1024:
      settings.setValue("memview.bh1k", myInitialBankHeight);
      break;
    case 2048:
      settings.setValue("memview.bh2k", myInitialBankHeight);
      break;
    case 4096:
      settings.setValue("memview.bh4k", myInitialBankHeight);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VariantList MemViewWidget::getContextMenuItems() const
{
  VariantList list;
  VarList::push_back(list, "Toggle breakpoint", "bp");
  VarList::push_back(list, "Toggle R/W trap", "rwt");
  VarList::push_back(list, "Toggle read trap", "rt");
  VarList::push_back(list, "Toggle write trap", "wt");
  VarList::push_back(list, "Show totals", "st");
#ifdef IMAGE_SUPPORT
  VarList::push_back(list, "Save data picture", "pic");
#endif
  if (myIsZoomable)
  {
    VarList::push_back(list, std::format("Bank height auto{}",
      ((myInitialBankHeight == 0) ? " *" : "")), "bh0");
    VarList::push_back(list, std::format("Bank height 64{}",
      ((myInitialBankHeight == 64) ? " *" : "")), "bh64");
    VarList::push_back(list, std::format("Bank height 128{}",
      ((myInitialBankHeight == 128) ? " *" : "")), "bh128");
    VarList::push_back(list, std::format("Bank height 256{}",
      ((myInitialBankHeight == 256) ? " *" : "")), "bh256");
    VarList::push_back(list, std::format("Bank height 512{}",
      ((myInitialBankHeight == 512) ? " *" : "")), "bh512");
  }

  return list;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                  int data, int id)
{
  switch (cmd)
  {
    case GuiObject::Cmd::SetPosition:
      if ((sender == myVScrollBar) && (myParams.myOffsetY != data))
      {
        myParams.myOffsetY = data;
        myParams.layoutRecalc();
        updateMarker();
        setDirtyData();
      }
      else if ((sender == myHScrollBar) && (myParams.myOffsetX != data))
      {
        myParams.myOffsetX = data;
        myParams.layoutRecalc();
        updateMarker();
        setDirtyData();
      }
      break;
    case ContextMenu::Cmd::ItemSelected:
    {
      const string& rmb = myMenu->getSelectedTag().toString();
      if ((rmb == "bp") || (rmb == "rwt") || (rmb == "rt") || (rmb == "wt"))
      {
        // User wishes to run a debugger function
        string message;
        int bank = 0;
        Common::RwAddress address = myParams.getAddress(myRightClickX, myRightClickY, &bank);

        if (address.valid)
        {
          Debugger& debugger = instance().debugger();
          const bool wasLocked = debugger.systemIsLocked();
          if (!wasLocked)
            debugger.lockSystem();

          // Build debugger command
          string command;
          if (rmb == "bp")
          {
            if (!(myParams.myBaseAddress & QUERY_RAM_BANK_ORIGIN))
              command = std::format("break ${:X} {}", address.read, bank);
            else
              command = std::format("break ${:X}", address.read);
          }
          else if (rmb == "rwt")
          {
            command = std::format("trap ${:X}", address.read);
            if (address.write != address.read)
            {
              debugger.parser().run(command);
              command = std::format("trap ${:X}", address.write);
            }
          }
          else if (rmb == "rt")
            command = std::format("trapRead ${:X}", address.read);
          else if (rmb == "wt")
            command = std::format("trapWrite ${:X}", address.write);
          else
            assert(false);

          // Run
          message = debugger.parser().run(command);
          if (!wasLocked)
            debugger.unlockSystem();
        }
        else
        {
          message = "Address currently not mapped";
        }
        dialog().showTextMessage(message);
      }
      else if (rmb == "st")
      {
        // Show totals
        const bool goToPause = (instance().eventHandler().state() == EventHandlerState::EMULATION);
        myReadLayer.loadTotals(goToPause);
        myWriteLayer.loadTotals(goToPause);
        myPcLayer.loadTotals(goToPause);
        heatmapsToFields(true);
        if (goToPause)
        {
          // Set current state to paused so the user can actually see the
          // total values before they disappear
          instance().eventHandler().setState(EventHandlerState::PAUSE);
        }
      }
      else if ((rmb == "bh0") || (rmb == "bh64") || (rmb == "bh128") || (rmb == "bh256") || (rmb == "bh512"))
      {
        uInt16 bankHeight = U16(BSPF::stoi(rmb.substr(2)));
        if (myInitialBankHeight != bankHeight)
        {
          myInitialBankHeight = bankHeight;
          if ((bankHeight == 0) || (myParams.myBankHeight != bankHeight))
            // Set the layout parameters again to let findBestLayout use the
            // new bank height setting
            setLayoutParameters(myParams.mySingleRow, myParams.mySeparators);
          }
          // Update context menu for new bank height selection
          myMenu->addItems(getContextMenuItems());
          // Store for next time
          writeBankHeightConfig();
      }
#ifdef IMAGE_SUPPORT
      else if (rmb == "pic")
      {
        // Save a picture of the data
        savePicture();
      }
#endif
      break;
    }
    default:
      break;

  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if (!isSetup())
    return;

  x -= BORDER;
  y -= BORDER;

  myClickX = x;
  myClickY = y;

  // Button 1 is for 'drag'/movement of the image
  // Button 2 is for context menu
  if(b == MouseButton::LEFT)
  {
    // Indicate mouse drag started/in progress
    myMouseDragging = true;
    dialog().tooltip().hide();
  }
  else if(b == MouseButton::RIGHT)
  {
    myRightClickX = myClickX;
    myRightClickY = myClickY;

    // Add menu at current x, y mouse location
    myMenu->show(x + getLeft(), y + getTop(), dialog().surface().dstRect());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  myMouseDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleMouseLeft()
{
  if (!isSetup())
    return;
  myMouseDragging = false;
  if(myMouseMarker.set(false))
    requestRender();
  Widget::handleMouseLeft();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleMouseWheel(int x, int y, int direction)
{
  if (!isSetup())
    return;

  dialog().tooltip().hide();

  x -= BORDER;
  y -= BORDER;

  // Zoom towards mouse position
  myClickX = x;
  myClickY = y;

  if(direction > 0)
  {
    // Zoom out
    if(myParams.myZoomLevel > myParams.myMinZoom)
      zoom(myParams.myZoomLevel - 1);
  }
  else
  {
    // Zoom in
    if(myParams.myZoomLevel < MemViewParams::myMaxZoom)
      zoom(myParams.myZoomLevel + 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::handleMouseMoved(int x, int y)
{
  if (!isSetup())
    return;

  x -= BORDER;
  y -= BORDER;

  if(myMouseDragging)
  {
    const int diffx = x - myClickX;
    const int diffy = y - myClickY;

    myClickX = x;
    myClickY = y;

    myParams.myOffsetX -= diffx;
    myParams.myOffsetY -= diffy;

    myParams.layoutRecalc();
    recalcScrollBars();
    updateMarker();
    setDirtyData();
  }
  else if (intPosInData(x, y))
  {
    // Data marker
    unsigned int byteOffset = 0;
    int xFracPixels = 0;
    int yFracPixels = 0;
    myParams.getPosition(x - myParams.myLeftBorderWidth, y - myParams.myTopBorderHeight,
      &byteOffset, &xFracPixels, &yFracPixels);
    if(myMouseMarker.set(true, byteOffset))
      requestRender();
  }
  else if(myMouseMarker.set(false))
    requestRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Raw heat values to display colors via each layer's ColorTab, with the
// same bank-row rearrangement as updateData()
void MemViewWidget::heatmapsToFields(bool force)
{
  if (
    !force
    &&
    (!myReadLayer.myVisibility && !myWriteLayer.myVisibility && !myPcLayer.myVisibility)
  )
    return;

  // Precalc some often used values
  const int myBankHeightMinusOne = myParams.myBankHeight - 1;
  const int lineStopAdd = myParams.myBankRowSize - myParams.myBankHeight + 1;
  const int nextLineSub = myParams.myBankRowSize - 1;

  // The raw access data arrays are our sources
  const MemViewAccessLayer::HeatmapValue* srcReadData = myReadLayer.myHeatmap.data();
  const MemViewAccessLayer::HeatmapValue* srcWriteData = myWriteLayer.myHeatmap.data();
  const MemViewAccessLayer::HeatmapValue* srcPcData = myPcLayer.myHeatmap.data();

  // The layers are our destinations
  uInt32* destReadLayer = myReadLayer.myFields.data(); // NOLINT(misc-const-correctness)
  uInt32* destWriteLayer = myWriteLayer.myFields.data(); // NOLINT(misc-const-correctness)
  uInt32* destPcLayer = myPcLayer.myFields.data(); // NOLINT(misc-const-correctness)

  // Stop pointer values for comparison
  // (we only work on the srcReadData here because all three buffers
  // are the same size)
  const MemViewAccessLayer::HeatmapValue* lineStop = srcReadData + myParams.myBankRowSize;
  const MemViewAccessLayer::HeatmapValue* bankRowStop = srcReadData + myParams.myBankRowSize + myBankHeightMinusOne;
  const MemViewAccessLayer::HeatmapValue* fullStop = srcReadData + myParams.myDataSize + myBankHeightMinusOne;

  const bool doRead = force || myReadLayer.myVisibility;
  const bool doWrite = force || myWriteLayer.myVisibility;
  const bool doPc = force || myPcLayer.myVisibility;

  // One total line at the time
  while (true)
  {
    do
    {
      // Read-access data
      if (doRead)
        *(destReadLayer++) = myReadLayer.myColorTab[static_cast<unsigned int>(*srcReadData)];

      // Write-access data
      if (doWrite)
        *(destWriteLayer++) = myWriteLayer.myColorTab[static_cast<unsigned int>(*srcWriteData)];

      // PC-access data
      if (doPc)
        *(destPcLayer++) = myPcLayer.myColorTab[static_cast<unsigned int>(*srcPcData)];

      // Advance src pointers
      srcReadData += myParams.myBankHeight;
      srcWriteData += myParams.myBankHeight;
      srcPcData += myParams.myBankHeight;

    } while (srcReadData != lineStop);

    if (srcReadData == fullStop)
    {
      // Done
      break;
    }
    if (srcReadData == bankRowStop)
    {
      // Next bank row
      srcReadData -= myBankHeightMinusOne;
      srcWriteData -= myBankHeightMinusOne;
      srcPcData -= myBankHeightMinusOne;
      lineStop += lineStopAdd;
      bankRowStop += myParams.myBankRowSize;
    }
    else
    {
      // Next line
      srcReadData -= nextLineSub;
      srcWriteData -= nextLineSub;
      srcPcData -= nextLineSub;
      lineStop++;
    }
  }

  myAccessIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::updateData(const ByteSpan& data)
{
  if(!isSetup())
    return;

  if(data.size() != myParams.myDataSize)
  {
    cerr << "MemView data size mismatch (" << data.size() << " != " << myParams.myDataSize << ")\n";
    return;
  }

  myCurrentData.assign(data.begin(), data.end());
  myDataLayer.updateData(myCurrentData);
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::setAccessDataParams(uInt32 size, uInt32 offset)
{
  Logger::debug(std::format("[{}] Access counters offset = {}", myParams.myDataSize, offset));
  Logger::debug(std::format("[{}] Access counters size   = {}", myParams.myDataSize, size));
  return myParams.setAccessDataParams(size, offset);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::updateAccessData(Device::AccessCounter* readData,
                                     Device::AccessCounter* writeData,
                                     Device::AccessCounter* pcData,
                                     uInt32 elapsedCycles, int elapsedFrames)
{
  if(!isSetup())
    return;

  const auto currentDecrement =
    static_cast<MemViewAccessLayer::HeatmapValue>(
      (DBL(elapsedCycles) / 19912.0) * myParams.myAccessDataDecrement);

  myReadLayer.updateAccessData(readData, currentDecrement, elapsedFrames);
  myWriteLayer.updateAccessData(writeData, currentDecrement, elapsedFrames);
  myPcLayer.updateAccessData(pcData, currentDecrement, elapsedFrames);

  heatmapsToFields();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::drawWidget(bool hilite)
{
  // Draw outer frame
  FBSurface& s = dialog().surface();
  s.frameRect(_x, _y, _w, _h, hilite ? kWidColorHi : kColor);

  if(!isSetup())
  {
    s.drawString(_font, TEXT_UNSUPPORTED, _x, _y + (_h - _font.getFontHeight()) / 2,
                 _w, kTextColor, TextAlign::Center);
    return;
  }

  // Normally done by the dialog's tick(), which doesn't run while another
  // dialog is on top of it
  drawLayers();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::drawLayers()
{
  if(!isSetup() || (!myDataIsDirty && !myAccessIsDirty))
    return false;

  if(myDataIsDirty)
    myDataLayer.draw();
  if(myAccessIsDirty)
  {
    myReadLayer.draw();
    myWriteLayer.draw();
    myPcLayer.draw();
  }
  myDataIsDirty = myAccessIsDirty = false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setDirtyData()
{
  myDataIsDirty = myAccessIsDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::requestRender() const
{
  FrameBuffer::setPendingRender(dialog().window());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::render()
{
  if (!isSetup())
    return;

  // The view moves on a resize, so its origin is read at render time
  const Common::Rect& s_dst = dialog().surface().dstRect();
  const Int32 dpi = instance().frameBuffer().hidpiScaleFactor(dialog().window());
  myParams.mySurfacePosX = s_dst.x() + (getAbsX() + BORDER) * dpi;
  myParams.mySurfacePosY = s_dst.y() + (getAbsY() + BORDER) * dpi;

  // Render layers bottom to top
  myDataLayer.render();
  myReadLayer.render();
  myWriteLayer.render();
  myPcLayer.render();
  myPcMarker.render();
  myMouseMarker.render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::updateMarker()
{
  myPcMarker.update();
  myMouseMarker.update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::updateRest()
{
  if (!isSetup())
    return;

  const bool stopped = (instance().eventHandler().state() != EventHandlerState::EMULATION);

  // Update ToolTip (even if the mouse didn't move)
  if (!stopped)
    dialog().tooltip().refresh(this);

  // Care about PC counter marker
  if (stopped)
  {
    // Get current bank from the cartridge debugger
    int bank = instance().debugger().cartDebug().getPCBank();

    // Adjust for RAM banks if necessary
    if (myParams.myBaseAddress & QUERY_RAM_BANK_ORIGIN)
      bank -= myParams.myCartridge.ramBankOffset();

    if ((bank >= 0) && (bank < myParams.myBankCount))
    {
      // Check if the PC really is in the address range of that bank
      const uInt16 pc = U16(instance().debugger().cpuDebug().pc());

      Common::RwAddress address = myParams.getBankOrigin(bank, pc);
      if (
        address.valid
        &&
        (pc >= address.read)
        &&
        (pc < (address.read + myParams.myBankSize))
      )
      {
        const int offset = bank * myParams.myBankSize + pc - address.read;
        const int byteOffset = myParams.getRearrangedOffset(offset);

        if (myPcMarker.set(true, byteOffset))
          requestRender();

        return;
      }
    }
  }

  // PC marker not active
  if (myPcMarker.set(false))
    requestRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string MemViewWidget::getToolTip(const Common::Point& pos) const
{
  if (!extPosInData(pos))
    return "";

  const Common::Point internalPos = extPosConv(pos);
  int bank = 0;
  unsigned int offset = 0;
  const Common::RwAddress address = myParams.getAddress(
    internalPos.x, internalPos.y, &bank, &offset);

  // Build tip

  // Hexadecimal
  const uInt8 value = myCurrentData[offset];
  string text = std::format("${:0>2X}", value);

  // Decimal
  text += std::format("  {:>4}", std::format("#{}", value));
  if (value >= 0x80)
    text += std::format("/{:<4}", value - 256);
  else
    text += "     ";

  // Binary
  text += std::format("  %{:0>8B}", myCurrentData[offset]);

  // Address
  if (
    address.valid
    &&
    !(myParams.myBaseAddress & (QUERY_ROM_BANK_ORIGIN | QUERY_RAM_BANK_ORIGIN)) 
    &&
    ((myParams.myBaseAddress + myParams.myDataSize) <= 0x100)
  )
  {
    // 2-digit address (internal RAM)
    text += std::format("\n${:0>2X}", address.read);
#if 0
    if (address.write != address.read)
      text += std::format("/${:0>2X}", address.write);
#endif
    text += " [" + myTypeText + "]";
  }
  else
  {
    // 4-digit address
    if (address.valid)
    {
      // Normal address display
      text += std::format("\n${:0>4X}", address.read);
      if (address.write != address.read)
        text += std::format("/${:0>4X}", address.write);
    }
    else
    {
      // Invalid address (probably currently not mapped)
      text += std::format("\n(${:0>4X}", address.read);
      if (address.write != address.read)
        text += std::format("/${:0>4X})", address.write);
      else
        text += ")";
    }
    text += " [" + myTypeText;
    // Bank?
    if (myParams.myBankCount > 1)
      text += std::format(" Bank {}]", bank);
    else
      text += ']';
  }

  // Total access counters
  text += std::format("\nTotal: PC {}, R {}, W {}",
    myPcLayer.getTotalValue(offset),
    myReadLayer.getTotalValue(offset),
    myWriteLayer.getTotalValue(offset)
  );

  // Delta access counters since last clear
  text += std::format("\nDelta: PC {}, R {}, W {}",
    myPcLayer.getDeltaValue(offset),
    myReadLayer.getDeltaValue(offset),
    myWriteLayer.getDeltaValue(offset)
  );

  // Label (if any)
  if (address.valid)
  {
    const CartDebug& cartDebug = instance().debugger().cartDebug();
    string label = cartDebug.getLabel(U16(address.read), true);
    if (!label.empty())
      text += "\n" + label;
  }

  return text;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Point MemViewWidget::extPosConv(const Common::Point& pos) const
{
  // Our inner surface starts just inside the frame, at (_x, _y) + BORDER
  return Common::Point(
    BSPF::clamp(pos.x - _x - BORDER, 0, _w - 2 * BORDER - 1),
    BSPF::clamp(pos.y - _y - BORDER, 0, _h - 2 * BORDER - 1)
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::extPosInData(const Common::Point& pos) const
{
  return
    (pos.x >= (_x + BORDER + myParams.myLeftBorderWidth))
    &&
    (pos.x < (_x + BORDER + myParams.myLeftBorderWidth + myParams.myCurrentWidth))
    &&
    (pos.y >= (_y + BORDER + myParams.myTopBorderHeight))
    &&
    (pos.y < (_y + BORDER + myParams.myTopBorderHeight + myParams.myCurrentHeight));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::intPosInData(int x, int y) const
{
  return
    (x >= myParams.myLeftBorderWidth)
    &&
    (x < (myParams.myLeftBorderWidth + myParams.myCurrentWidth))
    &&
    (y >= myParams.myTopBorderHeight)
    &&
    (y < (myParams.myTopBorderHeight + myParams.myCurrentHeight));
}

#ifdef IMAGE_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::savePicture()
{
  // Build an unique filename
  string ppath = std::format("{}{}", instance().snapshotSaveDir().getPath(),
    instance().console().properties().get(PropType::Cart_Name));
  // Determine if the file already exists, checking each successive filename
  // until one doesn't exist
  if(FSNode(ppath + ".png").exists())
  {
    for(const uInt32 i: std::views::iota(1U))
    {
      const string candidate = std::format("{}_{}.png", ppath, i);
      if(!FSNode(candidate).exists())
      {
        ppath += std::format("_{}", i);
        break;
      }
    }
  }
  ppath += ".png";

  // First build a dummy Params struct and reset some values
  // to get rid of the borders and be able to save the whole thing
  // with a defined zoom level
  MemViewParams dummyParams = myParams;
  if (!dummyParams.mySeparators)
    dummyParams.myZoomLevel = 1;
  dummyParams.layoutPreCalc();
  dummyParams.mySurfaceWidth = dummyParams.myTotalWidth;
  dummyParams.mySurfaceHeight = dummyParams.myTotalHeight;
  dummyParams.layoutPreCalc();
  dummyParams.layoutPostCalc();
  // Let the data layer draw the content
  uIntArray dummyData(SZT(dummyParams.myTotalWidth) * dummyParams.myTotalHeight);
  myDataLayer.drawDirect(dummyParams, dummyData.data());

  // Try to save
  string message = "Picture saved";
  try
  {
    const Common::Rect rect(dummyParams.mySurfaceWidth, dummyParams.mySurfaceHeight);
    PNGLibrary::saveImage(ppath, rect, dummyData.data(), dummyParams.myTotalWidth);
  }
  catch(const std::runtime_error& e)
  {
    message = e.what();
  }
  dialog().showTextMessage(message);
}
#endif
