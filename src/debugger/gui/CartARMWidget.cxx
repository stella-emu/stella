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
#include "PopUpWidget.hxx"
#include "DataGridWidget.hxx"
#include "CartARMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARMWidget::CartridgeARMWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    CartridgeARM& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::createCycleWidgets()
{
  VariantList items;

  // Everything is created at a placeholder position; layoutContent() positions it
  myArmCyclesLabel = new StaticTextWidget(_boss, _font, "ARM emulation cycles:");
  myArmCyclesLabel->setToolTip("Cycle count enabled by developer settings.");

  myIncCycles = new CheckboxWidget(_boss, _font, "Increase 6507 cycles",
                                   kIncCyclesChanged);
  myIncCycles->setToolTip("Increase 6507 cycles with approximated ARM cycles.");
  myIncCycles->setTarget(this);

  myCycleFactor = new SliderWidget(_boss, _font, _fontWidth * 10,
                                   "Cycle factor", 0, kFactorChanged, _fontWidth * 4, "%");
  myCycleFactor->setMinValue(90); myCycleFactor->setMaxValue(110);
  myCycleFactor->setTickmarkIntervals(4);
  myCycleFactor->setToolTip("Correct approximated ARM cycles by factor.");
  myCycleFactor->setTarget(this);

  const auto addCounter = [&](DataGridWidget*& grid, string_view tip) {
    grid = new DataGridWidget(_boss, _font, 1, 1, 6, 32, Common::Base::Fmt::_10_6);
    grid->setEditable(false);
    grid->setToolTip(tip);
  };

  myCyclesLabel = new StaticTextWidget(_boss, _font, "Cycles #");
  addCounter(myPrevThumbCycles, "Approximated CPU cycles of last but one ARM run.\n");
  addCounter(myThumbCycles,     "Approximated CPU cycles of last ARM run.\n");

  myInstructionsLabel = new StaticTextWidget(_boss, _font, "Instructions #");
  addCounter(myPrevThumbInstructions, "Instructions of last but one ARM run.\n");
  addCounter(myThumbInstructions,     "Instructions of last ARM run.\n");

  VarList::push_back(items, "AUTO",                        static_cast<Int32>(Thumbulator::ChipType::AUTO));
  VarList::push_back(items, "LPC2101" + ELLIPSIS + "3",    static_cast<Int32>(Thumbulator::ChipType::LPC2101));
  VarList::push_back(items, "LPC2104" + ELLIPSIS + "6 OC", static_cast<Int32>(Thumbulator::ChipType::LPC2104_OC));
  VarList::push_back(items, "LPC2104" + ELLIPSIS + "6",    static_cast<Int32>(Thumbulator::ChipType::LPC2104));
  VarList::push_back(items, "LPC213x",                     static_cast<Int32>(Thumbulator::ChipType::LPC213x));
  myChipType = new PopUpWidget(_boss, _font, items, "Chip", 0, kChipChanged);
  myChipType->setToolTip("Select emulated ARM chip.");
  myChipType->setTarget(this);

  myLockMamMode = new CheckboxWidget(_boss, _font, "MAM Mode", kMamLockChanged);
  myLockMamMode->setToolTip("Check to lock Memory Accelerator Module (MAM) mode.");
  myLockMamMode->setTarget(this);

  items.clear();
  VarList::push_back(items, "Off (0)", static_cast<uInt32>(Thumbulator::MamModeType::mode0));
  VarList::push_back(items, "Partial (1)", static_cast<uInt32>(Thumbulator::MamModeType::mode1));
  VarList::push_back(items, "Full (2)", static_cast<uInt32>(Thumbulator::MamModeType::mode2));
  VarList::push_back(items, "1 Cycle (X)", static_cast<uInt32>(Thumbulator::MamModeType::modeX));
  myMamMode = new PopUpWidget(_boss, _font, items, "", 0, kMamModeChanged);
  myMamMode->setToolTip("Select emulated Memory Accelerator Module (MAM) mode.");
  myMamMode->setTarget(this);

  // define the tab order
  addFocusWidget(myIncCycles);
  addFocusWidget(myCycleFactor);
  addFocusWidget(myChipType);
  addFocusWidget(myLockMamMode);
  addFocusWidget(myMamMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::stretchedItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  // "Chip" and "Cycle factor" are drawn INSIDE their widgets, and the counter
  // captions are labels beside theirs: either way the clearance from what follows
  // comes from a group -- and each of these has nothing to line up with, so each
  // is a group of one
  GUI::alignLabels({{myChipType}});
  GUI::alignLabels({{myCycleFactor}});
  GUI::alignLabels({{myCyclesLabel}});
  GUI::alignLabels({{myInstructionsLabel}});

  // A pair of counters -- the previous run, then the last -- beside their label
  const auto counters = [&](StaticTextWidget* label, DataGridWidget* prev,
                            DataGridWidget* last) {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth / 2);
    row->addAuto(anchoredItem(label));
    row->addAuto(anchoredItem(prev));
    row->addAuto(anchoredItem(last));
    return row;
  };

  // The MAM lock, with the mode it locks beside it
  auto mam = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth);
  mam->addAuto(anchoredItem(myLockMamMode));
  mam->addStretch(stretchedItem(myMamMode));

  // Two columns, each as wide as the counters at its foot: the cycle figures on
  // the left, the instruction figures on the right, and the controls that drive
  // them above.  The pop-ups FILL their column, so each ends flush with the
  // counters below it -- which is what they were hand-measured against before
  auto grid = std::make_unique<GridLayout>(2, 3, _fontWidth * 2, VGAP);
  grid->columnAuto(0);
  grid->columnAuto(1);
  grid->rowAuto(0);
  grid->rowAuto(1);
  grid->rowAuto(2);

  grid->place(0, 0, anchoredItem(myIncCycles));
  grid->place(1, 0, anchoredItem(myCycleFactor));
  grid->place(0, 1, stretchedItem(myChipType));
  grid->place(1, 1, std::move(mam));
  grid->place(0, 2, counters(myCyclesLabel, myPrevThumbCycles, myThumbCycles));
  grid->place(1, 2, counters(myInstructionsLabel, myPrevThumbInstructions,
                             myThumbInstructions));

  // The block is indented under its heading
  auto block = std::make_unique<BoxLayout>(Dir::Horizontal);
  block->addSpace(_fontWidth * 2);
  block->addAuto(std::move(grid));

  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(myArmCyclesLabel));
  col.addAuto(std::move(block));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::saveOldState()
{
  myOldState.armPrevRun.clear();
  myOldState.armRun.clear();

  myOldState.mamMode = static_cast<uInt32>(myCart.mamMode());

  myOldState.armPrevRun.push_back(myCart.prevCycles());
  myOldState.armPrevRun.push_back(myCart.prevStats().instructions);

  myOldState.armRun.push_back(myCart.cycles());
  myOldState.armRun.push_back(myCart.stats().instructions);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::loadConfig()
{
  const bool devSettings = instance().settings().getBool("dev.settings");
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  myChipType->setSelectedIndex(static_cast<Int32>(instance().settings().getInt("dev.thumb.chiptype")
    - static_cast<int>(Thumbulator::ChipType::AUTO)));
  handleChipType();

  const bool isChanged = static_cast<uInt32>(myCart.mamMode()) != myOldState.mamMode;
  myMamMode->setSelectedIndex(static_cast<uInt32>(myCart.mamMode()), isChanged);
  myMamMode->setEnabled(devSettings && myLockMamMode->getState());
  myLockMamMode->setEnabled(devSettings);

  // ARM cycles
  myIncCycles->setState(instance().settings().getBool("dev.thumb.inccycles"));
  myCycleFactor->setValue(std::round(instance().settings().getFloat("dev.thumb.cyclefactor") * 100.F));
  handleArmCycles();

  const auto setSingle = [&](auto* widget, uInt32 cur, uInt32 old) {
    alist.clear(); vlist.clear(); changed.clear();
    alist.push_back(0);
    vlist.push_back(static_cast<int>(cur));
    changed.push_back(cur != old);
    widget->setList(alist, vlist, changed);
  };
  setSingle(myPrevThumbCycles, myCart.prevCycles(), myOldState.armPrevRun[0]);
  setSingle(myPrevThumbInstructions, myCart.prevStats().instructions, myOldState.armPrevRun[1]);
  setSingle(myThumbCycles, myCart.cycles(), myOldState.armRun[0]);
  setSingle(myThumbInstructions, myCart.stats().instructions, myOldState.armRun[1]);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  switch(cmd)
  {
    case kChipChanged:
      handleChipType();
      break;

    case kMamLockChanged:
      handleMamLock();
      break;

    case kMamModeChanged:
      handleMamMode();
      break;

    case kIncCyclesChanged:
    case kFactorChanged:
      handleArmCycles();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleChipType()
{
  const bool devSettings = instance().settings().getBool("dev.settings");

  myChipType->setEnabled(devSettings);

  if(devSettings)
  {
    instance().settings().setValue("dev.thumb.chiptype",
      myChipType->getSelectedTag().toInt());

    const Thumbulator::ChipPropsType chipProps =
      myCart.setChipType(static_cast<Thumbulator::ChipType>
      (myChipType->getSelectedTag().toInt()));

    // update tooltip with currently selected chip's properties
    const string tip = myChipType->getToolTip(Common::Point(0, 0)).substr(0, 25);
    myChipType->setToolTip(std::format(
      "{}\nCurrent: {}\n{} flash bank{}, {} MHz, {} wait states",
      tip,
      chipProps.name,
      chipProps.flashBanks,
      chipProps.flashBanks > 1 ? "s" : "",
      chipProps.MHz,
      chipProps.flashCycles - 1));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleMamLock()
{
  const bool checked = myLockMamMode->getState();

  myMamMode->setEnabled(checked);
  myCart.lockMamMode(checked);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleMamMode()
{
  // override MAM mode set by ROM
  const Int32 mode = myMamMode->getSelected();

  const string name = myMamMode->getSelectedName();
  myMamMode->setSelectedName(name + "XXX");

  instance().settings().setValue("dev.thumb.mammode", mode);
  myCart.setMamMode(static_cast<Thumbulator::MamModeType>(mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleArmCycles()
{
  const bool devSettings = instance().settings().getBool("dev.settings");
  const bool enable = myIncCycles->getState();
  const double factor = static_cast<double>(myCycleFactor->getValue()) / 100.0;

  if(devSettings)
  {
    instance().settings().setValue("dev.thumb.inccycles", enable);
    instance().settings().setValue("dev.thumb.cyclefactor", factor);
  }

  myIncCycles->setEnabled(devSettings);
  myCycleFactor->setEnabled(devSettings);
  myCyclesLabel->setEnabled(devSettings);
  myThumbCycles->setEnabled(devSettings);
  myPrevThumbCycles->setEnabled(devSettings);

  myCart.incCycles(devSettings && enable);
  myCart.cycleFactor(factor);
  myCart.enableCycleCount(devSettings);
}
