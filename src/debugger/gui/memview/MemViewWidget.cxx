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
                              uInt16 bankSize, uInt16 bankCount, uInt16 bankHeight,
                              uInt16 baseAddress,
                              bool isZoomable,
                              const MemViewWidget::ColorTab& readColorTab,
                              const MemViewWidget::ColorTab& writeColorTab,
                              const MemViewWidget::ColorTab& pcColorTab,
                              uInt32 dataDefaultColor, uInt32 dataFadedColor,
                              string_view typeText, int mirrorAddrOffset)
  : Widget(boss, font),
    CommandSender(boss),
    myIsZoomable{isZoomable},
    myIsSetup{isDisplayable(bankSize, bankCount, bankHeight)},
    myParams(myIsSetup ? bankSize : DEFAULT_BANK_SIZE, myIsSetup ? bankCount : 1,
             baseAddress, boss->instance().console().cartridge(), 0, 0),
    myDataLayer(dialog(), myParams, dataDefaultColor, dataFadedColor),
    myReadLayer(dialog(), myParams, readColorTab),
    myWriteLayer(dialog(), myParams, writeColorTab),
    myPcLayer(dialog(), myParams, pcColorTab),
    myPcMarker{dialog(), myParams, MemViewWindowDialog::PC_COLOR_HIGH | 0xFF000000},
    myMouseMarker{dialog(), myParams, FBSurface::getColorRgb(kWidColorHi)},
    myTypeText{typeText},
    myMirrorAddrOffset{mirrorAddrOffset}
{
  _flags = Widget::Flag::Enabled | Widget::Flag::ClearBG |
           Widget::Flag::RetainFocus | Widget::Flag::TrackMouse;
  _bgcolor = _bgcolorhi = kDlgColor;

  addFocusWidget(this);

  Logger::debug(std::format("New MemViewWidget"));

  if(isSetup())
  {
    Logger::debug(std::format("Data size   = {}", myParams.myDataSize));
    Logger::debug(std::format("Banks       = {}", bankCount));
    Logger::debug(std::format("Bank size   = {}", bankSize));
    Logger::debug(std::format("Bank width  = {}", bankSize / bankHeight));
    Logger::debug(std::format("Bank height = {}", bankHeight));
  }
  else
    Logger::error(std::format("MemViewWidget: Unsupported bank size: {}", bankSize));

  // One row of banks at the initial bank height, no separators.  A zoomable
  // region gets the real settings from loadConfig()'s setLayoutParameters();
  // a non-zoomable one keeps this arrangement and only re-fits its zoom
  myParams.setLayoutParameters(myParams.myBankSize / bankHeight, bankHeight,
                               myParams.myBankCount, 1, 1, false);
  myParams.setAccessDataParams(myParams.myDataSize, 0);
  myParams.setDecayRate(50);

  myDataLayer.setVisibility(true);
  myReadLayer.setVisibility(true);
  myWriteLayer.setVisibility(true);
  myPcLayer.setVisibility(true);

  // Sibling widgets (same boss), not layout children: positioned in
  // setArea() below.  Only a zoomable region scrolls
  if(myIsZoomable)
  {
    myVScrollBar = new ScrollBarWidget(boss, font);
    myVScrollBar->setTarget(this);
    myHScrollBar = new ScrollBarHWidget(boss, font);
    myHScrollBar->setTarget(this);
  }

  // Create context menu for commands
  VariantList l;
  VarList::push_back(l, "Toggle breakpoint", "bp");
  VarList::push_back(l, "Toggle R/W trap", "rwt");
  VarList::push_back(l, "Toggle read trap", "rt");
  VarList::push_back(l, "Toggle write trap", "wt");
  VarList::push_back(l, "Show totals", "st");
#ifdef IMAGE_SUPPORT
  VarList::push_back(l, "Save data picture", "pic");
#endif
  myMenu = std::make_unique<ContextMenu>(this, font, l);

  // Resize data array
  myCurrentData.resize(myParams.myDataSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWidget::isDisplayable(uInt16 bankSize, uInt16 bankCount, uInt16 bankHeight)
{
  // A single bank only has to divide into rows; several banks must suit every
  // selectable bank height
  return (bankSize * bankCount) > 0 &&
         (bankSize % bankHeight) == 0 &&
         (bankCount == 1 || (bankSize % MemViewWindowDialog::MAX_BANK_HEIGHT) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::loadConfig()
{
  setDirtyData();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setArea(int x, int y, int w, int h)
{
  // _w/_h exclude the scrollbars' own space, so this widget's background-clear
  // (Widget::draw(), via ClearBG) never wipes the scrollbars' pixels
  const int scrollBarW = myIsZoomable ? ScrollBarWidget::scrollBarWidth(_font) : 0;
  const int scrollBarH = myIsZoomable ? ScrollBarHWidget::scrollBarHeight(_font) : 0;
  Widget::setArea(x, y, w - scrollBarW, h - scrollBarH);

  // A live drag can transiently report an area smaller than the border
  // itself; floor at 1 so this never asks FBSurface for a non-positive size
  myParams.mySurfaceWidth = std::max(1, _w - 2 * BORDER);
  myParams.mySurfaceHeight = std::max(1, _h - 2 * BORDER);

  // Live-resizable, so re-fit to the assigned area on every resize, at the
  // same fixed params naturalSize() used to arrive at that area
  if(!myIsZoomable)
  {
    bool singleRow = true;
    int hBanks = 0, vBanks = 0, minZoomLevel = 1;
    const int bankWidth = myParams.myBankWidth;
    const int bankHeight = myParams.myBankHeight;
    findBestLayout(myParams.mySurfaceWidth, myParams.mySurfaceHeight,
      bankWidth, bankHeight, false, singleRow, hBanks, vBanks, minZoomLevel);
    myParams.setLayoutParameters(bankWidth, bankHeight, hBanks, vBanks, minZoomLevel, false);
  }

  // mySurfacePosX/Y need dialog().surface(), not valid yet here; set in drawWidget()
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

  bool singleRow = true;
  int hBanks = 0, vBanks = 0, minZoomLevel = 1;
  const Common::Size size = findBestLayout(myMaxCandidateW, myMaxCandidateH,
    myParams.myBankWidth, myParams.myBankHeight, false, singleRow, hBanks, vBanks, minZoomLevel);

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
  auto& fb = instance().frameBuffer();

  if(myDataLayer.mySurface)
  {
    fb.deallocateSurface(dialog().window(), myDataLayer.mySurface);
    myDataLayer.mySurface = nullptr;
  }
  if(myReadLayer.mySurface)
  {
    fb.deallocateSurface(dialog().window(), myReadLayer.mySurface);
    myReadLayer.mySurface = nullptr;
  }
  if(myWriteLayer.mySurface)
  {
    fb.deallocateSurface(dialog().window(), myWriteLayer.mySurface);
    myWriteLayer.mySurface = nullptr;
  }
  if(myPcLayer.mySurface)
  {
    fb.deallocateSurface(dialog().window(), myPcLayer.mySurface);
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
// Odd bank counts can only be displayed in single row mode
bool MemViewWidget::lockedSingleRow() const
{
  return (U32(myParams.myBankCount) & 1U) != 0;
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
Common::Size MemViewWidget::findBestLayout(int availableWidth, int availableHeight,
                                           int bankWidth, int bankHeight, bool separators,
                                           bool& singleRow, int& hBanks, int& vBanks,
                                           int& minZoomLevel) const
{
  if(lockedSingleRow())
    singleRow = true;

  bool fits = false;
  Common::Size size;
  hBanks = myParams.myBankCount;
  vBanks = 1;
  minZoomLevel = 1;

  const int w = availableWidth;
  const int h = availableHeight;

  if(singleRow)
  {
    // The easy case: only one bank row
    size = calcSizeAndZoom(w, h, bankWidth, bankHeight, hBanks, vBanks, separators, minZoomLevel);
    fits = (size.w <= U32(w)) && (size.h <= U32(h));
  }
  else
  {
    const double destAspect = DBL(w) / DBL(h);
    int testZoomLevel = 1;

    size = calcSizeAndZoom(w, h, bankWidth, bankHeight, hBanks, vBanks, separators, minZoomLevel);
    fits = (size.w <= U32(w)) && (size.h <= U32(h));

    double aspectDiff = std::fabs(
      DBL(size.w) / DBL(size.h) - destAspect);

    int testVBanks = vBanks;
    int testHBanks = hBanks;

    do
    {
      testVBanks *= 2;
      testHBanks /= 2;

      if(!testHBanks)
        break;

      const Common::Size testSize = calcSizeAndZoom(w, h, bankWidth, bankHeight,
        testHBanks, testVBanks, separators, testZoomLevel);

      const bool testFits = (testSize.w <= U32(w)) && (testSize.h <= U32(h));
      const double testDiff = std::fabs(
        DBL(testSize.w) / DBL(testSize.h) - destAspect);

      if((!fits || testFits) &&
         ((!fits && testFits) || (testZoomLevel > minZoomLevel) || (testDiff < aspectDiff)))
      {
        // Take new best option
        size = testSize;
        vBanks = testVBanks;
        hBanks = testHBanks;
        minZoomLevel = testZoomLevel;
        aspectDiff = testDiff;
        fits = testFits;
      }
    } while(testHBanks != 1);
  }

  // At least one condition must be met
  assert(fits || myIsZoomable);

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Never called for a non-zoomable region (RAM): its layout is fixed at
// construction and the settings panel never touches it
void MemViewWidget::setLayoutParameters(bool singleRow, bool separators, int bankHeight)
{
  if(!isSetup())
    return;

  const int bankWidth = myParams.myBankSize / bankHeight;

  int hBanks = 0, vBanks = 0, minZoomLevel = 1;
  findBestLayout(myParams.mySurfaceWidth, myParams.mySurfaceHeight,
    bankWidth, bankHeight, separators, singleRow, hBanks, vBanks, minZoomLevel);

  myParams.setLayoutParameters(bankWidth, bankHeight, hBanks, vBanks, minZoomLevel, separators);

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
        int bank = 0;
        uInt16 address = myParams.getAddress(myRightClickX, myRightClickY, &bank);

        Debugger& debugger = instance().debugger();
        const bool wasLocked = debugger.systemIsLocked();
        if (!wasLocked)
          debugger.lockSystem();

        // Build debugger command
        string command;
        if (rmb == "bp")
          command = std::format("break ${:X} {}", address, bank);
        else if (rmb == "rwt")
          command = std::format("trap ${:X}", address);
        else if (rmb == "rt")
          command = std::format("trapRead ${:X}", address);
        else if (rmb == "wt")
          command = std::format("trapWrite ${:X}", address);
        else
          assert(false);

        // Run
        const string message = debugger.parser().run(command);
        if (!wasLocked)
          debugger.unlockSystem();
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
  myMouseMarker.set(false);
  setDirty(false);
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
    myMouseMarker.set(true, byteOffset);
    setDirty(false);
  }
  else
  {
    myMouseMarker.set(false);
    setDirty(false);
  }
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
  uInt32* destReadLayer = myReadLayer.myFields.data();
  uInt32* destWriteLayer = myWriteLayer.myFields.data();
  uInt32* destPcLayer = myPcLayer.myFields.data();

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

  setDirty(false);
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

  const Common::Rect& s_dst = s.dstRect();
  const Int32 dpi = instance().frameBuffer().hidpiScaleFactor(dialog().window());
  myParams.mySurfacePosX = s_dst.x() + (_x + BORDER) * dpi;
  myParams.mySurfacePosY = s_dst.y() + (_y + BORDER) * dpi;

  // Let the layers draw again if necessary
  if (myDataIsDirty) {
    myDataLayer.draw();
  }
  if (isDirty())
  {
    myReadLayer.draw();
    myWriteLayer.draw();
    myPcLayer.draw();
  }

  clearEverythingDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setDirtyData()
{
  myDataIsDirty = true;
  setDirty(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::clearEverythingDirty()
{
  clearDirty();
  myDataIsDirty = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::render()
{
  if (!isSetup())
    return;

  // Render layers bottom to top
  myDataLayer.render();
  myReadLayer.render();
  myWriteLayer.render();
  myPcLayer.render();
  myPcMarker.render();
  myMouseMarker.render();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWidget::setDirty(bool renderGui)
{
  if (renderGui)
    setDirty();
  else
    // Don't tell parents about my dirtyness
    // to prevent the GUI redraw everything
    _dirty = true;
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

  const bool debuggerActive = (instance().eventHandler().state() == EventHandlerState::DEBUGGER);

  // Update ToolTip (even if the mouse didn't move)
  if (!debuggerActive)
    dialog().tooltip().refresh(this);

  // Care about PC counter marker
  if (debuggerActive)
  {
    // Check if the PC is in our address range at all
    const uInt16 pc = instance().debugger().cpuDebug().pc();
    const uInt16 pc13 = U32(pc) & ((1U << 13U) - 1U);
    const uInt16 base13 = U32(myParams.myBaseAddress) & ((1U << 13U) - 1U);

    if (
      (pc13 >= base13)
      &&
      (pc13 < (base13 + myParams.myBankSize))
    )
    {
      // PC is within our range
      const int bank = (U32(base13) & 0x1000U) ? instance().debugger().cartDebug().getPCBank() : 0;
      const int offset = bank * myParams.myBankSize + pc13 - base13;
      const int byteOffset = myParams.getRearrangedOffset(offset);

      if (myPcMarker.set(true, byteOffset))
        setDirty(true);

      return;
    }
  }

  // PC marker not active
  if (myPcMarker.set(false))
    setDirty(debuggerActive);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string MemViewWidget::getToolTip(const Common::Point& pos) const
{
  if (!extPosInData(pos))
    return "";

  const Common::Point internalPos = extPosConv(pos);
  int bank = 0;
  unsigned int offset = 0;
  uInt16 address = myParams.getAddress(internalPos.x, internalPos.y, &bank, &offset);
  uInt16 mirrorAddress = address + myMirrorAddrOffset;

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
  if ((myParams.myBaseAddress + myParams.myDataSize) <= 0x100)
  {
    // 2-digit address
    text += std::format("\n${:0>2X}", address);
#if 0
    if (mirrorAddress != address)
      text += std::format("/${:0>2X}", mirrorAddress);
#endif
    text += " [" + myTypeText + "]";
  }
  else
  {
    // 4-digit address
    text += std::format("\n${:0>4X}", address);
    if (mirrorAddress != address)
      text += std::format("/${:0>4X}", mirrorAddress);
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
  const CartDebug& cartDebug = instance().debugger().cartDebug();
  string label = cartDebug.getLabel(address, true);
  if (!label.empty())
    text += "\n" + label;

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
