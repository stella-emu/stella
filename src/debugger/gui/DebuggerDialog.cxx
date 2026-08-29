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
#include "FontManager.hxx"
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
  // Font is sized according to available space; the tooltip follows it
  changeFont();

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
void DebuggerDialog::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                   int data, int id)
{
  // We reload the tabs in the cases where the actions could possibly
  // change their contents
  switch(cmd)
  {
    case Cmd::Step:
      doStep();
      break;

    case Cmd::Trace:
      doTrace();
      break;

    case Cmd::AdvanceFrame:
      doAdvance();
      break;

    case Cmd::AdvanceScanline:
      doScanlineAdvance();
      break;

    case Cmd::Rewind:
      doRewind();
      break;

    case Cmd::Unwind:
      doUnwind();
      break;

    case Cmd::Run:
      doExitDebugger();
      break;

    case Cmd::Options:
      saveConfig();

      if(myOptions == nullptr)
        myOptions = std::make_unique<OptionsDialog>(instance(), parent(), this,
                                                    AppMode::debugger);
      myOptions->open();

      loadConfig();
      break;

    case RomWidget::Cmd::InvalidateListing:
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
const GUI::Font& DebuggerDialog::lfont() const
{
  return instance().fonts().debuggerLabelFont();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Font& DebuggerDialog::nfont() const
{
  return instance().fonts().debuggerTextFont();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const GUI::Font& DebuggerDialog::dfont() const
{
  return instance().fonts().debuggerDisasmFont();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::changeFont()
{
  // Every widget already references the same Font objects, so the FontManager
  // swaps their descriptors in place; the new glyphs and metrics are picked
  // up without recreating anything (see refreshFont())
  instance().fonts().loadConfig(instance().settings());

  tooltip().setFont(nfont());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::refreshFont()
{
  Dialog::refreshFont();

  // The tooltip is not a Dialog, so the container's broadcast cannot reach
  // it; it also wants the debugger's own font rather than the base's
  tooltip().setFont(nfont());
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
  myTiaOutput = new TiaOutputWidget(this, nfont());
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
  myTab = new TabWidget(this, lfont());
  myTab->setID(0);
  addTabWidget(myTab);

  // The Prompt/console tab
  int tabID = myTab->addTab("Prompt");
  myPrompt = new PromptWidget(myTab, nfont());
  myTab->setParentWidget(tabID, myPrompt);
  addToFocusList(myPrompt->getFocusList(), myTab, tabID);

  // The TIA tab
  tabID = myTab->addTab("TIA");
  myTiaTab = new TiaWidget(myTab, lfont(), nfont());
  myTab->setParentWidget(tabID, myTiaTab);
  addToFocusList(myTiaTab->getFocusList(), myTab, tabID);

  // The input/output tab (includes RIOT and INPTx from TIA)
  tabID = myTab->addTab("I/O");
  myRiotTab = new RiotWidget(myTab, lfont(), nfont());
  myTab->setParentWidget(tabID, myRiotTab);
  addToFocusList(myRiotTab->getFocusList(), myTab, tabID);

  // The Audio tab
  tabID = myTab->addTab("Audio");
  myAudioTab = new AudioWidget(myTab, lfont(), nfont());
  myTab->setParentWidget(tabID, myAudioTab);
  addToFocusList(myAudioTab->getFocusList(), myTab, tabID);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myTab->setActiveTab(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addStatusArea()
{
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTiaInfo = new TiaInfoWidget(this, lfont(), nfont());

  myTiaZoom = new TiaZoomWidget(this, nfont());
  addToFocusList(myTiaZoom->getFocusList());

  myMessageBox = new EditTextWidget(this, lfont(), 1);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
  myMessageBox->setEditable(false, false);
  myMessageBox->clearFlags(Widget::Flag::RetainFocus);
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

  const int vGap = lfont().getLineHeight() / 3;

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
  // Icons rather than raw bitmaps, so the buttons holding them size themselves
  static constexpr auto leftArrowBits = std::to_array<uInt32>({
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
  });
  static constexpr auto rightArrowBits = std::to_array<uInt32>({
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
  });
  static constexpr GUI::Icon LEFT_ARROW(7, 11, leftArrowBits);
  static constexpr GUI::Icon RIGHT_ARROW(7, 11, rightArrowBits);

  WidgetArray wid1, wid2;

  // Every widget is created at a placeholder position/size; layoutRomArea()
  // sizes and positions them
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto addStepButton = [&](size_t idx, string_view label, GuiCmd::Code cmd,
                                 string_view tip, bool repeat) {
    auto* b = new ButtonWidget(this, lfont(), label, cmd, repeat);
    b->setToolTip(tip);
    b->setHelpAnchor("GlobalButtons", true);
    myStepButtons[idx] = b;
    wid2.push_back(b);
  };
  addStepButton(0, "Step",     Cmd::Step,            "Ctrl+S", true);
  addStepButton(1, "Trace",    Cmd::Trace,           "Ctrl+T", true);
  addStepButton(2, "Scan +1",  Cmd::AdvanceScanline, "Ctrl+L", true);
  addStepButton(3, "Frame +1", Cmd::AdvanceFrame,    "Ctrl+F", true);
  addStepButton(4, "Run",      Cmd::Run,             "Escape", false);

  myRewindButton =
    new ButtonWidget(this, lfont(), LEFT_ARROW, Cmd::Rewind, true);
  myRewindButton->setToolTip("Alt[+Shift]+Left");
  myRewindButton->setHelpAnchor("GlobalButtons", true);
  myRewindButton->clearFlags(Widget::Flag::Enabled);

  myUnwindButton =
    new ButtonWidget(this, lfont(), RIGHT_ARROW, Cmd::Unwind, true);
  myUnwindButton->setToolTip("Alt[+Shift]+Right");
  myUnwindButton->setHelpAnchor("GlobalButtons", true);
  myUnwindButton->clearFlags(Widget::Flag::Enabled);

  myOptionsButton = new ButtonWidget(this, lfont(),
                                     "Options" + ELLIPSIS, Cmd::Options);
  // It heads the operations column, so it is trimmed like the op buttons under it
  myOptionsButton->setCompact();
  wid1.push_back(myOptionsButton);
  wid1.push_back(myRewindButton);
  wid1.push_back(myUnwindButton);

  myDataGridOps = new DataGridOpsWidget(this, lfont());

  myCpu = new CpuWidget(this, lfont(), nfont());
  addToFocusList(myCpu->getFocusList());

  addToFocusList(wid1);
  addToFocusList(wid2);

  myRam = new RiotRamWidget(this, lfont(), nfont());
  addToFocusList(myRam->getFocusList());

  // Add the DataGridOpsWidget to any widgets which contain a
  // DataGridWidget which we want controlled
  myCpu->setOpsWidget(myDataGridOps);
  myRam->setOpsWidget(myDataGridOps);

  ////////////////////////////////////////////////////////////////////
  // Disassembly area

  // Since there are two tab widgets in this dialog, we specifically
  // assign an ID of 1
  myRomTab = new TabWidget(this, lfont());
  myRomTab->setID(1);
  addTabWidget(myRomTab);

  // The main disassembly tab
  int tabID = myRomTab->addTab("  Disassembly  ", TabWidget::AUTO_WIDTH);
  myRom = new RomWidget(myRomTab, lfont(), nfont(), dfont());
  myRom->setHelpAnchor("Disassembly", true);
  myRomTab->setParentWidget(tabID, myRom);
  addToFocusList(myRom->getFocusList(), myRomTab, tabID);

  // The 'cart-specific' information tab (optional).  A tab is added BEFORE the
  // content it will hold is created: a cart widget parents its children to us,
  // and they join whichever tab is active at that moment (see setActiveTab)
  tabID = myRomTab->addTab(" " + instance().console().cartridge().name() + " ", TabWidget::AUTO_WIDTH);
  myCartInfo = instance().console().cartridge().infoWidget(myRomTab, lfont(), nfont());
  if(myCartInfo != nullptr)
  {
    myRomTab->setParentWidget(tabID, myCartInfo);
    addToFocusList(myCartInfo->getFocusList(), myRomTab, tabID);
    tabID = myRomTab->addTab("    States    ", TabWidget::AUTO_WIDTH);
  }

  // The 'cart-specific' state tab, which every scheme has (Cartridge's
  // debugWidget() is pure, so one cannot be added without it)
  myCartDebug = instance().console().cartridge().debugWidget(myRomTab, lfont(), nfont());
  myRomTab->setHelpAnchor("BankswitchInformation", true);
  myRomTab->setParentWidget(tabID, myCartDebug);
  addToFocusList(myCartDebug->getFocusList(), myRomTab, tabID);

  // The cartridge RAM tab
  if(myCartDebug->internalRamSize() > 0)
  {
    tabID = myRomTab->addTab(myCartDebug->tabLabel(), TabWidget::AUTO_WIDTH);
    myCartRam =
      new CartRamWidget(myRomTab, lfont(), nfont(), *myCartDebug);
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

  const int bheight = lfont().getLineHeight() + 2;

  // Every column in this band -- the register grids, the grid operations and the
  // step buttons -- ends level with the others.  The grids set the height, and
  // the button columns divide it into ROWS rows that SHARE what is left after a
  // fixed VGAP between each: the engine divides the same slack the same way in
  // every column, so they stay level at any font without anyone measuring anyone.
  // The rows take the slack rather than the gaps, so it goes into the buttons
  const int bwidth = lfont().getStringWidth("Frame +1 ");
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

  // The Options button heads the operations column; it sizes its own width from
  // its label (so it follows a font change), and takes its height from the row
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
