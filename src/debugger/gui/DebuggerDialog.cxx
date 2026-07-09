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

#include "Cart.hxx"
#include "Widget.hxx"
#include "Dialog.hxx"
#include "ToolTip.hxx"
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
#include "BrowserDialog.hxx"
#include "StateManager.hxx"
#include "FrameManager.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "TIAConstants.hxx"
#include "Layout.hxx"
#include "DebuggerDialog.hxx"

// Horizontal border of the status and ROM areas, left of their widgets
static constexpr int HBORDER = 10;

// Border and gap of the ROM area, around and between its widgets
static constexpr int VBORDER = 4, VGAP = 4;

// Inset and gap of the ROM area's control columns, which the CPU area stretches
// up to: the data grid operations, the rewind/unwind arrows and the step buttons
static constexpr int HGAP = 5;

// Vertical gap separating the CPU, RAM and disassembly sections
static constexpr int SECTION_GAP = 10;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem& osystem, DialogContainer& parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  createFont();  // Font is sized according to available space

  addTiaArea();
  addTabArea();
  addStatusArea();
  addRomArea();

  // Inform the TIA output widget about its associated zoom widget
  myTiaOutput->setZoomWidget(myTiaZoom);

  setHelpAnchor(" ", true);

  // Settle the geometry now, because Debugger::initialize() measures
  // getMinHeight() before ever opening the dialog, and that reads back widget
  // positions.  open() lays it out again, at whatever size the debugger has
  // settled on by then
  DebuggerDialog::layout();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::layout()
{
  // The debugger owns the (resizable) window, and so its size; take ours from
  // it every time, which is what makes a live resize re-flow this dialog
  const Common::Size& size = instance().debugger().size();
  _w = static_cast<int>(size.w);
  _h = static_cast<int>(size.h);

  // The TIA image and the status area beside it share the half of the dialog
  // left of centre, above the tabs; the ROM area has the half right of it
  layoutTiaArea();
  layoutStatusArea();
  layoutTabArea();
  layoutRomArea();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
  if(myFocusedWidget == nullptr)
    // Set initial focus to prompt tab
    myFocusedWidget = myPrompt;
  // Restore focus
  setFocus(myFocusedWidget);

  myTab->loadConfig();
  myTiaInfo->loadConfig();
  myTiaOutput->loadConfig();
  myTiaZoom->loadConfig();
  myCpu->loadConfig();
  myRam->loadConfig();
  myRomTab->loadConfig();

  myMessageBox->setText("");
  myMessageBox->setToolTip("");

  // This is the single funnel every refresh-required debugger command reaches
  // (step/trace/advance/scanline/rewind/unwind/register edits), so it is also
  // where the companion TIA window is told its contents may have changed.
  instance().debugger().invalidateTiaWindow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::saveConfig()
{
  myFocusedWidget = _focusedWidget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  if(key == StellaKey::GRAVE && !StellaModTest::isShift(mod))
  {
    // Swallow backtick, so we don't see it when exiting the debugger
    instance().eventHandler().enableTextEvents(false);
  }

  // Process widget keys first
  if(_focusedWidget && _focusedWidget->handleKeyDown(key, mod))
    return;

  // special debugger keys first (cannot be remapped)
  if(StellaModTest::isControl(mod))
  {
    switch(key)
    {
      case StellaKey::S:
        doStep();
        return;
      case StellaKey::T:
        doTrace();
        return;
      case StellaKey::L:
        doScanlineAdvance();
        return;
      case StellaKey::F:
        doAdvance();
        return;
      default:
        break;
    }
  }

  // Do not handle emulation events which have the same mapping as menu events
  if(!instance().eventHandler().checkEventForKey(EventMode::kMenuMode, key, mod))
  {
    // handle emulation keys second (can be remapped)
    const Event::Type event = instance().eventHandler().eventForKey(EventMode::kEmulationMode, key, mod);
    switch(event)
    {
      case Event::ExitMode:
        // make consistent, exit debugger on key UP
        if(!repeated)
          myExitPressed = true;
        return;

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
      case Event::PreviousState:
      case Event::NextState:
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
        if(!repeated)
          instance().eventHandler().handleEvent(event);
        return;

        // events which need special handling in debugger
      case Event::TakeSnapshot:
        if(!repeated)
          instance().debugger().parser().run("saveSnap");
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
  }
  Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  if(myExitPressed
     && Event::ExitMode == instance().eventHandler().eventForKey(EventMode::kEmulationMode, key, mod))
  {
    myExitPressed = false;
    instance().debugger().parser().run("run");
  }
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

    case kDDRunCmd:
      doExitDebugger();
      break;

    case kDDExitFatalCmd:
      doExitRom();
      break;

    case kDDOptionsCmd:
      saveConfig();

      if(myOptions == nullptr)
        myOptions = std::make_unique<OptionsDialog>(instance(), parent(), this,
                                               AppMode::debugger);
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
  instance().debugger().parser().run("scanLine #1");
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
  instance().debugger().parser().run("exitRom");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::createFont()
{
  const string fontSize = instance().settings().getString("dbg.fontsize");
  const int fontStyle = instance().settings().getInt("dbg.fontstyle");

  if(fontSize == "large")
  {
    // Large font doesn't use fontStyle at all
    myLFont = std::make_unique<GUI::Font>(GUI::stellaMediumDesc);
    myNFont = std::make_unique<GUI::Font>(GUI::stellaMediumDesc);
  }
  else if(fontSize == "medium")
    {
      switch(fontStyle)
      {
        case 1:
          myLFont = std::make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          myNFont = std::make_unique<GUI::Font>(GUI::consoleMediumDesc);
          break;
        case 2:
          myLFont = std::make_unique<GUI::Font>(GUI::consoleMediumDesc);
          myNFont = std::make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          break;
        case 3:
          myLFont = std::make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          myNFont = std::make_unique<GUI::Font>(GUI::consoleMediumBDesc);
          break;
        default: // default to zero
          myLFont = std::make_unique<GUI::Font>(GUI::consoleMediumDesc);
          myNFont = std::make_unique<GUI::Font>(GUI::consoleMediumDesc);
          break;
      }
    }
  else
  {
    switch(fontStyle)
    {
      case 1:
        myLFont = std::make_unique<GUI::Font>(GUI::consoleBDesc);
        myNFont = std::make_unique<GUI::Font>(GUI::consoleDesc);
        break;
      case 2:
        myLFont = std::make_unique<GUI::Font>(GUI::consoleDesc);
        myNFont = std::make_unique<GUI::Font>(GUI::consoleBDesc);
        break;
      case 3:
        myLFont = std::make_unique<GUI::Font>(GUI::consoleBDesc);
        myNFont = std::make_unique<GUI::Font>(GUI::consoleBDesc);
        break;
      default: // default to zero
        myLFont = std::make_unique<GUI::Font>(GUI::consoleDesc);
        myNFont = std::make_unique<GUI::Font>(GUI::consoleDesc);
        break;
    }
  }
  tooltip().setFont(*myNFont);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::showFatalMessage(string_view msg)
{
  myFatalError = std::make_unique<GUI::MessageBox>(this, *myLFont, msg, _w-20, _h-20,
                                              kDDExitFatalCmd, "Exit ROM", "Continue", "Fatal error");
  myFatalError->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTiaArea()
{
  myTiaOutput = new TiaOutputWidget(this, *myNFont, 0, 0, 1, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::layoutTiaArea()
{
  const Common::Rect& r = getTiaBounds();

  // The widget fits the image within this area, keeping its proportion
  myTiaOutput->setArea(r.x(), r.y(), r.w(), r.h());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTabArea()
{
  // Every widget is created at a placeholder position/size; layoutTabArea()
  // sizes and positions them.  Each tab's content must be created while that
  // tab is the active one, so that recordContentHeight() only measures it
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)

  // The tab widget
  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 0
  myTab = new TabWidget(this, *myLFont, 0, 0, 1, 1);
  myTab->setID(0);
  addTabWidget(myTab);

  // The Prompt/console tab
  int tabID = myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, *myNFont, 0, 0, 1, 1);
  myTab->setParentWidget(tabID, myPrompt);
  addToFocusList(myPrompt->getFocusList(), myTab, tabID);

  // The TIA tab
  tabID = myTab->addTab("TIA");
  myTiaTab = new TiaWidget(myTab, *myLFont, *myNFont, 2, 2, 1, 1);
  myTiaTab->recordContentHeight();
  myTab->setParentWidget(tabID, myTiaTab);
  addToFocusList(myTiaTab->getFocusList(), myTab, tabID);

  // The input/output tab (includes RIOT and INPTx from TIA)
  tabID = myTab->addTab("I/O");
  myRiotTab = new RiotWidget(myTab, *myLFont, *myNFont, 2, 2, 1, 1);
  myRiotTab->recordContentHeight();
  myTab->setParentWidget(tabID, myRiotTab);
  addToFocusList(myRiotTab->getFocusList(), myTab, tabID);

  // The Audio tab
  tabID = myTab->addTab("Audio");
  myAudioTab = new AudioWidget(myTab, *myLFont, *myNFont, 2, 2, 1, 1);
  myAudioTab->recordContentHeight();
  myTab->setParentWidget(tabID, myAudioTab);
  addToFocusList(myAudioTab->getFocusList(), myTab, tabID);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::layoutTabArea()
{
  const Common::Rect& r = getTabBounds();

  myTab->setArea(r.x(), r.y() + VBORDER, r.w(), r.h() - VBORDER);
  myTab->updateTabSizes();

  const int widWidth  = r.w() - VBORDER;
  const int widHeight = r.h() - myTab->getTabHeight() - VBORDER - 4;

  // Tab contents are positioned relative to their tab, which carries them along
  myPrompt->setArea(2, 2, widWidth - 4, widHeight);
  myTiaTab->setArea(2, 2, widWidth, widHeight);
  myRiotTab->setArea(2, 2, widWidth, widHeight);
  myAudioTab->setArea(2, 2, widWidth, widHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTiaInfo = new TiaInfoWidget(this, *myLFont, *myNFont, 0, 0, 1);

  myTiaZoom = new TiaZoomWidget(this, *myNFont, 0, 0, 1, 1);
  addToFocusList(myTiaZoom->getFocusList());

  myMessageBox = new EditTextWidget(this, *myLFont, 0, 0, 1,
                                    myLFont->getLineHeight());
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
  myMessageBox->setEditable(false, false);
  myMessageBox->clearFlags(Widget::FLAG_RETAIN_FOCUS);
  myMessageBox->setTextColor(kTextColorEm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::layoutStatusArea()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::vCentered;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  const Common::Rect& r = getStatusBounds();
  const int vGap = myLFont->getLineHeight() / 3;
  const int xpos = r.x() + HBORDER,
            width = r.w() - HBORDER;

  // The column reaches down to the tab area below it, leaving the message box
  // in the gap directly above the tabs
  const int height = getTabBounds().y() + VBORDER - r.y();

  // The info widget fits its height to the width it is given (and spells its
  // labels out only if they fit), so measure it before the column shares out
  // what is left; the box only repositions it
  myTiaInfo->setArea(xpos, r.y(), width, 0);

  // The zoom view takes whatever height is left; never force the message box's
  // height, which frames its own text
  BoxLayout column(Dir::Vertical, vGap);
  column.addFixed(anchoredItem(myTiaInfo), myTiaInfo->getHeight());
  column.addStretch(widgetItem(myTiaZoom));
  column.addFixed(vCentered(myMessageBox, myMessageBox->getHeight()),
                  myMessageBox->getHeight());
  column.doLayout(xpos, r.y(), width, height);
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

  WidgetArray wid1, wid2;

  // Every widget is created at a placeholder position/size; layoutRomArea()
  // sizes and positions them.  The one exception is the cart widgets, whose
  // ctors derive field widths (and the word wrapping of their descriptions)
  // from their width, and which have no reflow of their own; they are given
  // their real width here, which the externally sized dialog already knows.
  // Their height still is a placeholder, which only CartRamWidget reads (to
  // size the RAM view, which its setArea() then corrects)
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto addStepButton = [&](size_t idx, string_view label, int cmd,
                                 string_view tip, bool repeat) {
    auto* b = new ButtonWidget(this, *myLFont, 0, 0, 1, 1, label, cmd, repeat);
    b->setToolTip(tip);
    b->setHelpAnchor("GlobalButtons", true);
    myStepButtons[idx] = b;
    wid2.push_back(b);
  };
  addStepButton(0, "Step",     kDDStepCmd,  "Ctrl+S", true);
  addStepButton(1, "Trace",    kDDTraceCmd, "Ctrl+T", true);
  addStepButton(2, "Scan +1",  kDDSAdvCmd,  "Ctrl+L", true);
  addStepButton(3, "Frame +1", kDDAdvCmd,   "Ctrl+F", true);
  addStepButton(4, "Run",      kDDRunCmd,   "Escape", false);

  myRewindButton =
    new ButtonWidget(this, *myLFont, 0, 0, 1, 1,
                     LEFT_ARROW.data(), 7, 11, kDDRewindCmd, true);
  myRewindButton->setToolTip("Alt[+Shift]+Left");
  myRewindButton->setHelpAnchor("GlobalButtons", true);
  myRewindButton->clearFlags(Widget::FLAG_ENABLED);

  myUnwindButton =
    new ButtonWidget(this, *myLFont, 0, 0, 1, 1,
                     RIGHT_ARROW.data(), 7, 11, kDDUnwindCmd, true);
  myUnwindButton->setToolTip("Alt[+Shift]+Right");
  myUnwindButton->setHelpAnchor("GlobalButtons", true);
  myUnwindButton->clearFlags(Widget::FLAG_ENABLED);

  myOptionsButton = new ButtonWidget(this, *myLFont, 0, 0, 1, 1,
                                     "Options" + ELLIPSIS, kDDOptionsCmd);
  wid1.push_back(myOptionsButton);
  wid1.push_back(myRewindButton);
  wid1.push_back(myUnwindButton);

  myDataGridOps = new DataGridOpsWidget(this, *myLFont, 0, 0);

  myCpu = new CpuWidget(this, *myLFont, *myNFont, 0, 0, 1);
  addToFocusList(myCpu->getFocusList());

  addToFocusList(wid1);
  addToFocusList(wid2);

  myRam = new RiotRamWidget(this, *myLFont, *myNFont, 0, 0, 1);
  addToFocusList(myRam->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(myDataGridOps);
  myRam->setOpsWidget(myDataGridOps);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 1
  myRomTab = new TabWidget(this, *myLFont, 0, 0, 1, 1);
  myRomTab->setID(1);
  addTabWidget(myRomTab);

  // The main disassembly tab
  int tabID = myRomTab->addTab("  Disassembly  ", TabWidget::AUTO_WIDTH);
  myRom = new RomWidget(myRomTab, *myLFont, *myNFont, 2, 2, 1, 1);
  myRom->setHelpAnchor("Disassembly", true);
  myRomTab->setParentWidget(tabID, myRom);
  addToFocusList(myRom->getFocusList(), myRomTab, tabID);

  // The width the cart widgets are built for (see the note above)
  const int cartWidth = getRomBounds().w() - VBORDER - 2;

  // The 'cart-specific' information tab (optional)
  tabID = myRomTab->addTab(" " + instance().console().cartridge().name() + " ", TabWidget::AUTO_WIDTH);
  myCartInfo = instance().console().cartridge().infoWidget(
    myRomTab, *myLFont, *myNFont, 2, 2, cartWidth, 1);
  if(myCartInfo != nullptr)
  {
    myCartInfo->recordContentHeight();
    myRomTab->setParentWidget(tabID, myCartInfo);
    addToFocusList(myCartInfo->getFocusList(), myRomTab, tabID);
    tabID = myRomTab->addTab("    States    ", TabWidget::AUTO_WIDTH);
  }

  // The 'cart-specific' state tab
  myCartDebug = instance().console().cartridge().debugWidget(
        myRomTab, *myLFont, *myNFont, 2, 2, cartWidth, 1);
  if(myCartDebug)  // TODO - make this always non-null
  {
    myCartDebug->recordContentHeight();
    myRomTab->setHelpAnchor("BankswitchInformation", true);
    myRomTab->setParentWidget(tabID, myCartDebug);
    addToFocusList(myCartDebug->getFocusList(), myRomTab, tabID);

    // The cartridge RAM tab
    if(myCartDebug->internalRamSize() > 0)
    {
      tabID = myRomTab->addTab(myCartDebug->tabLabel(), TabWidget::AUTO_WIDTH);
      myCartRam =
        new CartRamWidget(myRomTab, *myLFont, *myNFont, 2, 2, cartWidth, 1,
                          *myCartDebug);
      if(myCartRam)  // TODO - make this always non-null
      {
        myCartRam->setHelpAnchor("CartridgeRAMInformation", true);
        myRomTab->setParentWidget(tabID, myCartRam);
        addToFocusList(myCartRam->getFocusList(), myRomTab, tabID);
        myCartRam->setOpsWidget(myDataGridOps);
      }
    }
  }
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myRomTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::layoutRomArea()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  const Common::Rect& r = getRomBounds();
  const int fontWidth = myLFont->getMaxCharWidth(),
            bheight = myLFont->getLineHeight() + 2;

  // The step and run buttons, in a column down the right-hand edge
  const int bwidth = myLFont->getStringWidth("Frame +1 ");

  auto stepCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  for(auto* b: myStepButtons)
    stepCol->addFixed(widgetItem(b), bheight);

  // The rewind and unwind arrows beside them, spanning three and two rows so
  // that the pair stands exactly as tall as the step buttons
  const int awidth = bheight;

  auto arrowCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  arrowCol->addFixed(widgetItem(myRewindButton), bheight * 3 + VGAP * 2);
  arrowCol->addFixed(widgetItem(myUnwindButton), bheight * 2 + VGAP);

  // The Options button, with the data grid operations below it.  Neither is
  // stretched, so the button takes its size from the live font here (a button
  // keeps its size across a font change); the ops widget sizes itself
  myOptionsButton->setWidth(myLFont->getStringWidth(myOptionsButton->getLabel())
                            + fontWidth);
  myOptionsButton->setHeight(bheight);

  auto opsCol = std::make_unique<BoxLayout>(Dir::Vertical);
  opsCol->addFixed(anchoredItem(myOptionsButton), bheight);
  opsCol->addSpace(VGAP * 2);
  opsCol->addFixed(anchoredItem(myDataGridOps), myDataGridOps->getHeight());

  // The CPU area takes whatever width the three control columns leave it, and
  // sizes its own height to its content
  BoxLayout topRow(Dir::Horizontal);
  topRow.addStretch(widgetItem(myCpu));
  topRow.addFixed(std::move(opsCol), myDataGridOps->getWidth());
  topRow.addSpace(HGAP);
  topRow.addFixed(std::move(arrowCol), awidth);
  topRow.addSpace(HGAP);
  topRow.addFixed(std::move(stepCol), bwidth);
  topRow.doLayout(r.x() + HBORDER, r.y() + HGAP, r.w() - HBORDER - HGAP,
                  bheight * 5 + VGAP * 4);

  // The RAM area spans the full width below the CPU, and sizes its own height
  myRam->setArea(r.x() + HBORDER, myCpu->getBottom() + SECTION_GAP,
                 r.w() - HBORDER, 0);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  const int tabY = myRam->getBottom() + HGAP;
  const int tabWidth = r.w() - VBORDER - 1;
  const int tabHeight = r.h() - tabY - 1;

  myRomTab->setArea(r.x() + VBORDER, tabY, tabWidth, tabHeight);
  myRomTab->updateTabSizes();

  const int contentW = tabWidth - 1;
  const int contentH = tabHeight - myRomTab->getTabHeight() - 2;

  myRom->setArea(2, 2, contentW, contentH);

  // The cart widgets have no reflow of their own (see addRomArea()), so this
  // only updates their bounds; their fields keep the width they were built for
  if(myCartInfo != nullptr)  myCartInfo->setArea(2, 2, contentW, contentH);
  if(myCartDebug != nullptr) myCartDebug->setArea(2, 2, contentW, contentH);
  if(myCartRam != nullptr)   myCartRam->setArea(2, 2, contentW, contentH);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DebuggerDialog::statusMinWidth() const
{
  // Whatever the status area holds, the TIA info fields must still fit, if only
  // in their short-label form
  return myTiaInfo->minWidth() + HBORDER;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getTiaBounds() const
{
  // The area showing the TIA image (NTSC and PAL supported, up to 274 lines
  // without scaling).  Its height follows the dialog, and its width follows its
  // height, so that the image keeps its proportion as it grows.  Extra width
  // therefore goes to the status area beside it, which the image never crowds
  // below the width its fields need
  const int h = std::max<int>(FrameManager::Metrics::baseHeightPAL, _h * 0.35);
  const int w = std::min<int>(
      h * TIAConstants::viewableWidth / FrameManager::Metrics::baseHeightPAL,
      _w / 2 - statusMinWidth() - 1);

  return {0, 0, static_cast<uInt32>(std::max(w, 1)), static_cast<uInt32>(h)};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getRomBounds() const
{
  // The ROM area is the full area right of the dialog's centre line
  return {
    static_cast<uInt32>(_w / 2 + 1), 0,
    static_cast<uInt32>(_w), static_cast<uInt32>(_h)
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getStatusBounds() const
{
  // The status area fills the rest of the left half, right of the TIA image
  const Common::Rect& tia = getTiaBounds();

  return {
      tia.x() + tia.w() + 1,
      0,
      static_cast<uInt32>(_w / 2),
      tia.y() + tia.h()
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DebuggerDialog::getMinHeight() const
{
  int minHeight = 0;

  // TIA-area tabs (TIA / I/O / Audio) sit below the TIA image, which takes
  // max(274, 0.35 * _h).  Find the smallest _h whose remaining height fits the
  // tallest tab content, covering both regimes (TIA image fixed at 274, or
  // proportional at 0.35 * _h); the larger of the two is the true minimum.
  if(myTab != nullptr)
  {
    const int tabReq = myTab->getMaxContentHeight();
    if(tabReq > 0)
    {
      const int extra = tabReq + myTab->getTabHeight() + 9;
      const int caseFixed = extra + static_cast<int>(FrameManager::Metrics::baseHeightPAL);
      const int caseProp  = (extra * 20 + 12) / 13;  // ceil(extra / 0.65)

      minHeight = std::max({minHeight, caseFixed, caseProp});
    }
  }

  // ROM-area cart tabs (info / state) fill the window height below the
  // CPU/RAM area, so their relationship to _h is linear.
  if(myRomTab != nullptr)
  {
    const int cartReq = myRomTab->getMaxContentHeight();
    if(cartReq > 0)
      minHeight = std::max(minHeight,
        cartReq + myRomTab->getTop() + myRomTab->getTabHeight() + 3);
  }

  return minHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect DebuggerDialog::getTabBounds() const
{
  // The tab area is the full area below the TIA image, left of the centre line
  const Common::Rect& tia = getTiaBounds();

  return {
    0, tia.y() + tia.h() + 1,
    static_cast<uInt32>(_w / 2 + 1), static_cast<uInt32>(_h)
  };
}
