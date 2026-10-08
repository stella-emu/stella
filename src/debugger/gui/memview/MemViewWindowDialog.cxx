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
#include <cstdlib>
#include <utility>

#include "Widget.hxx"
#include "PopUpWidget.hxx"
#include "ColorWidget.hxx"
#include "Layout.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "System.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Cart.hxx"
#include "Logger.hxx"
#include "MemViewWidget.hxx"
#include "MemViewWindow.hxx"
#include "MemViewWindowDialog.hxx"

// The dialog's border, inside which the memory grid sits
static constexpr int BORDER = 2;

/*
  Cartridge support status:

  03E0      Breakpoints not working (Montezuma's Revenge) (Cartridge::bankOrigin), Stepping through code shows false reads at ROM end.
  0FA0      OK
  2K        OK
  3E        Breakpoints not working, no access data for cartridge RAM (Cartridge::bankOrigin!)
  3E+       Breakpoints not working, no access data for cartridge RAM (Cartridge::bankOrigin!)
  3EX       ?
  3F        Breakpoints not working for big ROMs (Bad Apple)
  4A50      No access data for internal RAM, program ROM only 4K, cart RAM not exposed
  4K        OK
  4KSC      OK
  0840      OK
  AR        Unsure - what is RAM/ROM? No access data
  BF        OK
  BFSC      OK
  BUS       Cart RAM without content, no access data for cart RAM, no access data for internal RAM 
  CDF       Cart RAM without content, no access data for cart RAM, no access data for internal RAM
  CDFJ+     Cart RAM without content, no access data for cart RAM, no access data for internal RAM
  CM        No access data for internal RAM
  CTY       Cart RAM not exposed
  CV        OK
  DevCard   ?
  DF        OK
  DFSC      OK
  DPC       OK
  DPC+      Cart RAM without content, PC in cart RAM (false addresses)
  E0        Breakpoints not always working
  E7        Cart RAM not exposed, Stepping through code shows false reads at ROM end.
  EF/EFF    Breakpoints not always working (invalid bank)
  EFSC      OK
  ELF       Only 4K of ROM(?) exposed
  F0        OK
  F4        OK
  F4SC      OK
  F6        OK
  F6SC      OK
  F8        OK
  F8SC      OK
  FA        OK
  FA2       OK
  FC        OK
  FE        OK
  GL        OK
  JANE      ?
  MDM       OK
  MVC       Unsupported
  SB        Breakpoints not always working (invalid bank)
  TVBoy     OK
  UA        OK
  WD        OK
  WF8       OK
  X07       No access data for internal RAM
*/

namespace {
  // One third of color a, two thirds of color b, per RGB channel
  constexpr uInt32 colorMix33(uInt32 a, uInt32 b)
  {
    return ((((a & 0xFF0000U) / 3) + ((b & 0xFF0000U) * 2 / 3)) & 0xFF0000U) |
           ((((a & 0xFF00U) / 3) + ((b & 0xFF00U) * 2 / 3)) & 0xFF00U) |
           ((((a & 0xFFU) / 3) + ((b & 0xFFU) * 2 / 3)) & 0xFFU);
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewWindowDialog::MemViewWindowDialog(OSystem& osystem, DialogContainer& parent,
                                         int w, int h)
  : Dialog(osystem, parent, w, h)
{
  const GUI::Font& font = _font;
  WidgetArray wid;

  // Build middle value between high and mid colors for the three ColorWidgets
  static constexpr uInt32 readWidgetColor = colorMix33(READ_COLOR_HIGH, READ_COLOR_MID) | 0xFF000000U;
  static constexpr uInt32 writeWidgetColor = colorMix33(WRITE_COLOR_HIGH, WRITE_COLOR_MID) | 0xFF000000U;
  static constexpr uInt32 pcWidgetColor = colorMix33(PC_COLOR_HIGH, PC_COLOR_MID) | 0xFF000000U;
  // (The alpha values set to full are needed for Direct3D 11, which sometimes seems
  // to ignore the blend setting of the surface)

  // Calculate our color tables
  calcColorDataTab(READ_COLOR_HIGH, READ_COLOR_MID, ROM_ALPHA_MAX, myRomReadColorTab);
  calcColorDataTab(WRITE_COLOR_HIGH, WRITE_COLOR_MID, ROM_ALPHA_MAX, myRomWriteColorTab);
  calcColorDataTab(PC_COLOR_HIGH, PC_COLOR_MID, ROM_ALPHA_MAX, myRomPcColorTab);
  calcColorDataTab(READ_COLOR_HIGH, READ_COLOR_MID, RAM_ALPHA_MAX, myRamReadColorTab);
  calcColorDataTab(WRITE_COLOR_HIGH, WRITE_COLOR_MID, RAM_ALPHA_MAX, myRamWriteColorTab);
  calcColorDataTab(PC_COLOR_HIGH, PC_COLOR_MID, RAM_ALPHA_MAX, myRamPcColorTab);

  // Initial cartridge evaluation and basic setup (incl. small RAM views)
  cartEvaluation();

  // Add settings controls
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  mySingleRow = new CheckboxWidget(this, font, TEXT_SINGLE_ROW, Cmd::SingleRowChanged);
  wid.push_back(mySingleRow);
  mySeparators = new CheckboxWidget(this, font, TEXT_SEPARATORS, Cmd::SeparatorsChanged);
  wid.push_back(mySeparators);
  myInverted = new CheckboxWidget(this, font, TEXT_INVERTED, Cmd::InvertedChanged);
  wid.push_back(myInverted);
  myByteFade = new CheckboxWidget(this, font, TEXT_BYTE_FADE, Cmd::ByteFadeChanged);
  wid.push_back(myByteFade);

  myShowData = new CheckboxWidget(this, font, TEXT_SHOW_DATA, Cmd::ShowDataChanged);
  wid.push_back(myShowData);

  myShowPc = new CheckboxWidget(this, font, TEXT_SHOW_PC, Cmd::ShowPcChanged);
  wid.push_back(myShowPc);
  myPcColor = new ColorWidget(this, font);
  myPcColor->setColorRgb(pcWidgetColor);

  myShowReads = new CheckboxWidget(this, font, TEXT_SHOW_READS, Cmd::ShowReadsChanged);
  wid.push_back(myShowReads);
  myReadColor = new ColorWidget(this, font);
  myReadColor->setColorRgb(readWidgetColor);

  myShowWrites = new CheckboxWidget(this, font, TEXT_SHOW_WRITES, Cmd::ShowWritesChanged);
  wid.push_back(myShowWrites);
  myWriteColor = new ColorWidget(this, font);
  myWriteColor->setColorRgb(writeWidgetColor);

  myDecayLbl = new LabelWidget(this, font, TEXT_DECAY_RATE);
  myDecaySlider = new SliderWidget(this, font, 10, Cmd::DecayRateChanged, 4, "%");
  myDecaySlider->setMinValue(0);
  myDecaySlider->setMaxValue(100);
  myDecaySlider->setStepValue(1);
  wid.push_back(myDecaySlider);

  myClearButton = new ButtonWidget(this, font, TEXT_CLEAR, Cmd::ClearPressed);
  wid.push_back(myClearButton);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // Setup rest
  addToFocusList(wid);

  myLastCycles = instance().console().system().cycles();
  myLastFrames = instance().console().tia().frameCount();
  myLastHadFrameWrap = false;

  // Add a callback for rendering our extra surfaces at the correct time
  // (after the base GUI surface has been rendered and before any overlaying
  // ContextMenus or similar)
  addRenderCallback(
    [this]
    {
      for (const auto &[scope, view] : myViews)
        view->render();
    }
  );

  MemViewWindowDialog::layout();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::cartEvaluation()
{
  const GUI::Font& font = _font;
  const Cartridge& cart = instance().console().cartridge();

  // Determine ROM parameters
  const ByteSpan programRomContent = cart.getImage(Cartridge::ImageScope::PROGRAM);
  const uInt32 programRomSize = U32(programRomContent.size());
  uInt16 romBankSize = cart.bankSize();
  uInt16 romBankCount = cart.romBankCount();
  uInt32 romSize = romBankSize * romBankCount;

  Logger::debug(std::format("ROM size        = {}", programRomSize));
  Logger::debug(std::format("Calced ROM size = {} ({} x {})", romSize, romBankCount, romBankSize));

  // Check if the calculated size is bigger than the delivered one
  if (romSize > programRomSize)
  {
    if (programRomSize < romBankSize)
    {
      // Special case (e.g. for small CV ROMs)
      romBankCount = 1;
      romBankSize = U16(programRomSize);
      romSize = programRomSize;
    }
    else
    {
      // For now: cut down size to full banks
      romBankCount = U16(programRomSize / romBankSize);
      romSize = romBankSize * romBankCount;
    }
    Logger::debug(std::format("Corrected size  = {} ({} x {})", romSize, romBankCount, romBankSize));
  }

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Place RAM view
  myRamView = new MemViewWidget(this, font, RAM_SIZE, 1, RAM_SIZE, false,
    myRamReadColorTab, myRamWriteColorTab, myRamPcColorTab,
    MemViewDataLayer::RAM_DATA_COLOR_DEFAULT, MemViewDataLayer::RAM_DATA_COLOR_FADED,
    TEXT_RAM, RAM_BASE);
  myViews.insert({Cartridge::ImageScope::NONE, myRamView});
  myRamLbl = new LabelWidget(this, font, TEXT_RAM);

  // Configure RAM view
  myRamView->setAccessDataParams(M6532::getRamCounterSize(), M6532::getRamCounterOffset());

  // Setup cartridge's RAM part (if any)
  const uInt32 cartRamSize = cart.internalRamSize();
  myCartRamSize = cartRamSize;
  const uInt16 cartRamBankCount = cart.ramBankCount();
  Logger::debug(std::format("RAM bank count = {}", cartRamBankCount));
  Logger::debug(std::format("Int RAM size   = {}", cartRamSize));

  // Does the cartridge have internal RAM to be displayed?
  if (cartRamSize > 0)
  {
    if (cartRamSize <= 256)
    {
      // Smaller cartridge RAM will be shown next to the RIOT's RAM
      myCartRamView = new MemViewWidget(this, font, U16(cartRamSize), 1, RAM_SIZE, false,
        myRamReadColorTab, myRamWriteColorTab, myRamPcColorTab,
        MemViewDataLayer::RAM_DATA_COLOR_DEFAULT, MemViewDataLayer::RAM_DATA_COLOR_FADED,
        "Cart RAM", MemViewWidget::QUERY_RAM_BANK_ORIGIN
      );
      myViews.insert({Cartridge::ImageScope::NONE, myCartRamView});

      myCartRamView->setAccessDataParams(cart.getRamCounterSize(), Cartridge::getRamCounterOffset());
    }
    else
    {
      // Big RAM is placed in the big GUI area before the ROM
      myBigCartRam = true;
      myCartRamLbl = new LabelWidget(this, font, TEXT_RAM);

      // Put entry into main area map (view will be instantiated later)
      const uInt16 bankCount = std::max((uInt16)1, cartRamBankCount);
      const uInt16 bankSize = U16(cartRamSize / bankCount);
      myMainAreaScopes[Cartridge::ImageScope::NONE] =
        MainAreaScope(bankSize, bankCount, readBankHeightConfig(bankSize));
    }
  }

  // Setup cartridge's ROM part
  myRomLbl = new LabelWidget(this, font, TEXT_ROM);

  // Put main ROM view into map
  myMainAreaScopes[Cartridge::ImageScope::PROGRAM] =
    MainAreaScope(romBankSize, romBankCount, readBankHeightConfig(romBankSize));

  // Go through additional image scopes to build a map
  // of scopes needed at the main area
  for (
    Cartridge::ImageScope scope = extraScopeFirst;
    scope <= extraScopeLast;
    scope = Cartridge::ImageScope(std::to_underlying(scope) + 1)
  )
  {
    uInt16 extraBytes = U16(cart.getImage(scope).size());
    if (extraBytes > 0)
      myMainAreaScopes[scope] = MainAreaScope(extraBytes, 1, 0);
  }
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::createViews()
{
  const GUI::Font& font = _font;
  const Cartridge& cart = instance().console().cartridge();

  bool displayable = true;
  bool singleRow = true;
  bool singleBank = true;

  // Instantiate and place all main area views
  for (const auto &[scope, entry] : myMainAreaScopes)
  {
    MemViewWidget* newView = nullptr;

    switch (scope)
    {
      case Cartridge::ImageScope::NONE:
      {
        // This is the cartridge RAM
        myCartRamView = newView = new MemViewWidget(this, font,
          entry.myBankSize, entry.myBankCount, entry.myBankHeight, true,
          myRamReadColorTab, myRamWriteColorTab, myRamPcColorTab,
          MemViewDataLayer::DATA_COLOR_DEFAULT, MemViewDataLayer::DATA_COLOR_FADED,
          "Cart RAM", MemViewWidget::QUERY_RAM_BANK_ORIGIN
        );
        myCartRamView->setAccessDataParams(cart.getRamCounterSize(), cart.getRamCounterOffset());
        break;
      }

      case Cartridge::ImageScope::PROGRAM:
      {
        // Create program ROM view
        newView = new MemViewWidget(this, font,
          entry.myBankSize, entry.myBankCount, entry.myBankHeight, true,
          myRomReadColorTab, myRomWriteColorTab, myRomPcColorTab,
          MemViewDataLayer::DATA_COLOR_DEFAULT, MemViewDataLayer::DATA_COLOR_FADED, TEXT_ROM,
          cart.getRomScopeOffset(scope) | MemViewWidget::QUERY_ROM_BANK_ORIGIN
        );
        break;
      }

      case Cartridge::ImageScope::DISPLAY_DATA:
      {
        // Create display data ROM view
        newView = new MemViewWidget(this, font,
          entry.myBankSize, entry.myBankCount, entry.myBankHeight, true,
          myRomReadColorTab, myRomWriteColorTab, myRomPcColorTab,
          MemViewDataLayer::DATA_COLOR_DEFAULT, MemViewDataLayer::DATA_COLOR_FADED, "Display data",
          cart.getRomScopeOffset(scope) | MemViewWidget::QUERY_ROM_BANK_ORIGIN
        );
        break;
      }

      default:
        cerr << "Maybe you forgot something here?\n";
        continue;
    }

    if (newView != nullptr)
    {
      displayable = displayable && newView->isDisplayable();
      singleRow = singleRow && newView->lockedSingleRow();
      singleBank = singleBank && newView->lockedSingleBank();

      if (newView != myCartRamView)
      {
        newView->setAccessDataParams(
          cart.getRomCounterSize(scope),
          cart.getRomCounterOffset(scope)
        );
      }
      myViews.insert({scope, newView});
    }
  }

  // Check if ROM is supported and hide settings if unnecessary
  if (displayable)
  {
    if (singleRow)
    {
      // No single row selection if everything is already single row
      mySingleRow->setState(false);
      mySingleRow->setEnabled(false);
    }

    if (singleBank)
    {
      // No separators if one bank on all views only
      mySeparators->setState(false);
      mySeparators->setEnabled(false);
    }
  }
  else
  {
    // Setup error
    // Disable corresponding GUI elements
    mySingleRow->setState(false);
    mySingleRow->setEnabled(false);
    mySeparators->setState(false);
    mySeparators->setEnabled(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::calcViewSizes(int totalWidth, int totalHeight)
{
  const size_t areaCount = myMainAreaScopes.size();
  // Calculate the available size without all the scroll bars, borders and gaps
  const int netWidth = totalWidth - areaCount * MemViewWidget::getHFrameSize(_font, true);
  const int netHeight = totalHeight - MemViewWidget::getVFrameSize(_font, true);

  // Sum up total bytes and find largest scope
  uInt32 totalBytes = 0;
  Cartridge::ImageScope largestScope = Cartridge::ImageScope::PROGRAM;
  for (auto &[scope, entry] : myMainAreaScopes)
  {
    totalBytes += entry.myBytes;
    if (entry.myBytes > myMainAreaScopes[largestScope].myBytes)
      largestScope = scope;
    // Reset entry width
    entry.myWidth = 0;
  }

  // Pre-calculate all main area view sizes
  if (totalBytes > 0)
  {
    // First run for assigning raw calculated sizes
    for (auto &[scope, entry] : myMainAreaScopes)
    {
      double ratio = DBL(entry.myBytes) / DBL(totalBytes);

      int width = static_cast<int>(round(DBL(netWidth) * ratio));
      int clampedWidth = std::max(MIN_ROM_WIDTH, width);
      int deltaWidth = clampedWidth - width;
      // Subtract extra needs from the largest area
      if (deltaWidth && (scope != largestScope))
      {
        // Take the extra needed space from the largest one
        myMainAreaScopes[largestScope].myWidth -= deltaWidth;
        entry.myWidth += clampedWidth;
      }
      else
        entry.myWidth += width;
    }

    // When there is more than one view, maybe spread the unused space accross them
    if (areaCount > 1)
    {
      int deltaWidthCount = 0;
      int deltaWidthSum = 0;
      // Go through list again and ask the widgets how much space they would claim
      for (auto &[scope, entry] : myMainAreaScopes)
      {
        bool success = false;
        auto [size, layoutParams] = MemViewWidget::calcNeededSize(
          entry.myWidth, netHeight,
          true, entry.myBankSize, entry.myBankCount, entry.myBankHeight, success);
        // Sum up width deltas of assigned space vs. used space
        entry.mySize = size;
        if (entry.myWidth > MIN_ROM_WIDTH)
        {
          deltaWidthSum += entry.myWidth - size.w;
          deltaWidthCount++;
        }
      }
      if ((deltaWidthCount > 0) && (deltaWidthSum > deltaWidthCount))
      {
        const int eachDelta = deltaWidthSum / deltaWidthCount;
        for (auto &[scope, entry] : myMainAreaScopes)
        {
          if (entry.myWidth <= MIN_ROM_WIDTH)
            continue;
          deltaWidthSum -= eachDelta;
          // The last one gets the fractional rest
          entry.myWidth = entry.mySize.w + ((eachDelta <= deltaWidthSum) ? eachDelta : (deltaWidthSum + eachDelta));
        }
      }
    }
  }
  else
  {
    myMainAreaScopes[Cartridge::ImageScope::PROGRAM].myWidth = netWidth;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::updateViews()
{
  const Cartridge& cart = instance().console().cartridge();

  for (const auto &[scope, entry] : myMainAreaScopes)
  {
    switch (scope)
    {
      case Cartridge::ImageScope::PROGRAM:
      case Cartridge::ImageScope::DISPLAY_DATA:
      {
        const auto it = myViews.find(scope);
        if (it != myViews.end())
        {
          MemViewWidget* view = it->second;
          if (view != nullptr)
          {
            // Set current content
            view->updateData(ByteSpan(cart.getImage(scope).begin(),
              entry.myBankSize * entry.myBankCount));
          }
        }
        break;
      }
      default:
        break;
    }
  }

  // Let the layers copy the initial state of access counters
  updateAccessData();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::widgetItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const Common::Size& size = static_cast<const MemViewWindow&>(parent()).size();
  _w = I32(size.w);
  _h = I32(size.h);

  const int HBORDER = Dialog::hBorder(),
            VBORDER  = Dialog::vBorder(),
            VGAP     = Dialog::vGap();
  static constexpr int DIALOG_MARGIN_H = 14;
  static constexpr int DIALOG_MARGIN_V = 12;

  // Create views now that it is foreseeable they will be assigned a size
  if (myFirstLayout)
    createViews();

  const auto buildRoot = [&](int mainMaxW) {
    const auto swatchRow = [](CheckboxWidget* cb, ColorWidget* swatch) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      row->addAuto(anchoredItem(cb));
      row->addStretchSpace();
      row->addAuto(anchoredItem(swatch));
      return row;
    };

    auto settingsCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, 0, DIALOG_MARGIN_V);
    settingsCol->addAuto(anchoredItem(mySingleRow));
    settingsCol->addAuto(anchoredItem(mySeparators));
    settingsCol->addAuto(anchoredItem(myInverted));
    settingsCol->addAuto(anchoredItem(myByteFade));
    settingsCol->addSpace(VGAP);
    settingsCol->addAuto(anchoredItem(myShowData));
    settingsCol->addAuto(swatchRow(myShowPc, myPcColor));
    settingsCol->addAuto(swatchRow(myShowReads, myReadColor));
    settingsCol->addAuto(swatchRow(myShowWrites, myWriteColor));
    settingsCol->addSpace(VGAP);
    settingsCol->addAuto(labeledRow(myDecayLbl, myDecaySlider));
    settingsCol->addSpace(VGAP);
    settingsCol->addAuto(anchoredItem(myClearButton));

    const Common::Size settingsSize = settingsCol->naturalSize();
    const int settingsW = I32(settingsSize.w);

    // Declare RAM's own floor, or the row below holds nothing open
    const Common::Size ramMin = myRamView->minSize(settingsW);

    // RAM is non-zoomable: it shrink-fits within a candidate area (settings
    // column's width; whatever's left of the column's height once the
    // settings column takes its own share) rather than stretching to fill one.
    // Never offer less than RAM's own floor: the two are computed from
    // different border constants and drift apart by a few px, which is enough
    // to drop a whole zoom level and strand unused space below the grid
    const int ramMaxH = std::max(I32(ramMin.h),
      _h - 2 * VBORDER - I32(settingsSize.h) - VGAP);
    myRamView->setMaxCandidateSize(settingsW, ramMaxH);

    auto ramRow = std::make_unique<BoxLayout>(Dir::Horizontal, HBORDER);
    ramRow->addAuto(GUI::alignedItem(myRamView, GUI::HAlign::Left, GUI::VAlign::Top,
                                     I32(ramMin.w), I32(ramMin.h)));
    ramRow->addAuto(GUI::alignedItem(myRamLbl, GUI::HAlign::Left, GUI::VAlign::Top));

    // A small cartridge RAM sits beside RAM at the same zoom: each of its
    // (at most two) columns gets the candidate width RAM's one column does
    if(myCartRamView && !myBigCartRam)
    {
      const int columns = (myCartRamSize <= RAM_SIZE) ? 1 : 2;
      const Common::Size cartRamMin = myCartRamView->minSize(settingsW * columns);
      myCartRamView->setMaxCandidateSize(settingsW * columns, ramMaxH);
      ramRow->addAuto(GUI::alignedItem(myCartRamView, GUI::HAlign::Left, GUI::VAlign::Top,
                                       I32(cartRamMin.w),
                                       I32(cartRamMin.h)));
    }

    auto leftCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
    // Reserve the full candidate height so the settings column tracks a drag
    // continuously, rather than snapping when RAM's zoom level steps down
    leftCol->addFixed(std::move(ramRow), ramMaxH, 0);
    leftCol->addAuto(std::move(settingsCol));

    if (mainMaxW > 0)
    {
      // Calculate the best sizes of the main views
      const int mainMaxH = _h - 2 * DIALOG_MARGIN_V;
      calcViewSizes(mainMaxW, mainMaxH);
    }

    // The views right of the left column share whatever remains, in
    // proportion to their sizes; each asks for its share of mainMaxW.  A
    // stretch cell holds nothing open unless it declares a floor
    const auto addMainView = [&](BoxLayout& box, Cartridge::ImageScope scope,
                                 MemViewWidget* view)
    {
      const int& width = myMainAreaScopes[scope].myWidth;
      const Common::Size floor = (mainMaxW > 0)
        ? view->minSize(width) : Common::Size();
      box.addStretch(widgetItem(view, I32(floor.w), I32(floor.h)), width);
    };

    auto root = std::make_unique<BoxLayout>(Dir::Horizontal, HBORDER,
      DIALOG_MARGIN_H, DIALOG_MARGIN_V);
    root->addAuto(std::move(leftCol));
    if(myBigCartRam)
    {
      root->addAuto(GUI::alignedItem(myCartRamLbl, GUI::HAlign::Left, GUI::VAlign::Top));
      addMainView(*root, Cartridge::ImageScope::NONE, myCartRamView);
    }
    root->addAuto(GUI::alignedItem(myRomLbl, GUI::HAlign::Left, GUI::VAlign::Top));
    for(const auto& [scope, view]: myViews)
      if(scope != Cartridge::ImageScope::NONE)
        addMainView(*root, scope, view);
    return root;
  };

  // Keep the minimum usable on a small screen: the views right of the left
  // column ask for at most half the screen's width, and never more than
  // what's left beside everything else (with no floors, the tree reports what
  // everything else needs).  Their horizontal scrollbars cover any shortfall
  const Common::Size& desktop =
    instance().frameBuffer().desktopSize(BufferType::MemViewWindow);
  const int othersW = I32(buildRoot(0)->minSize().w);
  const int mainMaxW = I32(desktop.w) - othersW;
  auto root = buildRoot(mainMaxW);

  myMinSize = root->minSize();
  root->doLayout(0, 0, _w, _h);

  // After layouting (and hence also the arrangement setup within the views)
  // is finished, we can now update the views with the initial data to
  // be displayed
  updateViews();
  myFirstLayout = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::updateVisualParameters()
{
  for (const auto &[scope, view] : myViews)
  {
    view->setVisualParameters(
      myShowData->getState(),
      myShowPc->getState(),
      myShowReads->getState(),
      ((view == myRamView) || (view == myCartRamView))
        ? myShowWrites->getState() : false,
      myInverted->getState(),
      myByteFade->getState()
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::updateLayoutParameters()
{
  for (const auto &[scope, view] : myViews)
  {
    if (
      (view == myRamView)
      ||
      ((view == myCartRamView) && !myBigCartRam)
    )
      continue;

    view->setLayoutParameters(
      mySingleRow->getState(),
      mySeparators->getState()
    );
  }

  // The bank arrangement may have changed, and with it the width ROM needs
  static_cast<MemViewWindow&>(parent()).updateMinSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Persists each control's state under a "memview.*" settings key
void MemViewWindowDialog::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                        int data, int id)
{
  Settings& settings = instance().settings();

  switch(cmd)
  {
    case Cmd::SingleRowChanged:
      settings.setValue("memview.singlerow", mySingleRow->getState());
      break;
    case Cmd::SeparatorsChanged:
      settings.setValue("memview.separators", mySeparators->getState());
      break;
    case Cmd::InvertedChanged:
      settings.setValue("memview.inverted", myInverted->getState());
      break;
    case Cmd::ByteFadeChanged:
      settings.setValue("memview.bytefade", myByteFade->getState());
      break;
    case Cmd::ShowDataChanged:
      settings.setValue("memview.showdata", myShowData->getState());
      break;
    case Cmd::ShowPcChanged:
      settings.setValue("memview.showpc", myShowPc->getState());
      break;
    case Cmd::ShowReadsChanged:
      settings.setValue("memview.showreads", myShowReads->getState());
      break;
    case Cmd::ShowWritesChanged:
      settings.setValue("memview.showwrites", myShowWrites->getState());
      break;
    case Cmd::DecayRateChanged:
      settings.setValue("memview.decayrate", myDecaySlider->getValue());
      break;
    default:
      break;
  }

  switch(cmd)
  {
    case Cmd::InvertedChanged:
    case Cmd::ByteFadeChanged:
    case Cmd::ShowDataChanged:
    case Cmd::ShowPcChanged:
    case Cmd::ShowReadsChanged:
    case Cmd::ShowWritesChanged:
      updateVisualParameters();
      break;

    case Cmd::SingleRowChanged:
    case Cmd::SeparatorsChanged:
      updateLayoutParameters();
      break;

    case Cmd::DecayRateChanged:
      for (const auto &[scope, view] : myViews)
        view->setDecayRate(myDecaySlider->getValue());
      break;

    case Cmd::ClearPressed:
      for (const auto &[scope, view] : myViews)
        view->clearHeatmaps();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, id);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::loadConfig()
{
  // Called on open and on every expose, so the settings read is one-time:
  // reapplying every frame would fight a live mouse-wheel zoom on ROM
  if(!mySettingsLoaded)
  {
    mySettingsLoaded = true;
    const Settings& settings = instance().settings();

    if (mySingleRow->isEnabled())
      mySingleRow->setState(settings.getBool("memview.singlerow"));
    myInverted->setState(settings.getBool("memview.inverted"));
    myByteFade->setState(settings.getBool("memview.bytefade"));
    if (mySeparators->isEnabled())
      mySeparators->setState(settings.getBool("memview.separators"));
    myShowData->setState(settings.getBool("memview.showdata"));
    myShowPc->setState(settings.getBool("memview.showpc"));
    myShowReads->setState(settings.getBool("memview.showreads"));
    myShowWrites->setState(settings.getBool("memview.showwrites"));
    myDecaySlider->setValue(settings.getInt("memview.decayrate"));

    updateLayoutParameters();
    updateVisualParameters();
    // The slider only reports a change, so a saved rate equal to its
    // starting value would never reach the views
    for (const auto &[scope, view] : myViews)
      view->setDecayRate(myDecaySlider->getValue());
  }

  for (const auto &[scope, view] : myViews)
    view->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 MemViewWindowDialog::readBankHeightConfig(uInt16 bankSize) const
{
  const Settings& settings = instance().settings();
  uInt16 bankHeight = 0;

  switch (bankSize)
  {
    case 1024:
      bankHeight = U16(settings.getInt("memview.bh1k"));
      break;
    case 2048:
      bankHeight = U16(settings.getInt("memview.bh2k"));
      break;
    case 4096:
      bankHeight = U16(settings.getInt("memview.bh4k"));
      break;
    default:
      break;
  }

  if (bankHeight <= 0)
    return 0;
  else if (bankHeight <= 64)
    return 64;
  else if (bankHeight <= 128)
    return 128;
  else if (bankHeight <= 256)
    return 256;
  else
    return 512;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::updateAccessData(uInt32 cyclesDiff, int elapsedFrames)
{
  // Update RAM data
  M6532& riot = instance().console().riot();
  Cartridge &cart = instance().console().cartridge();

  // Update RAM data and accesses
  myRamView->updateData(riot.getRAM());
  myRamView->updateAccessData(
    riot.getRamDataPeekCounter(),
    riot.getRamPokeCounter(),
    riot.getRamCodePeekCounter(),
    cyclesDiff,
    elapsedFrames
  );

  // Update cartridge RAM data and accesses
  if (myCartRamView)
  {
    myCartRamView->updateData(cart.getRAM());
    myCartRamView->updateAccessData(
      cart.getRamDataPeekCounter(),
      cart.getRamPokeCounter(),
      cart.getRamCodePeekCounter(),
      cyclesDiff,
      elapsedFrames
    );
  }

  // Update ROM views
  for (const auto &[scope, view] : myViews)
  {
    if (scope == Cartridge::ImageScope::NONE)
      // These are the RAM views already handled above
      continue;

    // Update ROM accesses
    view->updateAccessData(
      cart.getRomDataPeekCounter(scope),
      cart.getRomPokeCounter(scope),
      cart.getRomCodePeekCounter(scope),
      cyclesDiff,
      elapsedFrames
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewWindowDialog::tick()
{
  Dialog::tick();

  // Nothing to read without a console
  if(!instance().hasConsole())
    return;

  // Get elapsed cycles
  const uInt64 currentCycles = instance().console().system().cycles();
  uInt64 cyclesDiff = 0;

  // TODO: the uInt64 wrap-around won't work like this
  // but will anyone of us will be still around when this happens?
  if (currentCycles > myLastCycles)
    cyclesDiff = currentCycles - myLastCycles;
  myLastCycles = currentCycles;

  if ((cyclesDiff != 0) && (cyclesDiff <= UINT32_MAX))
  {
    // Evaluate frame counts
    const uInt32 currentFrames = instance().console().tia().frameCount();
    const bool hadFrameWrap = (currentFrames != myLastFrames);
    // We consider the time which has passed since the last call a full frame
    // if both this and the last call had different total frame counts
    const bool fullFrames = myLastHadFrameWrap && hadFrameWrap;
    const int elapsedFrames = fullFrames ? (currentFrames - myLastFrames) : 0;
    myLastFrames = currentFrames;
    myLastHadFrameWrap = hadFrameWrap;

    updateAccessData(U32(cyclesDiff), elapsedFrames);
  }

  // Update misc stuff
  for (const auto &[scope, view] : myViews)
    view->updateRest();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewWindowDialog::needsRedraw()
{
  const bool dirtyGui = isDirty() || isChainDirty();
  bool dirtyViews = false;
  for (const auto &[scope, view] : myViews)
    dirtyViews = dirtyViews || view->isDirty();

  return dirtyGui || dirtyViews;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculate color table as such:
// 0..127   rising alpha value of the colorMid
// 128      colorMid with full alpha (this is the color to be used when access counter diff == 1
// 129..255 color sweep from colorMid to colorHigh
void MemViewWindowDialog::calcColorDataTab(uInt32 colorHigh, uInt32 colorMid, const double alphaMax,
  MemViewWidget::ColorTab& tab)
{
  // Precalc color data table with alpha channel
  colorHigh &= 0xFFFFFFU;
  colorMid &= 0xFFFFFFU;

  static constexpr int middle = 128;

  const double rHigh = DBL((colorHigh >> 16U) & 0xFFU);
  const double gHigh = DBL((colorHigh >> 8U) & 0xFFU);
  const double bHigh = DBL(colorHigh & 0xFFU);
  const double rMid = DBL((colorMid >> 16U) & 0xFFU);
  const double gMid = DBL((colorMid >> 8U) & 0xFFU);
  const double bMid = DBL(colorMid & 0xFFU);
  const double steps = DBL(255 - middle);
  const double rStep = (rHigh - rMid) / steps;
  const double gStep = (gHigh - gMid) / steps;
  const double bStep = (bHigh - bMid) / steps;
  double r = rHigh;
  double g = gHigh;
  double b = bHigh;

  for (int i = 255; i >= middle; i--)
  {
    tab[i] = (U32(round(alphaMax)) << 24U) |
      (U32(round(r)) << 16U) |
      (U32(round(g)) << 8U) |
      U32(round(b));
    r -= rStep;
    g -= gStep;
    b -= bStep;
  }

  double alpha = alphaMax;
  const double alphaStep = alphaMax / DBL(middle);

  // Just go down with the alpha value
  for (int i = middle - 1; i >= 0; i--)
  {
    alpha -= alphaStep;
    tab[i] = (U32(round(alpha)) << 24U) | colorMid;
  }
}
