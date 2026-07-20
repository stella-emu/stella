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

#include "CartELF.hxx"
#include "BusTransactionQueue.hxx"
#include "DataGridWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "EditTextWidget.hxx"

#include "CartELFStateWidget.hxx"

namespace {
  string registerName(uInt8 reg)
  {
    switch(reg)
    {
      case 11:
        return "FP (R11) = ";

      case 12:
        return "IP (R12) = ";

      case 13:
        return "SP (R13) = ";

      case 14:
        return "LR (R14) = ";

      case 15:
        return "PC (R15) = ";

      default:
        return std::format("R{} = ", static_cast<int>(reg));
    }
  }

  string describeTransaction(uInt16 address, uInt16 mask, uInt64 timestamp)
  {
    return std::format("waiting for 0x{:04X} mask 0x{:04X} time {}",
      address, mask, timestamp);
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELFStateWidget::CartridgeELFStateWidget(GuiObject* boss,
                       const GUI::Font& lfont, const GUI::Font& nfont,
                       CartridgeELF& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart},
    myFlagValues(4)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFStateWidget::initialize()
{
  // Everything is created at a placeholder position; layoutContent() places it.
  // The row labels carry no padding of their own: they join the tab's label
  // column, and GUI::alignLabels() (in the skeleton) supplies the column
  myRegistersLabel = new StaticTextWidget(_boss, _font, 0, 0, "ARM registers:");

  myArmRegisters = new DataGridWidget(_boss, _font, 4, 4, 8, 8,
                                      Common::Base::Fmt::_16_8);
  myArmRegisters->setEditable(false);
  for(uInt8 i = 0; i < 16; i++) myArmRegisters->setToolTip(i % 4, i / 4, registerName(i));

  myFlagsLabel = new StaticTextWidget(_boss, _font, 0, 0, "ARM flags:");
  myFlags = new ToggleBitWidget(_boss, _font, 4, 1, 1);
  myFlags->setEditable(false);

  static constexpr std::array<string_view, 4> flagNames = {"N", "Z", "C", "V"};
  for(size_t i = 0; i < myFlagLabels.size(); ++i)
    myFlagLabels[i] = new StaticTextWidget(_boss, _font, 0, 0, flagNames[i]);

  // The readouts are as wide as the value they hold, in characters
  myTimeVcsLabel = new StaticTextWidget(_boss, _font, 0, 0, "Time VCS:");
  myCurrentCyclesVcs = new EditTextWidget(_boss, _font, 0, 0,
                                          16 * _font.getMaxCharWidth());
  myCurrentCyclesVcs->setEditable(false, true);

  myTimeArmLabel = new StaticTextWidget(_boss, _font, 0, 0, "Time ARM:");
  myCurrentCyclesArm = new EditTextWidget(_boss, _font, 0, 0,
                                          16 * _font.getMaxCharWidth());
  myCurrentCyclesArm->setEditable(false, true);

  myQueueSizeLabel = new StaticTextWidget(_boss, _font, 0, 0, "Bus queue size:");
  myQueueSize = new EditTextWidget(_boss, _font, 0, 0,
                                   4 * _font.getMaxCharWidth());
  myQueueSize->setEditable(false, true);

  myNextTransaction = new StaticTextWidget(_boss, _font, 0, 0,
                          describeTransaction(0xffff, 0xffff, ~0LLU));

  myLabelColumn.insert(myLabelColumn.end(),
                       {{myRegistersLabel}, {myFlagsLabel}, {myTimeVcsLabel},
                        {myTimeArmLabel}, {myQueueSizeLabel}});

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFStateWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::centeredItem;
  using GUI::labeledRow;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  // A label beside a multi-row grid sits on the grid's FIRST row, not on the
  // middle of the whole block, so the two share that row's baseline (CpuWidget)
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };
  const auto labelCell = [](StaticTextWidget* l) { return l->getWidth(); };

  // The 16 ARM registers, in a 4x4 grid beside its caption
  {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(onBaseline(myRegistersLabel), labelCell(myRegistersLabel));
    row->addAuto(onBaseline(myArmRegisters));
    col.addAuto(std::move(row));
  }

  // The four flags, each name centered over its own column of the toggle below
  // (the AudioWidget channel-header idiom), the caption beside the pair
  {
    auto names = std::make_unique<BoxLayout>(Dir::Horizontal);
    for(auto* l: myFlagLabels)
      names->addFixed(centeredItem(l), myFlags->colWidth());

    auto flags = std::make_unique<BoxLayout>(Dir::Vertical);
    flags->addAuto(std::move(names));
    flags->addAuto(anchoredItem(myFlags));

    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addFixed(alignedItem(myFlagsLabel, HAlign::Left, VAlign::Top),
                  labelCell(myFlagsLabel));
    row->addAuto(std::move(flags));
    col.addAuto(std::move(row));
  }

  // The cycle counts and the queue depth, each a read-only field beside its label
  col.addAuto(labeledRow(myTimeVcsLabel, myCurrentCyclesVcs));
  col.addAuto(labeledRow(myTimeArmLabel, myCurrentCyclesArm));
  col.addAuto(labeledRow(myQueueSizeLabel, myQueueSize));

  // The pending bus transaction, spanning the tab on a line of its own
  col.addAuto(anchoredItem(myNextTransaction));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFStateWidget::loadConfig()
{
  for(uInt8 i = 0; i < 16; i++)
    myArmRegisters->setValue(i, myCart.myCortexEmu.getRegister(i));

  BoolArray flags(4);
  flags[0] = myCart.myCortexEmu.getN();
  flags[1] = myCart.myCortexEmu.getZ();
  flags[2] = myCart.myCortexEmu.getC();
  flags[3] = myCart.myCortexEmu.getV();

  BoolArray flagsChanged(4);
  for(uInt8 i = 0; i < 4; i++) flagsChanged[i] = flags[i] != myFlagValues[i];
  myFlagValues = flags;

  myFlags->setState(flags, flagsChanged);

  myCurrentCyclesVcs->setText(std::to_string(myCart.getVcsCyclesArm()));
  myCurrentCyclesArm->setText(std::to_string(myCart.getArmCycles()));
  myQueueSize->setText(std::to_string(myCart.myTransactionQueue.size()));

  const BusTransactionQueue::Transaction* nextTransaction =
      myCart.myTransactionQueue.peekNextTransaction();
  myNextTransaction->setLabel(nextTransaction
    ? describeTransaction(
        nextTransaction->address,
        nextTransaction->mask,
        nextTransaction->timestamp + myCart.myArmCyclesOffset
      )
    : ""
  );
}
