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
                               int w, int h)
  : Dialog(osystem, parent, w, h)
{
  // Font is sized according to available space
  changeFont(instance().settings().getString("dbg.fontsize"),
             instance().settings().getInt("dbg.fontstyle"));

  addTiaArea();
  addTabArea();
  addStatusArea();
  addRomArea();

  // Inform the TIA output widget about its associated zoom widget
  myTiaOutput->setZoomWidget(myTiaZoom);

  setHelpAnchor(" ", true);

  // Settle the geometry now, because Debugger::initialize() asks for minSize()
  // before ever opening the dialog, and only laying out produces it.  open()
  // lays it out again, at whatever size the debugger has settled on by then
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

  auto root = buildLayout();

  // The window minimum is the same tree's answer to a different question, so it
  // follows the font, the current ROM's tabs and the proportional band by
  // construction -- and can never depend on the size we happen to be at
  myMinSize = root->minSize();

  root->doLayout(0, 0, _w, _h);

  // Each tab widget lays its own active content out, once it has been sized
  myTab->updateTabSizes();
  myRomTab->updateTabSizes();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> DebuggerDialog::buildLayout()
{
  using GUI::BoxLayout;
  using Dir = BoxLayout::Dir;

  // Left of centre: the TIA band over the tab area.  The band takes its share of
  // the height but never less than a full PAL frame, and the tabs take the rest
  const Common::Size tabNatural = myTab->naturalSize();

  auto left = std::make_unique<BoxLayout>(Dir::Vertical);
  left->addPercent(buildTopBand(), TIA_BAND_PERCENT,
                   FrameManager::Metrics::baseHeightPAL);
  left->addSpace(1 + VBORDER);
  left->addStretch(GUI::widgetItem(myTab, static_cast<int>(tabNatural.w),
                                          static_cast<int>(tabNatural.h)));

  // The two halves meet at the centre with the divider between them: the prompt
  // gets exactly half the window and the disassembly the other half, whatever
  // either of them holds.  Stating it as two halves rather than two equal
  // stretches matters -- stretch cells share the LEFTOVER, so unequal content
  // would put the divider off centre
  auto root = std::make_unique<BoxLayout>(Dir::Horizontal);
  root->addPercent(std::move(left), 50);
  root->addSpace(1);
  root->addStretch(buildRomArea());

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> DebuggerDialog::buildTopBand()
{
  using GUI::BoxLayout;
  using Dir = BoxLayout::Dir;

  // The image is as wide as its own height calls for, so extra width goes to the
  // status area beside it -- but it is also the one thing here that gives way,
  // and this is where it does: a TALLER window makes the image want to be wider
  // without making the window any wider, so unchecked it would squeeze the
  // status column below what its fields need.  Its cell says it can be squeezed
  // away entirely (a floor of 0), so the window minimum is decided by the status
  // fields and the tabs alone -- what a small display can least afford to lose
  const int available = _w / 2 - (1 + HBORDER) - myTiaInfo->minWidth();
  const int imageWidth = std::max(0, std::min(tiaImageWidth(), available));

  auto band = std::make_unique<BoxLayout>(Dir::Horizontal);
  band->addFixed(GUI::widgetItem(myTiaOutput), imageWidth, 0);
  band->addSpace(1 + HBORDER);
  band->addStretch(buildStatusArea());

  return band;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DebuggerDialog::tiaBandHeight() const
{
  return std::max<int>(FrameManager::Metrics::baseHeightPAL,
                       _h * TIA_BAND_PERCENT / 100);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DebuggerDialog::tiaImageWidth() const
{
  // A cell can say "a share of the box" or "what my content wants", but not "as
  // wide as my own height makes me": the image is the one leaf that needs the
  // latter, so it reads the same band law the tree is given above
  return tiaBandHeight() * TIAConstants::viewableWidth
       / FrameManager::Metrics::baseHeightPAL;
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
void DebuggerDialog::changeFont(string_view fontSize, int fontStyle)
{
  FontDesc lDesc, nDesc;

  if(fontSize == "large")
  {
    // Large font doesn't use fontStyle at all
    lDesc = nDesc = GUI::stellaMediumDesc;
  }
  else if(fontSize == "medium")
  {
    switch(fontStyle)
    {
      case 1:
        lDesc = GUI::consoleMediumBDesc;
        nDesc = GUI::consoleMediumDesc;
        break;
      case 2:
        lDesc = GUI::consoleMediumDesc;
        nDesc = GUI::consoleMediumBDesc;
        break;
      case 3:
        lDesc = nDesc = GUI::consoleMediumBDesc;
        break;
      default: // default to zero
        lDesc = nDesc = GUI::consoleMediumDesc;
        break;
    }
  }
  else
  {
    switch(fontStyle)
    {
      case 1:
        lDesc = GUI::consoleBDesc;
        nDesc = GUI::consoleDesc;
        break;
      case 2:
        lDesc = GUI::consoleDesc;
        nDesc = GUI::consoleBDesc;
        break;
      case 3:
        lDesc = nDesc = GUI::consoleBDesc;
        break;
      default: // default to zero
        lDesc = nDesc = GUI::consoleDesc;
        break;
    }
  }

  if(myLFont == nullptr)
  {
    myLFont = std::make_unique<GUI::Font>(lDesc);
    myNFont = std::make_unique<GUI::Font>(nDesc);
  }
  else
  {
    // Every widget already references *myLFont/*myNFont directly, so
    // swapping the descriptors in place picks up the new glyphs and metrics
    // without recreating anything (see refreshFont())
    myLFont->changeDesc(lDesc);
    myNFont->changeDesc(nDesc);
  }

  tooltip().setFont(*myNFont);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::refreshFont()
{
  Dialog::refreshFont();
  tooltip().setFont(*myNFont);

  // The Options dialog is a separate Dialog; only allocated on first use, so
  // it must forward explicitly rather than assume it will always be freshly
  // reconstructed before it goes stale
  if(myOptions)
    myOptions->refreshFont();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::showFatalMessage(string_view msg)
{
  GUI::MessageBox::confirm(this, msg,
    [this](bool ok) { if(ok) doExitRom(); },
    "Fatal error", "Exit ROM", "Continue", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTiaArea()
{
  myTiaOutput = new TiaOutputWidget(this, *myNFont);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addTabArea()
{
  // Every widget is created at a placeholder position/size; layoutTabArea()
  // sizes and positions them.  Each tab's content is created while that tab is
  // the active one, since setActiveTab() is what decides whose child list a
  // widget joins
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)

  // The tab widget
  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 0
  myTab = new TabWidget(this, *myLFont);
  myTab->setID(0);
  addTabWidget(myTab);

  // The Prompt/console tab
  int tabID = myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, *myNFont);
  myTab->setParentWidget(tabID, myPrompt);
  addToFocusList(myPrompt->getFocusList(), myTab, tabID);

  // The TIA tab
  tabID = myTab->addTab("TIA");
  myTiaTab = new TiaWidget(myTab, *myLFont, *myNFont);
  myTab->setParentWidget(tabID, myTiaTab);
  addToFocusList(myTiaTab->getFocusList(), myTab, tabID);

  // The input/output tab (includes RIOT and INPTx from TIA)
  tabID = myTab->addTab("I/O");
  myRiotTab = new RiotWidget(myTab, *myLFont, *myNFont);
  myTab->setParentWidget(tabID, myRiotTab);
  addToFocusList(myRiotTab->getFocusList(), myTab, tabID);

  // The Audio tab
  tabID = myTab->addTab("Audio");
  myAudioTab = new AudioWidget(myTab, *myLFont, *myNFont);
  myTab->setParentWidget(tabID, myAudioTab);
  addToFocusList(myAudioTab->getFocusList(), myTab, tabID);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTiaInfo = new TiaInfoWidget(this, *myLFont, *myNFont);

  myTiaZoom = new TiaZoomWidget(this, *myNFont);
  addToFocusList(myTiaZoom->getFocusList());

  myMessageBox = new EditTextWidget(this, *myLFont, 1);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
  myMessageBox->setEditable(false, false);
  myMessageBox->clearFlags(Widget::FLAG_RETAIN_FOCUS);
  myMessageBox->setTextColor(kTextColorEm);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> DebuggerDialog::buildStatusArea()
{
  using GUI::BoxLayout;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  const int vGap = myLFont->getLineHeight() / 3;

  // The info widget reports how tall its rows make it and spells its labels out
  // only if the width allows, so it fills the column and keeps its own height --
  // and states the width below which its short labels would no longer fit, which
  // is what stops the TIA image beside it from crowding the column.  The zoom
  // view takes whatever is left; never force the message box's height, which
  // frames its own text
  auto column = std::make_unique<BoxLayout>(Dir::Vertical, vGap);
  column->addAuto(alignedItem(myTiaInfo, HAlign::Fill, VAlign::Top,
                              myTiaInfo->minWidth()));
  column->addStretch(widgetItem(myTiaZoom));
  column->addFixed(alignedItem(myMessageBox, HAlign::Fill, VAlign::Center),
                   myMessageBox->getHeight());

  return column;
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
  // sizes and positions them
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto addStepButton = [&](size_t idx, string_view label, int cmd,
                                 string_view tip, bool repeat) {
    auto* b = new ButtonWidget(this, *myLFont, 1, 1, label, cmd, repeat);
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
    new ButtonWidget(this, *myLFont, 1, 1,
                     LEFT_ARROW.data(), 7, 11, kDDRewindCmd, true);
  myRewindButton->setToolTip("Alt[+Shift]+Left");
  myRewindButton->setHelpAnchor("GlobalButtons", true);
  myRewindButton->clearFlags(Widget::FLAG_ENABLED);

  myUnwindButton =
    new ButtonWidget(this, *myLFont, 1, 1,
                     RIGHT_ARROW.data(), 7, 11, kDDUnwindCmd, true);
  myUnwindButton->setToolTip("Alt[+Shift]+Right");
  myUnwindButton->setHelpAnchor("GlobalButtons", true);
  myUnwindButton->clearFlags(Widget::FLAG_ENABLED);

  myOptionsButton = new ButtonWidget(this, *myLFont, 1, 1,
                                     "Options" + ELLIPSIS, kDDOptionsCmd);
  wid1.push_back(myOptionsButton);
  wid1.push_back(myRewindButton);
  wid1.push_back(myUnwindButton);

  myDataGridOps = new DataGridOpsWidget(this, *myLFont);

  myCpu = new CpuWidget(this, *myLFont, *myNFont);
  addToFocusList(myCpu->getFocusList());

  addToFocusList(wid1);
  addToFocusList(wid2);

  myRam = new RiotRamWidget(this, *myLFont, *myNFont);
  addToFocusList(myRam->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(myDataGridOps);
  myRam->setOpsWidget(myDataGridOps);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 1
  myRomTab = new TabWidget(this, *myLFont);
  myRomTab->setID(1);
  addTabWidget(myRomTab);

  // The main disassembly tab
  int tabID = myRomTab->addTab("  Disassembly  ", TabWidget::AUTO_WIDTH);
  myRom = new RomWidget(myRomTab, *myLFont, *myNFont);
  myRom->setHelpAnchor("Disassembly", true);
  myRomTab->setParentWidget(tabID, myRom);
  addToFocusList(myRom->getFocusList(), myRomTab, tabID);

  // The 'cart-specific' information tab (optional).  A tab is added BEFORE the
  // content it will hold is created: a cart widget parents its children to us,
  // and they join whichever tab is active at that moment (see setActiveTab)
  tabID = myRomTab->addTab(" " + instance().console().cartridge().name() + " ", TabWidget::AUTO_WIDTH);
  myCartInfo = instance().console().cartridge().infoWidget(myRomTab, *myLFont, *myNFont);
  if(myCartInfo != nullptr)
  {
    myRomTab->setParentWidget(tabID, myCartInfo);
    addToFocusList(myCartInfo->getFocusList(), myRomTab, tabID);
    tabID = myRomTab->addTab("    States    ", TabWidget::AUTO_WIDTH);
  }

  // The 'cart-specific' state tab, which every scheme has (Cartridge's
  // debugWidget() is pure, so one cannot be added without it)
  myCartDebug = instance().console().cartridge().debugWidget(myRomTab, *myLFont, *myNFont);
  myRomTab->setHelpAnchor("BankswitchInformation", true);
  myRomTab->setParentWidget(tabID, myCartDebug);
  addToFocusList(myCartDebug->getFocusList(), myRomTab, tabID);

  // The cartridge RAM tab
  if(myCartDebug->internalRamSize() > 0)
  {
    tabID = myRomTab->addTab(myCartDebug->tabLabel(), TabWidget::AUTO_WIDTH);
    myCartRam =
      new CartRamWidget(myRomTab, *myLFont, *myNFont, *myCartDebug);
    myCartRam->setHelpAnchor("CartridgeRAMInformation", true);
    myRomTab->setParentWidget(tabID, myCartRam);
    addToFocusList(myCartRam->getFocusList(), myRomTab, tabID);
    myCartRam->setOpsWidget(myDataGridOps);
  }
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myRomTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> DebuggerDialog::buildRomArea()
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::widgetItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const int fontWidth = myLFont->getMaxCharWidth(),
            bheight = myLFont->getLineHeight() + 2;

  // Every column in this band -- the register grids, the grid operations and the
  // step buttons -- ends level with the others.  The grids set the height, and
  // the button columns divide it into ROWS rows that SHARE what is left after a
  // fixed VGAP between each: the engine divides the same slack the same way in
  // every column, so they stay level at any font without anyone measuring anyone.
  // The rows take the slack rather than the gaps, so it goes into the buttons
  const int bwidth = myLFont->getStringWidth("Frame +1 ");
  const int lastRow = (DataGridOpsWidget::ROWS - 1) * 2;

  const auto bandRows = [&](int cols, int hGap) {
    auto g = std::make_unique<GridLayout>(cols, lastRow + 1, hGap, 0);
    for(int row = 0; row <= lastRow; ++row)
    {
      if(row % 2)
        g->rowFixed(row, VGAP);
      else
        g->rowStretch(row);
    }
    return g;
  };
  const auto bandButton = [](ButtonWidget* b) {
    return alignedItem(b, HAlign::Fill, VAlign::Fill);
  };

  // The step and run buttons, in a column down the right-hand edge
  auto stepCol = bandRows(1, 0);
  stepCol->columnStretch(0);
  for(int i = 0; i < DataGridOpsWidget::ROWS; ++i)
    stepCol->place(0, i * 2, bandButton(myStepButtons[i]));

  // The rewind and unwind arrows beside them, spanning three rows and two so
  // that the pair stands exactly as tall as the step buttons
  const int awidth = bheight;

  auto arrowCol = bandRows(1, 0);
  arrowCol->columnStretch(0);
  arrowCol->place(0, 0, bandButton(myRewindButton), 1, 5);
  arrowCol->place(0, 6, bandButton(myUnwindButton), 1, 3);

  // The Options button heads the operations column.  It keeps its width across a
  // font change, so size it here; its height is the row's, like every other
  myOptionsButton->setWidth(myLFont->getStringWidth(myOptionsButton->getLabel())
                            + fontWidth);

  auto opsCol = myDataGridOps->buildLayout(VGAP, HGAP);
  opsCol->place(0, 0, alignedItem(myOptionsButton, HAlign::Left, VAlign::Fill), 2);

  // The CPU area takes whatever width the three control columns leave it, and
  // keeps the height its own rows come to
  auto topRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  topRow->addStretch(alignedItem(myCpu, HAlign::Fill, VAlign::Top));
  topRow->addAuto(std::move(opsCol));
  topRow->addSpace(HGAP);
  topRow->addFixed(std::move(arrowCol), awidth);
  topRow->addSpace(HGAP);
  topRow->addFixed(std::move(stepCol), bwidth);

  ////////////////////////////////////////////////////////////////////
  // The three bands, stacked.  Each sits in from the edges by its own amount,
  // so the inset rides on the band rather than on the column holding them
  const auto band = [](unique_ptr<GUI::Layout> content, int left, int right) {
    auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
    row->addSpace(left);
    row->addStretch(std::move(content));
    row->addSpace(right);
    return row;
  };

  // The RAM area spans the full width below the CPU and asks for the height its
  // content comes to; the disassembly tab takes everything that is left, and
  // says what its widest cart tab needs so the window minimum accounts for it
  const Common::Size romNatural = myRomTab->naturalSize();

  auto column = std::make_unique<BoxLayout>(Dir::Vertical);
  column->addSpace(HGAP);
  column->addAuto(band(std::move(topRow), HBORDER, HGAP));
  column->addSpace(SECTION_GAP);
  column->addAuto(band(alignedItem(myRam, HAlign::Fill, VAlign::Top), HBORDER, 0));
  column->addSpace(HGAP);
  column->addStretch(band(widgetItem(myRomTab, static_cast<int>(romNatural.w),
                                               static_cast<int>(romNatural.h)),
                          VBORDER, 1));
  column->addSpace(1);

  return column;
}

