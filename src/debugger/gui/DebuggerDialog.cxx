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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Cart.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "Settings.hxx"
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
#include "DebuggerDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem& osystem, DialogContainer& parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myTab(nullptr),
    myRomTab(nullptr),
    myFatalError(nullptr)
{
  createFont();  // Font is sized according to available space

  addTiaArea();
  addTabArea();
  addStatusArea();
  addRomArea();

  // Inform the TIA output widget about its associated zoom widget
  myTiaOutput->setZoomWidget(myTiaZoom);

  myOptions = make_unique<OptionsDialog>(osystem, parent, this, w, h, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
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
void DebuggerDialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  if(key == KBDK_GRAVE && !instance().eventHandler().kbdShift(mod))
  {
    // Swallow backtick, so we don't see it when exiting the debugger
    instance().eventHandler().enableTextEvents(false);
  }
  else if(key == KBDK_F12)
  {
    instance().debugger().parser().run("savesnap");
  }
  else if(instance().eventHandler().kbdControl(mod))
  {
    switch(key)
    {
      case KBDK_R:
        if(instance().eventHandler().kbdAlt(mod))
          doRewindAll();
        else if(instance().eventHandler().kbdShift(mod))
          doRewind10();
        else
          doRewind();
        break;
      case KBDK_Y:
        if(instance().eventHandler().kbdAlt(mod))
          doUnwindAll();
        else if(instance().eventHandler().kbdShift(mod))
          doUnwind10();
        else
          doUnwind();
        break;
      case KBDK_S:
        doStep();
        break;
      case KBDK_T:
        doTrace();
        break;
      case KBDK_L:
        doScanlineAdvance();
        break;
      case KBDK_F:
        doAdvance();
        break;
      default:
        break;
    }
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doTrace()
{
  instance().debugger().parser().run("trace");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doAdvance()
{
  instance().debugger().parser().run("frame #1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doScanlineAdvance()
{
  instance().debugger().parser().run("scanline #1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewind()
{
  instance().debugger().parser().run("rewind");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwind()
{
  instance().debugger().parser().run("unwind");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewind10()
{
  instance().debugger().parser().run("rewind #10");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwind10()
{
  instance().debugger().parser().run("unwind #10");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doRewindAll()
{
  instance().debugger().parser().run("rewind #1000");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doUnwindAll()
{
  instance().debugger().parser().run("unwind #1000");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExitDebugger()
{
  instance().debugger().parser().run("run");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::doExitRom()
{
  instance().debugger().parser().run("exitrom");
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
      };
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
  myFatalError = make_unique<GUI::MessageBox>(this, *myLFont, msg, _w/2, _h/2,
                          kDDExitFatalCmd, "Exit ROM", "Continue");
  myFatalError->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTiaArea()
{
  const GUI::Rect& r = getTiaBounds();
  myTiaOutput =
    new TiaOutputWidget(this, *myNFont, r.left, r.top, r.width(), r.height());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTabArea()
{
  const GUI::Rect& r = getTabBounds();
  const int vBorder = 4;

  // The tab widget
  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 0
  myTab = new TabWidget(this, *myLFont, r.left, r.top + vBorder,
                        r.width(), r.height() - vBorder);
  myTab->setID(0);
  addTabWidget(myTab);

  const int widWidth  = r.width() - vBorder;
  const int widHeight = r.height() - myTab->getTabHeight() - vBorder - 4;
  int tabID;

  // The Prompt/console tab
  tabID = myTab->addTab(" Prompt ");
  myPrompt = new PromptWidget(myTab, *myNFont,
                              2, 2, widWidth, widHeight);
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
  const GUI::Rect& r = getStatusBounds();
  int xpos, ypos;

  xpos = r.left;  ypos = r.top;
  myTiaInfo = new TiaInfoWidget(this, *myLFont, *myNFont, xpos, ypos, r.width());

  ypos += myTiaInfo->getHeight() + 10;
  myTiaZoom = new TiaZoomWidget(this, *myNFont, xpos+10, ypos,
                                r.width()-10, r.height()-lineHeight-ypos-10);
  addToFocusList(myTiaZoom->getFocusList());

  xpos += 10;  ypos += myTiaZoom->getHeight() + 10;
  myMessageBox = new EditTextWidget(this, *myLFont,
                                    xpos, ypos, myTiaZoom->getWidth(),
                                    myLFont->getLineHeight(), "");
  myMessageBox->setEditable(false, false);
  myMessageBox->clearFlags(WIDGET_RETAIN_FOCUS);
  myMessageBox->setTextColor(kTextColorEm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addRomArea()
{
  const GUI::Rect& r = getRomBounds();
  const int VBORDER = 4;
  const string ELLIPSIS = "\x1d";

  int bwidth  = myLFont->getStringWidth("Frame +1 "),
      bheight = myLFont->getLineHeight() + 2;
  int buttonX = r.right - bwidth - 5, buttonY = r.top + 5;
  new ButtonWidget(this, *myLFont, buttonX, buttonY,
                   bwidth, bheight, "Step", kDDStepCmd);
  buttonY += bheight + 4;
  new ButtonWidget(this, *myLFont, buttonX, buttonY,
                   bwidth, bheight, "Trace", kDDTraceCmd);
  buttonY += bheight + 4;
  new ButtonWidget(this, *myLFont, buttonX, buttonY,
                   bwidth, bheight, "Scan +1", kDDSAdvCmd);
  buttonY += bheight + 4;
  new ButtonWidget(this, *myLFont, buttonX, buttonY,
                   bwidth, bheight, "Frame +1", kDDAdvCmd);
  buttonY += bheight + 4;
  new ButtonWidget(this, *myLFont, buttonX, buttonY,
                   bwidth, bheight, "Exit", kDDExitCmd);

  bwidth = myLFont->getStringWidth("< ") + 4;
  bheight = bheight * 3 + 4 * 2;
  buttonX -= (bwidth + 5);
  buttonY = r.top + 5;
  myRewindButton =
    new ButtonWidget(this, *myLFont, buttonX, buttonY,
                     bwidth, bheight, "<", kDDRewindCmd);
  myRewindButton->clearFlags(WIDGET_ENABLED);

  buttonY += bheight + 4;
  bheight = (myLFont->getLineHeight() + 2) * 2 + 4 * 1;
  myUnwindButton =
    new ButtonWidget(this, *myLFont, buttonX, buttonY,
                     bwidth, bheight, ">", kDDUnwindCmd);
  myUnwindButton->clearFlags(WIDGET_ENABLED);

  int xpos = buttonX - 8*myLFont->getMaxCharWidth() - 20, ypos = 30;

  bwidth = myLFont->getStringWidth("Options " + ELLIPSIS);
  bheight = myLFont->getLineHeight() + 2;

  new ButtonWidget(this, *myLFont, xpos, r.top + 5, bwidth, bheight, "Options" + ELLIPSIS, kDDOptionsCmd);

  DataGridOpsWidget* ops = new DataGridOpsWidget(this, *myLFont, xpos, ypos);

  int max_w = xpos - r.left - 10;
  xpos = r.left + 10;  ypos = 10;
  myCpu = new CpuWidget(this, *myLFont, *myNFont, xpos, ypos, max_w);
  addToFocusList(myCpu->getFocusList());

  xpos = r.left + 10;  ypos += myCpu->getHeight() + 10;
  myRam = new RiotRamWidget(this, *myLFont, *myNFont, xpos, ypos, r.width() - 10);
  addToFocusList(myRam->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(ops);
  myRam->setOpsWidget(ops);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  xpos = r.left + VBORDER;  ypos += myRam->getHeight() + 5;
  const int tabWidth  = r.width() - VBORDER - 1;
  const int tabHeight = r.height() - ypos - 1;
  int tabID;

  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 1
  myRomTab = new TabWidget(
      this, *myLFont, xpos, ypos, tabWidth, tabHeight);
  myRomTab->setID(1);
  addTabWidget(myRomTab);

  // The main disassembly tab
  tabID = myRomTab->addTab("   Disassembly   ");
  myRom = new RomWidget(myRomTab, *myLFont, *myNFont, 2, 2, tabWidth - 1,
                        tabHeight - myRomTab->getTabHeight() - 2);
  myRomTab->setParentWidget(tabID, myRom);
  addToFocusList(myRom->getFocusList(), myRomTab, tabID);

  // The 'cart-specific' information tab
  tabID = myRomTab->addTab(instance().console().cartridge().name());
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
      tabID = myRomTab->addTab(" Cartridge RAM ");
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
GUI::Rect DebuggerDialog::getTiaBounds() const
{
  // The area showing the TIA image (NTSC and PAL supported, up to 260 lines)
  GUI::Rect r(0, 0, 320, std::max(260, int(_h * 0.35)));
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect DebuggerDialog::getRomBounds() const
{
  // The ROM area is the full area to the right of the tabs
  const GUI::Rect& status = getStatusBounds();
  GUI::Rect r(status.right + 1, 0, _w, _h);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect DebuggerDialog::getStatusBounds() const
{
  // The status area is the full area to the right of the TIA image
  // extending as far as necessary
  // 30% of any space above 1030 pixels will be allocated to this area
  const GUI::Rect& tia = getTiaBounds();

  int x1 = tia.right + 1;
  int y1 = 0;
  int x2 = tia.right + 225 + (_w > 1030 ? int(0.35 * (_w - 1030)) : 0);
  int y2 = tia.bottom;
  GUI::Rect r(x1, y1, x2, y2);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect DebuggerDialog::getTabBounds() const
{
  // The tab area is the full area below the TIA image
  const GUI::Rect& tia    = getTiaBounds();
  const GUI::Rect& status = getStatusBounds();
  GUI::Rect r(0, tia.bottom + 1, status.right + 1, _h);

  return r;
}
