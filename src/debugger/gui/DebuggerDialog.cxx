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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Cart.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Settings.hxx"
#include "StellaKeys.hxx"
#include "EventHandler.hxx"
#include "TabWidget.hxx"
#include "TiaInfoWidget.hxx"
#include "TiaOutputWidget.hxx"
#include "TiaZoomWidget.hxx"
#include "AudioWidget.hxx"
#include "PromptWidget.hxx"
#include "CpuWidget.hxx"
#include "RiotRamWidget.hxx"
#include "RiotWidget.hxx"
#include "RomWidget.hxx"
#include "TiaWidget.hxx"
#include "CartDebugWidget.hxx"
#include "CartRamWidget.hxx"
#include "DataGridOpsWidget.hxx"
#include "EditTextWidget.hxx"
#include "MessageBox.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "ConsoleFont.hxx"
#include "ConsoleBFont.hxx"
#include "ConsoleMediumFont.hxx"
#include "ConsoleMediumBFont.hxx"
#include "StellaMediumFont.hxx"
#include "OptionsDialog.hxx"
#include "StateManager.hxx"
#include "DebuggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem& osystem, DialogContainer& parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(nullptr),
    myRomTab(nullptr),
    myFatalError(nullptr),
    myFirstLoad(true)
{
  createFont();  // Font is sized according to available space

  addTiaArea();
  addTabArea();
  addStatusArea();
  addRomArea();

  // Inform the TIA output widget about its associated zoom widget
  myTiaOutput->setZoomWidget(myTiaZoom);

  myOptions = make_unique<OptionsDialog>(osystem, parent, this, w, h,
                                         Menu::AppMode::debugger);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
  // set initial focus to myPrompt
  if (myFirstLoad)
  {
    setFocus(myPrompt);
    myFirstLoad = false;
  }

  myTab->loadConfig();
  myTiaInfo->loadConfig();
  myTiaOutput->loadConfig();
  myTiaZoom->loadConfig();
  myCpu->loadConfig();
  myRam->loadConfig();
  myRomTab->loadConfig();

  myMessageBox->setText("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  if(key == KBDK_GRAVE && !StellaModTest::isShift(mod))
  {
    // Swallow backtick, so we don't see it when exiting the debugger
    instance().eventHandler().enableTextEvents(false);
  }

  // special debugger keys first (cannot be remapped)
  if (StellaModTest::isControl(mod))
  {
    switch (key)
    {
      case KBDK_S:
        doStep();
        return;
      case KBDK_T:
        doTrace();
        return;
      case KBDK_L:
        doScanlineAdvance();
        return;
      case KBDK_F:
        doAdvance();
        return;
      default:
        break;
    }
  }

  // handle emulation keys second (can be remapped)
  Event::Type event = instance().eventHandler().eventForKey(EventMode::kEmulationMode, key, mod);
  switch (event)
  {
    // events which can be handled 1:1
    case Event::ToggleP0Collision:
    case Event::ToggleP0Bit:
    case Event::ToggleP1Collision:
    case Event::ToggleP1Bit:
    case Event::ToggleM0Collision:
    case Event::ToggleM0Bit:
    case Event::ToggleM1Collision:
    case Event::ToggleM1Bit:
    case Event::ToggleBLCollision:
    case Event::ToggleBLBit:
    case Event::TogglePFCollision:
    case Event::TogglePFBit:
    case Event::ToggleFixedColors:
    case Event::ToggleCollisions:
    case Event::ToggleBits:

    case Event::ToggleTimeMachine:

    case Event::SaveState:
    case Event::SaveAllStates:
    case Event::ChangeState:
    case Event::LoadState:
    case Event::LoadAllStates:

    case Event::ConsoleColor:
    case Event::ConsoleBlackWhite:
    case Event::ConsoleColorToggle:
    case Event::Console7800Pause:
    case Event::ConsoleLeftDiffA:
    case Event::ConsoleLeftDiffB:
    case Event::ConsoleLeftDiffToggle:
    case Event::ConsoleRightDiffA:
    case Event::ConsoleRightDiffB:
    case Event::ConsoleRightDiffToggle:
      instance().eventHandler().handleEvent(event);
      return;

    // events which need special handling in debugger
    case Event::TakeSnapshot:
      instance().debugger().parser().run("savesnap");
      return;

    case Event::Rewind1Menu:
      doRewind();
      return;

    case Event::Rewind10Menu:
      doRewind10();
      return;

    case Event::RewindAllMenu:
      doRewindAll();
      return;

    case Event::Unwind1Menu:
      doUnwind();
      return;

    case Event::Unwind10Menu:
      doUnwind10();
      return;

    case Event::UnwindAllMenu:
      doUnwindAll();
      return;

    default:
      break;
  }

  Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  // We reload the tabs in the cases where the actions could possibly
  // change their contents
  switch(cmd)
  {
    case kDDStepCmd:
      doStep();
      break;

    case kDDTraceCmd:
      doTrace();
      break;

    case kDDAdvCmd:
      doAdvance();
      break;

    case kDDSAdvCmd:
      doScanlineAdvance();
      break;

    case kDDRewindCmd:
      doRewind();
      break;

    case kDDUnwindCmd:
      doUnwind();
      break;

    case kDDExitCmd:
      doExitDebugger();
      break;

    case kDDExitFatalCmd:
      doExitRom();
      break;

    case kDDOptionsCmd:
      myOptions->open();
      loadConfig();
      break;

    case RomWidget::kInvalidateListing:
      // Only do a full redraw if the disassembly tab is actually showing
      myRom->invalidate(myRomTab->getActiveTab() == 0);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doStep()
{
  instance().debugger().parser().run("step");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doTrace()
{
  instance().debugger().parser().run("trace");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doAdvance()
{
  instance().debugger().parser().run("frame #1");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doScanlineAdvance()
{
  instance().debugger().parser().run("scanline #1");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewind()
{
  instance().debugger().parser().run("rewind");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwind()
{
  instance().debugger().parser().run("unwind");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewind10()
{
  instance().debugger().parser().run("rewind #10");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwind10()
{
  instance().debugger().parser().run("unwind #10");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewindAll()
{
  instance().debugger().parser().run("rewind #1000");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwindAll()
{
  instance().debugger().parser().run("unwind #1000");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExitDebugger()
{
  instance().debugger().parser().run("run");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExitRom()
{
  instance().debugger().parser().run("exitrom");
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::createFont()
{
  string fontSize = instance().settings().getString("dbg.fontsize");
  int fontStyle = instance().settings().getInt("dbg.fontstyle");

  if(fontSize == "large")
  {
    // Large font doesn't use fontStyle at all
    myLFont = make_unique<GUI::Font>(GUI::stellaMediumDesc);
    myNFont = make_unique<GUI::Font>(GUI::stellaMediumDesc);
  }
  else if(fontSize == "medium")
    {
      switch(fontStyle)
      {
        case 1:
          myLFont = make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          myNFont = make_unique<GUI::Font>(GUI::consoleMediumDesc);
          break;
        case 2:
          myLFont = make_unique<GUI::Font>(GUI::consoleMediumDesc);
          myNFont = make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          break;
        case 3:
          myLFont = make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          myNFont = make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          break;
        default: // default to zero
          myLFont = make_unique<GUI::Font>(GUI::consoleMediumDesc);
          myNFont = make_unique<GUI::Font>(GUI::consoleMediumDesc);
          break;
      }
    }
  else
  {
    switch(fontStyle)
    {
      case 1:
        myLFont = make_unique<GUI::Font>(GUI::consoleBDesc);
        myNFont = make_unique<GUI::Font>(GUI::consoleDesc);
        break;
      case 2:
        myLFont = make_unique<GUI::Font>(GUI::consoleDesc);
        myNFont = make_unique<GUI::Font>(GUI::consoleBDesc);
        break;
      case 3:
        myLFont = make_unique<GUI::Font>(GUI::consoleBDesc);
        myNFont = make_unique<GUI::Font>(GUI::consoleBDesc);
        break;
      default: // default to zero
        myLFont = make_unique<GUI::Font>(GUI::consoleDesc);
        myNFont = make_unique<GUI::Font>(GUI::consoleDesc);
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::showFatalMessage(const string& msg)
{
  myFatalError = make_unique<GUI::MessageBox>(this, *myLFont, msg, _w-20, _h-20,
                                              kDDExitFatalCmd, "Exit ROM", "Continue", "Fatal error");
  myFatalError->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTiaArea()
{
  const Common::Rect& r = getTiaBounds();
  myTiaOutput =
    new TiaOutputWidget(this, *myNFont, r.x(), r.y(), r.w(), r.h());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTabArea()
{
  const Common::Rect& r = getTabBounds();
  const int vBorder = 4;

  // The tab widget
  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 0
  myTab = new TabWidget(this, *myLFont, r.x(), r.y() + vBorder,
                        r.w(), r.h() - vBorder);
  myTab->setID(0);
  addTabWidget(myTab);

  const int widWidth  = r.w() - vBorder;
  const int widHeight = r.h() - myTab->getTabHeight() - vBorder - 4;
  int tabID;

  // The Prompt/console tab
  tabID = myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, *myNFont,
                              2, 2, widWidth - 4, widHeight);
  myTab->setParentWidget(tabID, myPrompt);
  addToFocusList(myPrompt->getFocusList(), myTab, tabID);

  // The TIA tab
  tabID = myTab->addTab("TIA");
  TiaWidget* tia = new TiaWidget(myTab, *myLFont, *myNFont,
                                 2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, tia);
  addToFocusList(tia->getFocusList(), myTab, tabID);

  // The input/output tab (includes RIOT and INPTx from TIA)
  tabID = myTab->addTab("I/O");
  RiotWidget* riot = new RiotWidget(myTab, *myLFont, *myNFont,
                                    2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, riot);
  addToFocusList(riot->getFocusList(), myTab, tabID);

  // The Audio tab
  tabID = myTab->addTab("Audio");
  AudioWidget* aud = new AudioWidget(myTab, *myLFont, *myNFont,
                                     2, 2, widWidth, widHeight);
  myTab->setParentWidget(tabID, aud);
  addToFocusList(aud->getFocusList(), myTab, tabID);

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  const int lineHeight = myLFont->getLineHeight();
  const Common::Rect& r = getStatusBounds();
  int xpos, ypos;

  xpos = r.x();  ypos = r.y();
  myTiaInfo = new TiaInfoWidget(this, *myLFont, *myNFont, xpos, ypos, r.w());

  ypos += myTiaInfo->getHeight() + 10;
  myTiaZoom = new TiaZoomWidget(this, *myNFont, xpos+10, ypos,
                                r.w()-10, r.h()-lineHeight-ypos-10);
  addToFocusList(myTiaZoom->getFocusList());

  xpos += 10;  ypos += myTiaZoom->getHeight() + 10;
  myMessageBox = new EditTextWidget(this, *myLFont,
                                    xpos, ypos, myTiaZoom->getWidth(),
                                    myLFont->getLineHeight(), "");
  myMessageBox->setEditable(false, false);
  myMessageBox->clearFlags(Widget::FLAG_RETAIN_FOCUS);
  myMessageBox->setTextColor(kTextColorEm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addRomArea()
{
  static constexpr std::array<uInt32, 11> LEFT_ARROW = {
    0b0000010,
    0b0000110,
    0b0001110,
    0b0011110,
    0b0111110,
    0b1111110,
    0b0111110,
    0b0011110,
    0b0001110,
    0b0000110,
    0b0000010
  };
  static constexpr std::array<uInt32, 11> RIGHT_ARROW = {
    0b0100000,
    0b0110000,
    0b0111000,
    0b0111100,
    0b0111110,
    0b0111111,
    0b0111110,
    0b0111100,
    0b0111000,
    0b0110000,
    0b0100000
  };

  const Common::Rect& r = getRomBounds();
  const int VBORDER = 4;
  WidgetArray wid1, wid2;
  ButtonWidget* b;

  int bwidth  = myLFont->getStringWidth("Frame +1 "),
      bheight = myLFont->getLineHeight() + 2;
  int buttonX = r.x() + r.w() - bwidth - 5, buttonY = r.y() + 5;

  b = new ButtonWidget(this, *myLFont, buttonX, buttonY,
                       bwidth, bheight, "Step", kDDStepCmd, true);
  wid2.push_back(b);
  buttonY += bheight + 4;
  b = new ButtonWidget(this, *myLFont, buttonX, buttonY,
                       bwidth, bheight, "Trace", kDDTraceCmd, true);
  wid2.push_back(b);
  buttonY += bheight + 4;
  b = new ButtonWidget(this, *myLFont, buttonX, buttonY,
                       bwidth, bheight, "Scan +1", kDDSAdvCmd, true);
  wid2.push_back(b);
  buttonY += bheight + 4;
  b = new ButtonWidget(this, *myLFont, buttonX, buttonY,
                       bwidth, bheight, "Frame +1", kDDAdvCmd, true);
  wid2.push_back(b);
  buttonY += bheight + 4;
  b = new ButtonWidget(this, *myLFont, buttonX, buttonY,
                       bwidth, bheight, "Exit", kDDExitCmd);
  wid2.push_back(b);
  addCancelWidget(b);

  bwidth = bheight; // 7 + 12;
  bheight = bheight * 3 + 4 * 2;
  buttonX -= (bwidth + 5);
  buttonY = r.y() + 5;

  myRewindButton =
    new ButtonWidget(this, *myLFont, buttonX, buttonY,
                     bwidth, bheight, LEFT_ARROW.data(), 7, 11, kDDRewindCmd, true);
  myRewindButton->clearFlags(Widget::FLAG_ENABLED);

  buttonY += bheight + 4;
  bheight = (myLFont->getLineHeight() + 2) * 2 + 4 * 1;

  myUnwindButton =
    new ButtonWidget(this, *myLFont, buttonX, buttonY,
                     bwidth, bheight, RIGHT_ARROW.data(), 7, 11, kDDUnwindCmd, true);
  myUnwindButton->clearFlags(Widget::FLAG_ENABLED);

  int xpos = buttonX - 8*myLFont->getMaxCharWidth() - 20, ypos = 30;

  bwidth = myLFont->getStringWidth("Options " + ELLIPSIS);
  bheight = myLFont->getLineHeight() + 2;

  b = new ButtonWidget(this, *myLFont, xpos, r.y() + 5, bwidth, bheight,
                       "Options" + ELLIPSIS, kDDOptionsCmd);
  wid1.push_back(b);
  wid1.push_back(myRewindButton);
  wid1.push_back(myUnwindButton);

  DataGridOpsWidget* ops = new DataGridOpsWidget(this, *myLFont, xpos, ypos);

  int max_w = xpos - r.x() - 10;
  xpos = r.x() + 10;  ypos = 10;
  myCpu = new CpuWidget(this, *myLFont, *myNFont, xpos, ypos, max_w);
  addToFocusList(myCpu->getFocusList());

  addToFocusList(wid1);
  addToFocusList(wid2);

  xpos = r.x() + 10;  ypos += myCpu->getHeight() + 10;
  myRam = new RiotRamWidget(this, *myLFont, *myNFont, xpos, ypos, r.w() - 10);
  addToFocusList(myRam->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(ops);
  myRam->setOpsWidget(ops);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  xpos = r.x() + VBORDER;  ypos += myRam->getHeight() + 5;
  const int tabWidth  = r.w() - VBORDER - 1;
  const int tabHeight = r.h() - ypos - 1;
  int tabID;

  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 1
  myRomTab = new TabWidget(
      this, *myLFont, xpos, ypos, tabWidth, tabHeight);
  myRomTab->setID(1);
  addTabWidget(myRomTab);

  // The main disassembly tab
  tabID = myRomTab->addTab("  Disassembly  ", TabWidget::AUTO_WIDTH);
  myRom = new RomWidget(myRomTab, *myLFont, *myNFont, 2, 2, tabWidth - 1,
                        tabHeight - myRomTab->getTabHeight() - 2);
  myRomTab->setParentWidget(tabID, myRom);
  addToFocusList(myRom->getFocusList(), myRomTab, tabID);

  // The 'cart-specific' information tab (optional)

  tabID = myRomTab->addTab(" " + instance().console().cartridge().name() + " ", TabWidget::AUTO_WIDTH);
  myCartInfo = instance().console().cartridge().infoWidget(
    myRomTab, *myLFont, *myNFont, 2, 2, tabWidth - 1,
    tabHeight - myRomTab->getTabHeight() - 2);
  if(myCartInfo != nullptr)
  {
    myRomTab->setParentWidget(tabID, myCartInfo);
    addToFocusList(myCartInfo->getFocusList(), myRomTab, tabID);
    tabID = myRomTab->addTab("    States    ", TabWidget::AUTO_WIDTH);
  }

  // The 'cart-specific' state tab
  myCartDebug = instance().console().cartridge().debugWidget(
        myRomTab, *myLFont, *myNFont, 2, 2, tabWidth - 1,
        tabHeight - myRomTab->getTabHeight() - 2);
  if(myCartDebug)  // TODO - make this always non-null
  {
    myRomTab->setParentWidget(tabID, myCartDebug);
    addToFocusList(myCartDebug->getFocusList(), myRomTab, tabID);

    // The cartridge RAM tab
    if (myCartDebug->internalRamSize() > 0)
    {
      tabID = myRomTab->addTab(" Cartridge RAM ", TabWidget::AUTO_WIDTH);
      myCartRam =
        new CartRamWidget(myRomTab, *myLFont, *myNFont, 2, 2, tabWidth - 1,
                tabHeight - myRomTab->getTabHeight() - 2, *myCartDebug);
      if(myCartRam)  // TODO - make this always non-null
      {
        myRomTab->setParentWidget(tabID, myCartRam);
        addToFocusList(myCartRam->getFocusList(), myRomTab, tabID);
        myCartRam->setOpsWidget(ops);
      }
    }
  }

  myRomTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getTiaBounds() const
{
  // The area showing the TIA image (NTSC and PAL supported, up to 260 lines)
  return Common::Rect(0, 0, 320, std::max(260, int(_h * 0.35)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getRomBounds() const
{
  // The ROM area is the full area to the right of the tabs
  const Common::Rect& status = getStatusBounds();
  return Common::Rect(status.x() + status.w() + 1, 0, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getStatusBounds() const
{
  // The status area is the full area to the right of the TIA image
  // extending as far as necessary
  // 30% of any space above 1030 pixels will be allocated to this area
  const Common::Rect& tia = getTiaBounds();

  int x1 = tia.x() + tia.w() + 1;
  int y1 = 0;
  int x2 = tia.x() + tia.w() + 225 + (_w > 1030 ? int(0.35 * (_w - 1030)) : 0);
  int y2 = tia.y() + tia.h();

  return Common::Rect(x1, y1, x2, y2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getTabBounds() const
{
  // The tab area is the full area below the TIA image
  const Common::Rect& tia    = getTiaBounds();
  const Common::Rect& status = getStatusBounds();

  return Common::Rect(0, tia.y() + tia.h() + 1,
                      status.x() + status.w() + 1, _h);
}
