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

#include "DataGridWidget.hxx"
#include "GuiObject.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "TIADebug.hxx"
#include "Widget.hxx"
#include "Base.hxx"
#include "Layout.hxx"

#include "AudioWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioWidget::AudioWidget(GuiObject* boss, const GUI::Font& lfont,
                         const GUI::Font& nfont,
                         int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss)
{
  // Create every widget at a placeholder position/size; reflow() positions and
  // sizes them for the area the widget occupies
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)

  // Column headers for the two audio channels
  for(int col = 0; col < 2; ++col)
    myChannelLabels[col] = new StaticTextWidget(boss, lfont, 0, 0,
      Common::Base::toString(col, Common::Base::Fmt::_16_1), TextAlign::Left);

  // AUDF registers (frequency divider, two hex digits per channel)
  myRegLabels[0] = new StaticTextWidget(boss, lfont, 0, 0, "AUDF", TextAlign::Left);
  myAudF = new DataGridWidget(boss, nfont, 0, 0, 2, 1, 2, 5, Common::Base::Fmt::_16);
  myAudF->setTarget(this);
  myAudF->setID(kAUDFID);
  addFocusWidget(myAudF);

  // The resulting channel frequencies ("f0 / f1"), filled in by loadConfig()
  myAud0F = new StaticTextWidget(boss, lfont, 0, 0, "");
  mySlash = new StaticTextWidget(boss, lfont, 0, 0, "/");
  myAud1F = new StaticTextWidget(boss, lfont, 0, 0, "");

  // AUDC registers (control, one hex digit per channel)
  myRegLabels[1] = new StaticTextWidget(boss, lfont, 0, 0, "AUDC", TextAlign::Left);
  myAudC = new DataGridWidget(boss, nfont, 0, 0, 2, 1, 1, 4, Common::Base::Fmt::_16_1);
  myAudC->setTarget(this);
  myAudC->setID(kAUDCID);
  addFocusWidget(myAudC);

  // AUDV registers (volume, one hex digit per channel)
  myRegLabels[2] = new StaticTextWidget(boss, lfont, 0, 0, "AUDV", TextAlign::Left);
  myAudV = new DataGridWidget(boss, nfont, 0, 0, 2, 1, 1, 4, Common::Base::Fmt::_16_1);
  myAudV->setTarget(this);
  myAudV->setID(kAUDVID);
  addFocusWidget(myAudV);

  // The effective volume, filled in by loadConfig()
  myAudEffV = new StaticTextWidget(boss, lfont, 0, 0, "");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  setHelpAnchor("AudioTab", true);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::reflow()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::stretchedItem;
  using GUI::centeredItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  // A label, or the narrower AUDC/AUDV grid centered under the AUDF grid, sits on
  // that grid's line, so they share the row's baseline
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };
  const auto centeredGrid = [](Widget* wid) {
    return alignedItem(wid, HAlign::Center, VAlign::Baseline);
  };

  // Standard dialog borders/gaps, font-derived (as Dialog::hBorder/vBorder/vGap)
  const int fontWidth = _font.getMaxCharWidth(),
            VGAP      = _font.getFontHeight() / 4,
            HBORDER   = static_cast<int>(fontWidth * 1.25),
            VBORDER   = _font.getFontHeight() / 2;

  // One shared column for the three register labels
  GUI::alignLabels({{myRegLabels[0]}, {myRegLabels[1]}, {myRegLabels[2]}});

  const int labelW   = myRegLabels[0]->getWidth(),
            channelW = myAudF->colWidth(),  // one channel column of the AUDF grid
            gridW    = myAudF->getWidth();   // both AUDF channels
  // audFreq() formats each frequency as "{:7.1f}Hz" -- nine characters wide
  const int freqWidth = 9 * fontWidth;

  BoxLayout root(Dir::Vertical, VGAP, HBORDER, VBORDER);

  // Channel headers ("0"/"1"), each centered over its AUDF column
  auto headers = std::make_unique<BoxLayout>(Dir::Horizontal);
  headers->addSpace(labelW);
  for(auto* h: myChannelLabels)
    headers->addFixed(centeredItem(h), channelW);
  root.addAuto(std::move(headers));

  // AUDF row: label, the two-digit frequency-divider grid, then the resulting
  // channel frequencies "f0 / f1" (each readout fills a stated field, so its
  // loaded value never resizes it)
  auto audf = std::make_unique<BoxLayout>(Dir::Horizontal);
  audf->addAuto(onBaseline(myRegLabels[0]));
  audf->addFixed(onBaseline(myAudF), gridW);
  audf->addSpace(fontWidth);
  audf->addFixed(stretchedItem(myAud0F), freqWidth);
  audf->addFixed(anchoredItem(mySlash), fontWidth);
  audf->addFixed(stretchedItem(myAud1F), freqWidth);
  root.addAuto(std::move(audf));

  // AUDC row: label and the one-digit control grid, centered under AUDF
  auto audc = std::make_unique<BoxLayout>(Dir::Horizontal);
  audc->addAuto(onBaseline(myRegLabels[1]));
  audc->addFixed(centeredGrid(myAudC), gridW);
  root.addAuto(std::move(audc));

  // AUDV row: label, the one-digit volume grid centered under AUDF, then the
  // effective volume filling the rest of the row
  auto audv = std::make_unique<BoxLayout>(Dir::Horizontal);
  audv->addAuto(onBaseline(myRegLabels[2]));
  audv->addFixed(centeredGrid(myAudV), gridW);
  audv->addSpace(fontWidth * 2);
  audv->addStretch(stretchedItem(myAudEffV));
  root.addAuto(std::move(audv));

  root.doLayout(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::loadConfig()
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  const auto clearAll = [&]() {
    alist.clear(); vlist.clear(); changed.clear();
  };

  const Debugger& dbg = instance().debugger();
  TIADebug& tia = dbg.tiaDebug();
  const auto& state    = static_cast<const TiaState&>(tia.getState());
  const auto& oldstate = static_cast<const TiaState&>(tia.getOldState());

  // AUDF0/1
  clearAll();
  for(uInt32 i = 0; i < 2; ++i)
  {
    alist.push_back(i);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudF->setList(alist, vlist, changed);

  // AUDC0/1
  clearAll();
  for(uInt32 i = 2; i < 4; ++i)
  {
    alist.push_back(i - 2);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudC->setList(alist, vlist, changed);

  // AUDV0/1
  clearAll();
  for(uInt32 i = 4; i < 6; ++i)
  {
    alist.push_back(i - 4);
    vlist.push_back(state.aud[i]);
    changed.push_back(state.aud[i] != oldstate.aud[i]);
  }
  myAudV->setList(alist, vlist, changed);

  handleFrequencies();
  handleVolume();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::handleFrequencies()
{
  myAud0F->setLabel(instance().debugger().tiaDebug().audFreq0());
  myAud1F->setLabel(instance().debugger().tiaDebug().audFreq1());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::handleVolume()
{
  myAudEffV->setLabel(std::format("{}% (eff. volume)", getEffectiveVolume()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == DataGridWidget::kItemDataChangedCmd)
  {
    switch(id)
    {
      case kAUDFID:
        changeFrequencyRegs();
        break;

      case kAUDCID:
        changeControlRegs();
        break;

      case kAUDVID:
        changeVolumeRegs();
        break;

      default:
        cerr << "AudioWidget DG changed\n";
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::changeFrequencyRegs()
{
  const int addr = myAudF->getSelectedAddr();
  const int value = myAudF->getSelectedValue();

  switch(addr)
  {
    case kAud0Addr:
      instance().debugger().tiaDebug().audF0(value);
      break;

    case kAud1Addr:
      instance().debugger().tiaDebug().audF1(value);
      break;

    default:
      break;
  }
  handleFrequencies();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::changeControlRegs()
{
  const int addr = myAudC->getSelectedAddr();
  const int value = myAudC->getSelectedValue();

  switch(addr)
  {
    case kAud0Addr:
      instance().debugger().tiaDebug().audC0(value);
      break;

    case kAud1Addr:
      instance().debugger().tiaDebug().audC1(value);
      break;

    default:
      break;
  }
  handleFrequencies();
  handleVolume();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioWidget::changeVolumeRegs()
{
  const int addr = myAudV->getSelectedAddr();
  const int value = myAudV->getSelectedValue();

  switch(addr)
  {
    case kAud0Addr:
      instance().debugger().tiaDebug().audV0(value);
      break;

    case kAud1Addr:
      instance().debugger().tiaDebug().audV1(value);
      break;

    default:
      break;
  }
  handleVolume();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 AudioWidget::getEffectiveVolume()
{
  static constexpr std::array<int, 31> EFF_VOL = {
     0,  6, 13, 18, 24, 29, 33, 38,
    42, 46, 50, 54, 57, 60, 64, 67,
    70, 72, 75, 78, 80, 82, 85, 87,
    89, 91, 93, 95, 97, 98, 100
  };

  TIADebug& tia = instance().debugger().tiaDebug();
  const uInt32 vol = (tia.audC0() ? tia.audV0() : 0)
                   + (tia.audC1() ? tia.audV1() : 0);
  return EFF_VOL[vol];
}
