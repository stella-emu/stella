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

#include "CartAR.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "M6502.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "WrappedTextWidget.hxx"
#include "Layout.hxx"
#include "CartARWidget.hxx"

namespace {
  // D4-D2 of the bank-configuration byte select one of 8 hardwired slice-pair
  // mappings for the two 2K windows (see CartAR.hxx's doc comment); index is
  // (configuration >> 2)
  constexpr std::array<string_view, 8> SliceMap = {
    "$F000=RAM2  $F800=ROM",
    "$F000=RAM0  $F800=ROM",
    "$F000=RAM2  $F800=RAM0",
    "$F000=RAM0  $F800=RAM2",
    "$F000=RAM2  $F800=ROM",
    "$F000=RAM1  $F800=ROM",
    "$F000=RAM2  $F800=RAM1",
    "$F000=RAM1  $F800=RAM2"
  };
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARWidget::CartridgeARWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeAR& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
  const size_t size = myCart.getImage().size();

  constexpr string_view info =
    "Four 2K slices (3 RAM, 1 ROM)\n"
    "  $F000-$F7FF - always a RAM bank (0, 1, 2)\n"
    "  $F800-$FFFF - a RAM bank or ROM (bank 3)\n"
    "\nNote: on real hardware, RAM contents at power-on are uncertain "
    "(may be zero or random). Currently emulated as zero-filled.\n";

  createBaseInformation(size, "Starpath", info);

  myModeInfo = new LabelWidget(boss, _font,
    myCart.myIsSoundLoad ? "Sound-load mode" : "Fast-load mode");
  myModeDetail = new LabelWidget(boss, _font, myCart.myIsSoundLoad
    ? std::format("Real BIOS + cassette audio, {} load{} in ROM image",
        myCart.myNumberOfLoadImages, myCart.myNumberOfLoadImages == 1 ? "" : "s")
    : std::format("BIOS emulated internally, {} load{} in ROM image",
        myCart.myNumberOfLoadImages, myCart.myNumberOfLoadImages == 1 ? "" : "s"));

  VariantList items;
  for(const auto& label: SliceMap)
    VarList::push_back(items, label);

  mySliceLbl = new LabelWidget(boss, _font, "Slice map");
  mySlice = new PopUpWidget(boss, _font, items, Cmd::ConfigChanged);
  mySlice->setTarget(this);
  addFocusWidget(mySlice);

  myWriteEnable = new CheckboxWidget(boss, _font, "RAM Write Enable", Cmd::ConfigChanged);
  myWriteEnable->setTarget(this);
  addFocusWidget(myWriteEnable);

  myRomPower = new CheckboxWidget(boss, _font, "ROM Power (not currently emulated)",
                                  Cmd::ConfigChanged);
  myRomPower->setTarget(this);
  myRomPower->setEnabled(false);
  addFocusWidget(myRomPower);

  myWriteStateLbl = new LabelWidget(boss, _font, "Write pending");
  myWriteState = new EditTextWidget(boss, _nfont, 1, "None");
  myWriteState->setEditable(false);
  myLabelColumn.emplace_back(myWriteStateLbl);

  // Only sound-load carts have anything to log here; myIsSoundLoad never
  // changes after construction, so whether this exists at all is decided once.
  // A heading of its own, not a label beside a control -- it has nothing to
  // line up with, so it does not join myLabelColumn (see alignLabels())
  if(myCart.myIsSoundLoad)
  {
    myLoadLog = new WrappedTextWidget(boss, _nfont, myCart.myLoadLog, 5, 5);
    myLoadLog->setEditable(false);
    myLoadLog->setEnabled(false);
  }

  // The selector's box lines up with the info fields above it
  myLabelColumn.emplace_back(mySliceLbl);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::anchoredItem;
  using GUI::indentedFill;
  using GUI::indentedItem;
  using GUI::labeledRow;

  col.addAuto(labeledRow(mySliceLbl, mySlice));
  col.addAuto(indentedItem(myWriteEnable, _fontWidth * 2));
  col.addAuto(indentedItem(myRomPower, _fontWidth * 2));

  col.addSpace(_lineHeight / 2);
  col.addAuto(labeledRow(myWriteStateLbl, myWriteState, 0, 0, true));

  col.addSpace(_lineHeight / 2);
  col.addAuto(anchoredItem(myModeInfo));
  col.addAuto(indentedItem(myModeDetail, _fontWidth * 2));

  if(myLoadLog)
  {
    // WrappedTextWidget needs its width before it can report its own height
    // (see its class comment), so it gets one here, matching the indent
    // indentedFill() below gives it -- not a label beside it (see the ctor)
    myLoadLog->setWidth(contentWidth(_w) - _fontWidth * 2);
    col.addAuto(indentedFill(myLoadLog, _fontWidth * 2));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::saveOldState()
{
  myOldRAM.assign(myCart.myImage.begin(),
                  myCart.myImage.begin() + CartridgeAR::RAM_SIZE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::loadConfig()
{
  CartDebug& cart = instance().debugger().cartDebug();
  const auto& state = static_cast<const CartState&>(cart.getState());
  const auto& oldstate = static_cast<const CartState&>(cart.getOldState());
  const bool changed = state.bank != oldstate.bank;

  mySlice->setSelectedIndex(myCart.myCurrentBank >> 2, changed);
  myWriteEnable->setState(myCart.myWriteEnabled, changed);
  myRomPower->setState(myCart.myPower, changed);

  const uInt32 elapsed = myCart.myWritePending
    ? instance().debugger().m6502().distinctAccesses() - myCart.myNumberOfDistinctAccesses
    : 0;
  myWriteState->setText(myCart.myWritePending
    ? std::format("${:02X} held, {} of 5 accesses elapsed",
        myCart.myDataHoldRegister, std::min(elapsed, 5U))
    : "None");

  if(myLoadLog)
    myLoadLog->setContent(myCart.myLoadLog);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                      int data, int id)
{
  if(cmd == Cmd::ConfigChanged)
  {
    const auto configuration = static_cast<uInt8>(
      (mySlice->getSelected() << 2) |
      (myWriteEnable->getState() ? 0b010 : 0) |
      (myRomPower->getState() ? 0 : 0b001));

    myCart.unlockHotspots();
    myCart.bank(configuration);
    myCart.lockHotspots();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeARWidget::bankState()
{
  return std::format("{}, RAM write {}",
    SliceMap[myCart.myCurrentBank >> 2],
    myCart.myWriteEnabled ? "enabled" : "disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeARWidget::internalRamSize()
{
  return static_cast<uInt32>(CartridgeAR::RAM_SIZE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeARWidget::internalRamRPort(int start)
{
  return static_cast<uInt32>(start);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeARWidget::internalRamDescription()
{
  // Kept to 3 lines: WrappedTextWidget's floor is 4 (see the ctor comment in
  // CartridgeARWidget::CartridgeARWidget), and this must stay under it so the
  // window's own auto-sizing accounts for the full text, not just the floor
  return
    "6K RAM (banks 0, 1, 2); only two of the three are windowed "
    "into $F000-F7FF / $F800-FFFF at a time\n"
    "This view always shows all three, packed back-to-back";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeARWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldRAM.begin() + start, myOldRAM.begin() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeARWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myImage.begin() + start,
                      myCart.myImage.begin() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myImage[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeARWidget::internalRamGetValue(int addr)
{
  return myCart.myImage[addr];
}
