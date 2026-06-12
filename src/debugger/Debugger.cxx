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

#include "bspf.hxx"

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "FSNode.hxx"
#include "Settings.hxx"
#include "DebuggerDialog.hxx"
#include "TiaWindow.hxx"
#include "PromptWidget.hxx"
#include "DebuggerParser.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "Cart.hxx"

#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"

#include "TiaInfoWidget.hxx"
#include "EditTextWidget.hxx"

#include "RomWidget.hxx"
#include "Expression.hxx"
#include "YaccParser.hxx"

#include "TIA.hxx"
#include "Debugger.hxx"
#include "DispatchResult.hxx"

using Common::Base;

Debugger* Debugger::myStaticDebugger = nullptr;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem& osystem, Console& console)
  : DialogContainer(osystem),
    myConsole{console},
    mySystem{console.system()}
{
  // Init parser
  myParser = std::make_unique<DebuggerParser>(*this, osystem.settings());

  // Create debugger subsystems
  myCpuDebug  = std::make_unique<CpuDebug>(*this, myConsole);
  myCartDebug = std::make_unique<CartDebug>(*this, myConsole, osystem);
  myRiotDebug = std::make_unique<RiotDebug>(*this, myConsole);
  myTiaDebug  = std::make_unique<TIADebug>(*this, myConsole);

  // Allow access to this object from any class
  // Technically this violates pure OO programming, but since I know
  // there will only be ever one instance of debugger in Stella,
  // I don't care :)
  myStaticDebugger = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myDialog;  myDialog = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size Debugger::fontMinSize() const
{
  const string& fontSize = myOSystem.settings().getString("dbg.fontsize");

  if(fontSize == "large")
    return Common::Size(static_cast<uInt32>(DebuggerDialog::kLargeFontMinW),
                        static_cast<uInt32>(DebuggerDialog::kLargeFontMinH));
  if(fontSize == "medium")
    return Common::Size(static_cast<uInt32>(DebuggerDialog::kMediumFontMinW),
                        static_cast<uInt32>(DebuggerDialog::kMediumFontMinH));
  return Common::Size(static_cast<uInt32>(DebuggerDialog::kSmallFontMinW),
                      static_cast<uInt32>(DebuggerDialog::kSmallFontMinH));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size Debugger::dialogMinSize() const
{
  // Start from the font-based minimum, then make sure the window is tall
  // enough that the current ROM's fixed tab content (which can exceed the
  // font minimum, e.g. for DPC+) is never clipped
  Common::Size size = fontMinSize();

  if(myDialog != nullptr)
    size.h = std::max(size.h, static_cast<uInt32>(myDialog->getMinHeight()));

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  mySize = myOSystem.settings().getSize("dbg.res");
  const Common::Size& d = myOSystem.frameBuffer().desktopSize(BufferType::Debugger);

  // The debugger dialog is resizable, within certain bounds.  Clamp to the
  // font-based minimum first; the content-based minimum needs an existing
  // dialog to measure, so it is enforced just below.
  Common::Size minSize = fontMinSize();
  mySize.clamp(minSize.w, d.w, minSize.h, d.h);

  delete myDialog;  myDialog = nullptr;
  myDialog = new DebuggerDialog(myOSystem, *this, 0, 0, mySize.w, mySize.h);

  // Some cart types need more height than the font minimum; grow the window
  // and rebuild once if the freshly-created dialog would clip its tab content
  minSize = dialogMinSize();
  if(mySize.h < minSize.h)
  {
    mySize.clamp(minSize.w, d.w, minSize.h, d.h);
    delete myDialog;  myDialog = nullptr;
    myDialog = new DebuggerDialog(myOSystem, *this, 0, 0, mySize.w, mySize.h);
  }

  myOSystem.settings().setValue("dbg.res", mySize);

  myCartDebug->setDebugWidget(myDialog->cartDebug());

  saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Debugger::initializeVideo()
{
  const FBInitStatus status = myOSystem.frameBuffer().createDisplay(
    string{STELLA_FULL_TITLE} + ": Debugger mode",
    BufferType::Debugger, mySize);

  // The debugger window may be resized, but not below its usable minimum
  // (which depends on the configured debugger font size)
  myOSystem.frameBuffer().setWindowMinSize(dialogMinSize());

  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::requestResize()
{
  // The actual rebuild is deferred until the user stops dragging (see
  // updateTime()), since recreating the whole debugger dialog is expensive.
  // The framebuffer stretches the current frame meanwhile; here we just
  // restart the idle countdown.
  myResizePending = true;
  myResizeCountdown = 15;  // ~frames of idle before rebuilding
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::updateTime(uInt64 time)
{
  DialogContainer::updateTime(time);

  // Fire the debounced rebuild once resizing has settled
  if(myResizePending && --myResizeCountdown <= 0)
  {
    myResizePending = false;

    // Stop stretching and rebuild the framebuffer at the final dragged size,
    // so imageRect() below reflects the new window
    myOSystem.frameBuffer().applyPendingResize();

    // Derive the new (logical) dialog size from the updated window
    const uInt32 scale = myOSystem.frameBuffer().hidpiScaleFactor();
    const Common::Rect& r = myOSystem.frameBuffer().imageRect();
    Common::Size newSize(r.w() / scale, r.h() / scale);
    const Common::Size& d = myOSystem.frameBuffer().desktopSize(BufferType::Debugger);
    const Common::Size minSize = dialogMinSize();
    newSize.clamp(minSize.w, d.w, minSize.h, d.h);

    if(newSize != mySize)
    {
      mySize = newSize;
      myOSystem.settings().setValue("dbg.res", mySize);
      recreateDialog();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::recreateDialog()
{
  // Remember which tabs the user had selected; the recreate below would
  // otherwise reset them to the first tab in each area
  int activeTab = 0, activeRomTab = 0;
  myDialog->getActiveTabs(activeTab, activeRomTab);

  // Close everything currently shown (it references the existing dialog)
  while(!myDialogStack.empty())
    myDialogStack.top()->close();

  // Rebuild the base dialog at the new size, reusing all existing layout code
  delete myDialog;  myDialog = nullptr;
  myDialog = new DebuggerDialog(myOSystem, *this, 0, 0, mySize.w, mySize.h);
  myCartDebug->setDebugWidget(myDialog->cartDebug());

  // The freshly-created cart widget keeps its change-tracking baseline locally
  // (unlike TIA/CPU/RIOT, whose old state lives in the persistent debug
  // objects), so it starts empty.  Seed it now, before any loadConfig() runs,
  // otherwise clicking a cart info/state/RAM tab indexes an empty vector.
  myCartDebug->saveOldState();

  // Restore the previously-selected tabs before opening, so the initial
  // loadConfig() displays them directly (same ROM => same tab layout)
  myDialog->setActiveTabs(activeTab, activeRomTab);

  // Re-open it as the base dialog
  myOSystem.frameBuffer().clear();
  myDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start(string_view message, int address, bool read,
                     string_view toolTip)
{
  if(myOSystem.eventHandler().enterDebugMode())
  {
    myFirstLog = true;
    // This must be done *after* we enter debug mode,
    // so the message isn't erased

    string text{message};
    if(address > -1)
      text += cartDebug().getLabel(address, read, 4);
    myDialog->message().setText(text);
    myDialog->message().setToolTip(toolTip);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::startWithFatalError(string_view message)
{
  if(myOSystem.eventHandler().enterDebugMode())
  {
    // This must be done *after* we enter debug mode,
    // so the dialog is properly shown
    myDialog->showFatalMessage(message);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::quit()
{
  if(myOSystem.settings().getBool("dbg.autosave")
     && myDialog->prompt().isLoaded())
    myParser->run("save");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::exit(bool exitrom)
{
  if(exitrom)
    myOSystem.eventHandler().handleEvent(Event::ExitGame);
  else
  {
    myOSystem.eventHandler().leaveDebugMode();
    myOSystem.console().tia().clearPendingFrame();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::autoExec(StringList* history)
{
  string output;

  // autoexec.script is always run
  const FSNode autoexec(myOSystem.baseDir().getPath() + "autoexec.script");
  output += std::format(
    "autoExec():\n{}\n",
    myParser->exec(autoexec, history)
  );

  // Also, "romname.script" if present
  const string romScriptPath = myOSystem.userDir().getPath() +
      myOSystem.romFile().getNameWithExt(".script");

  const FSNode romname(romScriptPath);
  output += std::format("{}\n", myParser->exec(romname, history));

  // Also, script specified via -dbg.script command line parameter
  const string scriptPath = myOSystem.settings().getString("dbg.script");
  if(!scriptPath.empty())
  {
    const FSNode script(scriptPath);
    output += std::format("{}\n", myParser->exec(script, history));
  }

  // Init builtins
  for(const auto& func: ourBuiltinFunctions)
  {
    auto expr = YaccParser::parse(func.defn);
    if(expr)
      addFunction(func.name, func.defn, std::move(expr), true);
    else
      cerr << std::format("ERROR in builtin function {}!\n", func.name);
  }

  return output;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BreakpointMap& Debugger::breakPoints() const
{
  return mySystem.m6502().breakPoints();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrapArray& Debugger::readTraps() const
{
  return mySystem.m6502().readTraps();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrapArray& Debugger::writeTraps() const
{
  return mySystem.m6502().writeTraps();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::run(string_view command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::invIfChanged(int reg, int oldReg)
{
  string ret;

  const bool changed = reg != oldReg;
  if(changed) ret += "\177";
  ret += Common::Base::toString(reg, Common::Base::Fmt::_16_2);
  if(changed) ret += "\177";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reset()
{
  unlockSystem();
  mySystem.reset();
  lockSystem();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Element 0 of args is the address. The remaining elements are the data
   to poke, starting at the given address.
*/
string Debugger::setRAM(const IntArray& args)
{
  if(args.empty())
    return "no arguments provided";

  const size_t count = args.size(), written = count - 1;
  int address = args[0];
  for(size_t i = 1; i < count; ++i)
    mySystem.pokeOob(address++, args[i]);

  return std::format("changed {} {}", written,
    written == 1 ? "location" : "locations");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveState(int state)
{
  // Saving a state is implicitly a read-only operation, so we keep the
  // system locked, so no changes can occur
  myOSystem.state().saveState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveAllStates()
{
  // Saving states is implicitly a read-only operation, so we keep the
  // system locked, so no changes can occur
  myOSystem.state().rewindManager().saveAllStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadState(int state)
{
  // We're loading a new state, so we start with a clean slate
  mySystem.clearDirtyPages();

  // State loading could initiate a bankswitch, so we allow it temporarily
  unlockSystem();
  myOSystem.state().loadState(state);
  lockSystem();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadAllStates()
{
  // We're loading new states, so we start with a clean slate
  mySystem.clearDirtyPages();

  // State loading could initiate a bankswitch, so we allow it temporarily
  unlockSystem();
  myOSystem.state().rewindManager().loadAllStates();
  lockSystem();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::step(bool save)
{
  if(save)
    saveOldState();

  const uInt64 startCycle = mySystem.cycles();

  unlockSystem();
  myOSystem.console().tia().updateScanlineByStep().flushLineCache();
  lockSystem();

  if(save)
    addState("step");
  return static_cast<int>(mySystem.cycles() - startCycle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// trace is just like step, except it treats a subroutine call as one
// instruction.

// This implementation is not perfect: it just watches the program counter,
// instead of tracking (possibly) nested JSR/RTS pairs. In particular, it
// will fail for recursive subroutine calls. However, with 128 bytes of RAM
// to share between stack and variables, I doubt any 2600 games will ever
// use recursion...

int Debugger::trace()
{
  // 32 is the 6502 JSR instruction:
  if(mySystem.peekOob(myCpuDebug->pc()) == 32)
  {
    saveOldState();

    const uInt64 startCycle = mySystem.cycles();
    const int targetPC = myCpuDebug->pc() + 3; // return address

    // set temporary breakpoint at target PC (if not existing already)
    const Int8 bank = myCartDebug->getBank(targetPC);
    if(!checkBreakPoint(targetPC, bank))
    {
      // add temporary breakpoint and remove later
      setBreakPoint(targetPC, bank, BreakpointMap::ONE_SHOT);
    }

    unlockSystem();
    mySystem.m6502().execute(11900000); // max. ~10 seconds
    myOSystem.console().tia().flushLineCache();
    lockSystem();

    addState("trace");
    return static_cast<int>(mySystem.cycles() - startCycle);
  }
  else
    return step();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setBreakPoint(uInt16 addr, uInt8 bank, uInt32 flags) const
{
  if(checkBreakPoint(addr, bank))
    return false;

  breakPoints().add(addr, bank, flags);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::clearBreakPoint(uInt16 addr, uInt8 bank) const
{
  if(!checkBreakPoint(addr, bank))
    return false;

  breakPoints().erase(addr, bank);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::checkBreakPoint(uInt16 addr, uInt8 bank) const
{
  return breakPoints().check(addr, bank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::toggleBreakPoint(uInt16 addr, uInt8 bank) const
{
  if(checkBreakPoint(addr, bank))
    clearBreakPoint(addr, bank);
  else
    setBreakPoint(addr, bank);

  return breakPoints().check(addr, bank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addReadTrap(uInt16 t) const
{
  readTraps().initialize();
  readTraps().add(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addWriteTrap(uInt16 t) const
{
  writeTraps().initialize();
  writeTraps().add(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addTrap(uInt16 t) const
{
  addReadTrap(t);
  addWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::removeReadTrap(uInt16 t) const
{
  readTraps().initialize();
  readTraps().remove(t);
}

void Debugger::removeWriteTrap(uInt16 t) const
{
  writeTraps().initialize();
  writeTraps().remove(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::removeTrap(uInt16 t) const
{
  removeReadTrap(t);
  removeWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::readTrap(uInt16 t) const
{
  return readTraps().isInitialized() && readTraps().isSet(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::writeTrap(uInt16 t) const
{
  return writeTraps().isInitialized() && writeTraps().isSet(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::log(string_view triggerMsg)
{
  const int pc = myCpuDebug->pc();
  const auto romBanks = myCartDebug->romBankCount();

  if(myFirstLog)
  {
    string header = "Trigger:  Frame Scn Cy Pxl | PS       A  X  Y  SP | ";
    if(romBanks > 1)
      header += (romBanks > 9) ? "Bk/" : "B/";

    header += "Addr Code     Disasm";
    Logger::log(header);
    myFirstLog = false;
  }

  string msg;

  // Main CPU/timing line
  msg += std::format(
    "{:<10} {} {} {} {} | ",
    triggerMsg,
    Base::toString(myTiaDebug->frameCount(), Base::Fmt::_10_5),
    Base::toString(myTiaDebug->scanlines(), Base::Fmt::_10_3),
    Base::toString(myTiaDebug->clocksThisLine() / 3, Base::Fmt::_10_02),
    Base::toString(myTiaDebug->clocksThisLine() - 68, Base::Fmt::_10_3)
  );

  // Status flags
  msg += std::format(
    "{}{}-{}{}{}{}{} {:02X} {:02X} {:02X} {:02X} |",
    myCpuDebug->n() ? "N" : "n",
    myCpuDebug->v() ? "V" : "v",
    myCpuDebug->b() ? "B" : "b",
    myCpuDebug->d() ? "D" : "d",
    myCpuDebug->i() ? "I" : "i",
    myCpuDebug->z() ? "Z" : "z",
    myCpuDebug->c() ? "C" : "c",
    myCpuDebug->a(),
    myCpuDebug->x(),
    myCpuDebug->y(),
    myCpuDebug->sp()
  );

  // Bank info
  if(romBanks > 1)
  {
    const auto bank = myCartDebug->getBank(pc);
    msg += (romBanks > 9)
      ? Base::toString(bank, Base::Fmt::_10)
      : (" " + std::to_string(bank));
    msg += "/";
  }
  else
    msg += " ";

  // First find the lines in the range, and determine the longest string
  const auto& disasm = myCartDebug->disassembly();
  const uInt16 start = pc & mySystem.addressMask();

  for(const auto& tag: disasm.list)
  {
    if((tag.address & mySystem.addressMask()) >= start)
    {
      const string pcStr = Base::hex4(pc);
      msg += std::format("{} {:<8} {}", pcStr, tag.bytes, tag.disasm.substr(0, 7));

      if(tag.disasm.size() > 8)
        msg += tag.disasm.substr(8);

      break;
    }
  }

  Logger::log(msg);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::peek(uInt16 addr, Device::AccessFlags flags)
{
  return mySystem.peekOob(addr, flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::dpeek(uInt16 addr, Device::AccessFlags flags)
{
  return static_cast<uInt16>(mySystem.peekOob(addr, flags) |
                            (mySystem.peekOob(addr+1, flags) << 8));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::poke(uInt16 addr, uInt8 value, Device::AccessFlags flags)
{
  mySystem.pokeOob(addr, value, flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502& Debugger::m6502() const
{
  return mySystem.m6502();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::peekAsInt(int addr, Device::AccessFlags flags)
{
  return mySystem.peekOob(static_cast<uInt16>(addr), flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::dpeekAsInt(int addr, Device::AccessFlags flags)
{
  return mySystem.peekOob(static_cast<uInt16>(addr), flags) |
      (mySystem.peekOob(static_cast<uInt16>(addr+1), flags) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Device::AccessFlags Debugger::getAccessFlags(uInt16 addr) const
{
  return mySystem.getAccessFlags(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setAccessFlags(uInt16 addr, Device::AccessFlags flags)
{
  mySystem.setAccessFlags(addr, flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Device::AccessCounter Debugger::getAccessCounter(uInt16 addr) const
{
  return mySystem.getAccessCounter(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Debugger::getBaseAddress(uInt32 addr, bool read)
{
  if((addr & 0x1080) == 0x0000) // (addr & 0b 0001 0000 1000 0000) == 0b 0000 0000 0000 0000
  {
    if(read)
      // ADDR_TIA read (%xxx0 xxxx 0xxx ????)
      return addr & 0x000f; // 0b 0000 0000 0000 1111
    else
      // ADDR_TIA write (%xxx0 xxxx 0x?? ????)
      return addr & 0x003f; // 0b 0000 0000 0011 1111
  }

  // ADDR_ZPRAM (%xxx0 xx0x 1??? ????)
  if((addr & 0x1280) == 0x0080) // (addr & 0b 0001 0010 1000 0000) == 0b 0000 0000 1000 0000
    return addr & 0x00ff; // 0b 0000 0000 1111 1111

  // ADDR_ROM
  if(addr & 0x1000)
    return addr & 0x1fff; // 0b 0001 1111 1111 1111

  // ADDR_IO read/write I/O registers (%xxx0 xx1x 1xxx x0??)
  if((addr & 0x1284) == 0x0280) // (addr & 0b 0001 0010 1000 0100) == 0b 0000 0010 1000 0000
    return addr & 0x0283; // 0b 0000 0010 1000 0011

  // ADDR_IO write timers (%xxx0 xx1x 1xx1 ?1??)
  if(!read && (addr & 0x1294) == 0x0294) // (addr & 0b 0001 0010 1001 0100) == 0b 0000 0010 1001 0100
    return addr & 0x029f; // 0b 0000 0010 1001 1111

  // ADDR_IO read timers (%xxx0 xx1x 1xxx ?1x0)
  if(read && (addr & 0x1285) == 0x0284) // (addr & 0b 0001 0010 1000 0101) == 0b 0000 0010 1000 0100
    return addr & 0x028c; // 0b 0000 0010 1000 1100

  // ADDR_IO read timer/PA7 interrupt (%xxx0 xx1x 1xxx x1x1)
  if(read && (addr & 0x1285) == 0x0285) // (addr & 0b 0001 0010 1000 0101) == 0b 0000 0010 1000 0101
    return addr & 0x0285; // 0b 0000 0010 1000 0101

  // ADDR_IO write PA7 edge control (%xxx0 xx1x 1xx0 x1??)
  if(!read && (addr & 0x1294) == 0x0284) // (addr & 0b 0001 0010 1001 0100) == 0b 0000 0010 1000 0100
    return addr & 0x0287; // 0b 0000 0010 1000 0111

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextScanline(int lines)
{
  const auto label = std::format("scanline + {}", lines);

  saveOldState();
  unlockSystem();

  for(int i = 0; i < lines; ++i)
    myOSystem.console().tia().updateScanline();

  lockSystem();

  addState(label);
  myOSystem.console().tia().flushLineCache();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames)
{
  string buf = std::format("frame + {}", frames);

  saveOldState();
  unlockSystem();

  DispatchResult dispatchResult;
  auto& tia = myOSystem.console().tia();
  auto& emuTiming = myOSystem.console().emulationTiming();

  while(frames)
  {
    do
      tia.update(dispatchResult, emuTiming.maxCyclesPerTimeslice());
    while(dispatchResult.getStatus() == DispatchResult::Status::debugger);

    --frames;
  }

  lockSystem();
  addState(buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::updateRewindbuttons(const RewindManager& r)
{
  myDialog->rewindButton().setEnabled(!r.atFirst());
  myDialog->unwindButton().setEnabled(!r.atLast());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::windStates(uInt16 numStates, bool unwind, string& message)
{
  RewindManager& r = myOSystem.state().rewindManager();

  saveOldState();
  unlockSystem();

  const uInt64 startCycles = myOSystem.console().system().cycles();
  const uInt16 winds = r.windStates(numStates, unwind);
  message = r.getUnitString(myOSystem.console().system().cycles() - startCycles);

  lockSystem();

  updateRewindbuttons(r);
  return winds;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::rewindStates(uInt16 numStates, string& message)
{
  return windStates(numStates, false, message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::unwindStates(uInt16 numStates, string& message)
{
  return windStates(numStates, true, message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints() const
{
  breakPoints().clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllTraps() const
{
  readTraps().clearAll();
  writeTraps().clearAll();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::showWatches()
{
  return myParser->showWatches();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::stringToValue(string_view stringval)
{
  return myParser->decipherArg(stringval);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::patchROM(uInt16 addr, uInt8 value)
{
  return myConsole.cartridge().patch(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveOldState(bool clearDirtyPages)
{
  if(clearDirtyPages)
    mySystem.clearDirtyPages();

  lockSystem();
  myCartDebug->saveOldState();
  myCpuDebug->saveOldState();
  myRiotDebug->saveOldState();
  myTiaDebug->saveOldState();
  unlockSystem();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addState(string_view rewindMsg)
{
  // Add another rewind level to the Time Machine buffer
  RewindManager& r = myOSystem.state().rewindManager();
  r.addState(rewindMsg);
  updateRewindbuttons(r);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setStartState()
{
  // Request the companion TIA window if the user has enabled it.  Actual
  // creation is deferred to renderTiaWindow() so it happens once the state is
  // DEBUGGER (see myTiaWindowPending).
  if(myOSystem.settings().getBool("dbg.tiawindow"))
    myTiaWindowPending = true;

  // Lock the bus each time the debugger is entered, so we don't disturb anything
  lockSystem();

  // Save initial state and add it to the rewind list (except when in currently rewinding)
  const RewindManager& r = myOSystem.state().rewindManager();
  // avoid invalidating future states when entering the debugger e.g. during rewind
  if(r.atLast() && (myOSystem.eventHandler().state() != EventHandlerState::TIMEMACHINE
     || myOSystem.state().mode() == StateManager::Mode::Off))
    addState("enter debugger");
  else
    updateRewindbuttons(r);

  // Set the 're-disassemble' flag, but don't do it until the next scheduled time
  myDialog->rom().invalidate(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setQuitState()
{
  // Hide the companion TIA window (kept alive for a fast re-open)
  myTiaWindowPending = false;
  closeTiaWindow();

  myDialog->saveConfig();
  saveOldState();

  // Bus must be unlocked for normal operation when leaving debugger mode
  unlockSystem();

  // execute one instruction on quit. If we're
  // sitting at a breakpoint/trap, this will get us past it.
  // Somehow this feels like a hack to me, but I don't know why
  mySystem.m6502().execute(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleTiaWindow()
{
  if(myTiaWindowOpen)
    closeTiaWindow();
  else
    openTiaWindow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::openTiaWindow()
{
  if(myTiaWindowOpen)
    return;

  if(myTiaWindow == nullptr)
    myTiaWindow = std::make_unique<TiaWindow>(myOSystem);

  // The (single) FrameBuffer owns the secondary window/backend; palette, fonts
  // and TIASurface are shared with the main window, so nothing can be clobbered.
  const FBInitStatus status = myOSystem.frameBuffer().openSecondaryWindow(
    *myTiaWindow, string{STELLA_FULL_TITLE} + ": TIA",
    BufferType::TiaWindow, myTiaWindow->size());

  myTiaWindowOpen = (status == FBInitStatus::Success);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::closeTiaWindow()
{
  if(!myTiaWindowOpen)
    return;

  myOSystem.frameBuffer().closeSecondaryWindow();
  myTiaWindowOpen = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::renderTiaWindow()
{
  // Deferred open (now that the state is DEBUGGER, so createDisplay() doesn't
  // run the emulation-mode/phosphor path against the live console)
  if(myTiaWindowPending)
  {
    myTiaWindowPending = false;
    openTiaWindow();
  }

  if(!myTiaWindowOpen)
    return;

  // Render on demand: the companion is presented only when its dialog/widget is
  // dirty.  That covers user interaction (zoom/pan mark the widget dirty) and
  // the initial open (Dialog::open() marks it dirty); content changes from
  // stepping the emulation come in via invalidateTiaWindow().  When nothing is
  // dirty this neither redraws nor presents, so an idle companion is free.
  myOSystem.frameBuffer().renderSecondaryWindow(
    *myTiaWindow, FrameBuffer::UpdateMode::NONE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::invalidateTiaWindow()
{
  if(!myTiaWindowOpen)
    return;

  // loadConfig() cascades to TiaDisplayWidget::loadConfig(), which marks the
  // widget dirty so renderTiaWindow() redraws it next frame.  It only sets
  // dirty flags here; the actual draw is deferred (and scoped to the secondary
  // render target) by renderTiaWindow().
  myTiaWindow->baseDialog()->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DialogContainer* Debugger::tiaWindowContainer() const
{
  return myTiaWindowOpen ? myTiaWindow.get() : nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::addFunction(string_view name, string_view definition,
                           unique_ptr<Expression> exp, bool builtin)
{
  myFunctions.emplace(name, std::move(exp));
  myFunctionDefs.emplace(name, definition);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::isBuiltinFunction(string_view name)
{
  return std::ranges::any_of(ourBuiltinFunctions,
      [&name](const auto& func) { return name == func.name; });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::delFunction(string_view name)
{
  if(!myFunctions.contains(name))
    return false;

  // We never want to delete built-in functions
  if(isBuiltinFunction(name))
      return false;

  myFunctions.erase(string{name});    // TODO: heterogeneous erase fixed in C++26

  if(!myFunctionDefs.contains(name))
    return false;

  myFunctionDefs.erase(string{name}); // TODO: heterogeneous erase fixed in C++26
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Expression& Debugger::getFunction(string_view name) const
{
  const auto& iter = myFunctions.find(name);
  return iter != myFunctions.end() ? *(iter->second) : EmptyExpression();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Debugger::getFunctionDef(string_view name) const
{
  const auto& iter = myFunctionDefs.find(name);
  return iter != myFunctionDefs.end() ? iter->second : EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::FunctionDefMap Debugger::getFunctionDefMap() const
{
  return myFunctionDefs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::builtinHelp()
{
  std::ostringstream buf;
  size_t c_maxlen = 0, i_maxlen = 0;

  // Get column widths for aligned output (functions)
  for(const auto& func: ourBuiltinFunctions)
  {
    c_maxlen = std::max(func.name.size(), c_maxlen);
    i_maxlen = std::max(func.defn.size(), i_maxlen);
  }

  buf << std::setfill(' ') << "\nBuilt-in functions:\n";
  for(const auto& func: ourBuiltinFunctions)
  {
    buf << std::setw(static_cast<int>(c_maxlen)) << std::left << func.name
        << std::setw(2) << std::right << "{"
        << std::setw(static_cast<int>(i_maxlen)) << std::left << func.defn
        << std::setw(4) << "}"
        << func.help
        << '\n';
  }

  // Get column widths for aligned output (pseudo-registers)
  c_maxlen = 0;
  for(const auto& reg: ourPseudoRegisters)
    c_maxlen = std::max(reg.name.size(), c_maxlen);

  buf << "\nPseudo-registers:\n";
  for(const auto& reg: ourPseudoRegisters)
  {
    buf << std::setw(static_cast<int>(c_maxlen)) << std::left << reg.name
        << std::setw(2) << " "
        << std::setw(static_cast<int>(i_maxlen)) << std::left << reg.help
        << '\n';
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::getCompletions(string_view in, StringList& list) const
{
  // skip if filter equals "_" only
  if(!BSPF::equalsIgnoreCase(in, "_"))
  {
    for(const auto& iter: myFunctions)
    {
      const string_view l = iter.first;
      if(BSPF::matchesCamelCase(l, in))
        list.emplace_back(l);
    }

    for(const auto& reg: ourPseudoRegisters)
      if(BSPF::matchesCamelCase(reg.name, in))
        list.emplace_back(reg.name);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::lockSystem()
{
  mySystem.lockDataBus();
  myConsole.cartridge().lockHotspots();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::unlockSystem()
{
  mySystem.unlockDataBus();
  myConsole.cartridge().unlockHotspots();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::canExit() const
{
  return baseDialogIsActive();
}

// NOLINTBEGIN(bugprone-throwing-static-initialization)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::array<Debugger::BuiltinFunction, 18> Debugger::ourBuiltinFunctions = { {
  // left joystick:
  { "_joy0Left",    "!(*SWCHA & $40)", "Left joystick moved left" },
  { "_joy0Right",   "!(*SWCHA & $80)", "Left joystick moved right" },
  { "_joy0Up",      "!(*SWCHA & $10)", "Left joystick moved up" },
  { "_joy0Down",    "!(*SWCHA & $20)", "Left joystick moved down" },
  { "_joy0Fire",    "!(*INPT4 & $80)", "Left joystick fire button pressed" },

  // right joystick:
  { "_joy1Left",    "!(*SWCHA & $04)", "Right joystick moved left" },
  { "_joy1Right",   "!(*SWCHA & $08)", "Right joystick moved right" },
  { "_joy1Up",      "!(*SWCHA & $01)", "Right joystick moved up" },
  { "_joy1Down",    "!(*SWCHA & $02)", "Right joystick moved down" },
  { "_joy1Fire",    "!(*INPT5 & $80)", "Right joystick fire button pressed" },

  // console switches:
  { "_select",    "!(*SWCHB & $02)",  "Game Select pressed" },
  { "_reset",     "!(*SWCHB & $01)",  "Game Reset pressed" },
  { "_color",     "*SWCHB & $08",     "Color/BW set to Color" },
  { "_bw",        "!(*SWCHB & $08)",  "Color/BW set to BW" },
  { "_diff0B",    "!(*SWCHB & $40)",  "Left diff. set to B (easy)" },
  { "_diff0A",    "*SWCHB & $40",     "Left diff. set to A (hard)" },
  { "_diff1B",    "!(*SWCHB & $80)",  "Right diff. set to B (easy)" },
  { "_diff1A",    "*SWCHB & $80",     "Right diff. set to A (hard)" }
} };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Names are defined here, but processed in YaccParser
std::array<Debugger::PseudoRegister, 18> Debugger::ourPseudoRegisters = {{
// Debugger::PseudoRegister Debugger::ourPseudoRegisters[NUM_PSEUDO_REGS] = {
  { "_bank",          "Currently selected bank" },
  { "_cClocks",       "Color clocks on current scanline" },
  { "_cyclesHi",      "Higher 32 bits of number of cycles since emulation started" },
  { "_cyclesLo",      "Lower 32 bits of number of cycles since emulation started" },
  { "_fCount",        "Number of frames since emulation started" },
  { "_fCycles",       "Number of cycles since frame started" },
  { "_fTimReadCycles","Number of cycles used by timer reads since frame started" },
  { "_fWsyncCycles",  "Number of cycles skipped by WSYNC since frame started" },
  { "_iCycles",       "Number of cycles of last instruction" },
  { "_inTim",         "Curent INTIM value" },
  { "_scan",          "Current scanline count" },
  { "_scanEnd",       "Scanline count at end of last frame" },
  { "_sCycles",       "Number of cycles in current scanline" },
  { "_timInt",        "Current TIMINT value" },
  { "_timWrapRead",   "Timer read wrapped on this cycle" },
  { "_timWrapWrite",  "Timer write wrapped on this cycle" },
  { "_vBlank",        "Whether vertical blank is enabled (1 or 0)" },
  { "_vSync",         "Whether vertical sync is enabled (1 or 0)" }
  // CPU address access functions:
  /*{ "_lastRead", "last CPU read address" },
  { "_lastWrite", "last CPU write address" },
  { "__lastBaseRead", "last CPU read base address" },
  { "__lastBaseWrite", "last CPU write base address" }*/
} };
// NOLINTEND(bugprone-throwing-static-initialization)
