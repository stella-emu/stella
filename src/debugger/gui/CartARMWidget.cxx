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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "CartARMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARMWidget::CartridgeARMWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h, CartridgeARM& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::addCycleWidgets(int xpos, int ypos)
{
  const int INDENT = 20;
  const int VGAP = 4;

  new StaticTextWidget(_boss, _font, xpos, ypos + 1, "ARM emulation cycles:");
  xpos += INDENT; ypos += myLineHeight + VGAP;
  myIncCycles = new CheckboxWidget(_boss, _font, xpos, ypos + 1, "Increase 6507 cycles", kIncCyclesChanged);
  myIncCycles->setToolTip("Increase 6507 cycles with approximated ARM cycles.");
  myIncCycles->setTarget(this);
  addFocusWidget(myIncCycles);

  myCycleFactor = new SliderWidget(_boss, _font, myIncCycles->getRight() + _fontWidth * 2, ypos - 1,
                                   _fontWidth * 10, _lineHeight, "Factor ", _fontWidth * 7,
                                   kFactorChanged, _fontWidth * 4, "%");
  myCycleFactor->setMinValue(100); myCycleFactor->setMaxValue(200);
  myCycleFactor->setTickmarkIntervals(4);
  myCycleFactor->setToolTip("Multiply approximated ARM cycles by factor.");
  myCycleFactor->setTarget(this);
  addFocusWidget(myCycleFactor);

  ypos += myLineHeight + VGAP;
  StaticTextWidget* s = new StaticTextWidget(_boss, _font, xpos, ypos + 1, "Mem. cycles ");

  myPrevThumbMemCycles = new EditTextWidget(_boss, _font, s->getRight(), ypos - 1,
                                            EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myPrevThumbMemCycles->setEditable(false);
  myPrevThumbMemCycles->setToolTip("Number of memory cycles of last but one ARM run.");

  myThumbMemCycles = new EditTextWidget(_boss, _font, myPrevThumbMemCycles->getRight() + _fontWidth / 2, ypos - 1,
                                        EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbMemCycles->setEditable(false);
  myThumbMemCycles->setToolTip("Number of memory cycles of last ARM run.");

  s = new StaticTextWidget(_boss, _font, myThumbMemCycles->getRight() + _fontWidth * 2, ypos + 1, "Fetches ");
  myPrevThumbFetches = new EditTextWidget(_boss, _font, s->getRight(), ypos - 1,
                                          EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myPrevThumbFetches->setEditable(false);
  myPrevThumbFetches->setToolTip("Number of fetches/instructions of last but one ARM run.");

  myThumbFetches = new EditTextWidget(_boss, _font, myPrevThumbFetches->getRight() + _fontWidth / 2, ypos - 1,
                                      EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbFetches->setEditable(false);
  myThumbFetches->setToolTip("Number of fetches/instructions of last ARM run.");

  ypos += myLineHeight + VGAP;
  s = new StaticTextWidget(_boss, _font, xpos, ypos + 1, "Reads ");

  myPrevThumbReads = new EditTextWidget(_boss, _font, myPrevThumbMemCycles->getLeft(), ypos - 1,
                                        EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myPrevThumbReads->setEditable(false);
  myPrevThumbReads->setToolTip("Number of reads of last but one ARM run.");

  myThumbReads = new EditTextWidget(_boss, _font, myThumbMemCycles->getLeft(), ypos - 1,
                                    EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbReads->setEditable(false);
  myThumbReads->setToolTip("Number of reads of last ARM run.");

  s = new StaticTextWidget(_boss, _font, myThumbReads->getRight() + _fontWidth * 2, ypos + 1, "Writes ");

  myPrevThumbWrites = new EditTextWidget(_boss, _font, myPrevThumbFetches->getLeft(), ypos - 1,
                                         EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myPrevThumbWrites->setEditable(false);
  myPrevThumbWrites->setToolTip("Number of writes of last but one ARM run.");

  myThumbWrites = new EditTextWidget(_boss, _font, myThumbFetches->getLeft(), ypos - 1,
                                     EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbWrites->setEditable(false);
  myThumbWrites->setToolTip("Number of writes of last ARM run.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::saveOldState()
{
  myOldState.armStats.clear();
  myOldState.armPrevStats.clear();

  myOldState.armStats.push_back(myCart.stats().fetches
                                + myCart.stats().reads + myCart.stats().writes);
  myOldState.armStats.push_back(myCart.stats().fetches);
  myOldState.armStats.push_back(myCart.stats().reads);
  myOldState.armStats.push_back(myCart.stats().writes);

  myOldState.armPrevStats.push_back(myCart.prevStats().fetches
                                    + myCart.prevStats().reads + myCart.prevStats().writes);
  myOldState.armPrevStats.push_back(myCart.prevStats().fetches);
  myOldState.armPrevStats.push_back(myCart.prevStats().reads);
  myOldState.armPrevStats.push_back(myCart.prevStats().writes);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::loadConfig()
{
  // ARM cycles
  myIncCycles->setState(instance().settings().getBool("dev.thumb.inccycles"));
  myCycleFactor->setValue(std::round(instance().settings().getFloat("dev.thumb.cyclefactor") * 100.F));
  handleArmCycles();

  bool isChanged;

  isChanged = myCart.prevStats().fetches + myCart.prevStats().reads + myCart.prevStats().writes
    != myOldState.armPrevStats[0];
  myPrevThumbMemCycles->setText(Common::Base::toString(myCart.prevStats().fetches
                                + myCart.prevStats().reads + myCart.prevStats().writes,
                                Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.prevStats().fetches != myOldState.armPrevStats[1];
  myPrevThumbFetches->setText(Common::Base::toString(myCart.prevStats().fetches,
                              Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.prevStats().reads != myOldState.armPrevStats[2];
  myPrevThumbReads->setText(Common::Base::toString(myCart.prevStats().reads,
                            Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.prevStats().writes != myOldState.armPrevStats[3];
  myPrevThumbWrites->setText(Common::Base::toString(myCart.prevStats().writes,
                             Common::Base::Fmt::_10_6), isChanged);

  isChanged = myCart.stats().fetches + myCart.stats().reads + myCart.stats().writes
    != myOldState.armStats[0];
  myThumbMemCycles->setText(Common::Base::toString(myCart.stats().fetches
                            + myCart.stats().reads + myCart.stats().writes,
                            Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.stats().fetches != myOldState.armStats[1];
  myThumbFetches->setText(Common::Base::toString(myCart.stats().fetches,
                          Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.stats().reads != myOldState.armStats[2];
  myThumbReads->setText(Common::Base::toString(myCart.stats().reads,
                        Common::Base::Fmt::_10_6), isChanged);
  isChanged = myCart.stats().writes != myOldState.armStats[3];
  myThumbWrites->setText(Common::Base::toString(myCart.stats().writes,
                         Common::Base::Fmt::_10_6), isChanged);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  switch(cmd)
  {
    case kIncCyclesChanged:
    case kFactorChanged:
      handleArmCycles();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARMWidget::handleArmCycles()
{
  bool devSettings = instance().settings().getBool("dev.settings");
  bool enable = myIncCycles->getState();
  double factor = myCycleFactor->getValue() / 100.F;

  if(devSettings)
  {
    instance().settings().setValue("dev.thumb.inccycles", enable);
    instance().settings().setValue("dev.thumb.cyclefactor", factor);
  }

  myIncCycles->setEnabled(devSettings);
  enable &= devSettings;
  myCart.incCycles(enable);
  myCycleFactor->setEnabled(enable);
  myCart.cycleFactor(factor);
}
