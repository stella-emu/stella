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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <map>

#include "bspf.hxx"

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "FSNode.hxx"
#include "Settings.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "Cart.hxx"

#include "CartDebug.hxx"
#include "CartDebugWidget.hxx"
#include "CartRamWidget.hxx"
#include "CpuDebug.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"

#include "TiaInfoWidget.hxx"
#include "TiaOutputWidget.hxx"
#include "TiaZoomWidget.hxx"
#include "EditTextWidget.hxx"

#include "RomWidget.hxx"
#include "Expression.hxx"
#include "PackedBitArray.hxx"
#include "YaccParser.hxx"

#include "TIA.hxx"
#include "Debugger.hxx"

Debugger* Debugger::myStaticDebugger = nullptr;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem& osystem, Console& console)
  : DialogContainer(osystem),
    myConsole(console),
    mySystem(console.system()),
    myDialog(nullptr),
    myWidth(DebuggerDialog::kSmallFontMinW),
    myHeight(DebuggerDialog::kSmallFontMinH)
{
  // Init parser
  myParser = make_unique<DebuggerParser>(*this, osystem.settings());

  // Create debugger subsystems
  myCpuDebug  = make_unique<CpuDebug>(*this, myConsole);
  myCartDebug = make_unique<CartDebug>(*this, myConsole, osystem);
  myRiotDebug = make_unique<RiotDebug>(*this, myConsole);
  myTiaDebug  = make_unique<TIADebug>(*this, myConsole);

  // Allow access to this object from any class
  // Technically this violates pure OO programming, but since I know
  // there will only be ever one instance of debugger in Stella,
  // I don't care :)
  myStaticDebugger = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  const GUI::Size& s = myOSystem.settings().getSize("dbg.res");
  const GUI::Size& d = myOSystem.frameBuffer().desktopSize();
  myWidth = s.w;  myHeight = s.h;

  // The debugger dialog is resizable, within certain bounds
  // We check those bounds now
  myWidth  = std::max(myWidth,  uInt32(DebuggerDialog::kSmallFontMinW));
  myHeight = std::max(myHeight, uInt32(DebuggerDialog::kSmallFontMinH));
  myWidth  = std::min(myWidth,  uInt32(d.w));
  myHeight = std::min(myHeight, uInt32(d.h));

  myOSystem.settings().setValue("dbg.res", GUI::Size(myWidth, myHeight));

  delete myBaseDialog;  myBaseDialog = myDialog = nullptr;
  myDialog = new DebuggerDialog(myOSystem, *this, 0, 0, myWidth, myHeight);
  myBaseDialog = myDialog;

  myCartDebug->setDebugWidget(&(myDialog->cartDebug()));

  saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Debugger::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION + ": Debugger mode";
  return myOSystem.frameBuffer().createDisplay(title, myWidth, myHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start(const string& message, int address, bool read)
{
  if(myOSystem.eventHandler().enterDebugMode())
  {
    // This must be done *after* we enter debug mode,
    // so the message isn't erased
    ostringstream buf;
    buf << message;
    if(address > -1)
      buf << cartDebug().getLabel(address, read, 4);
    myDialog->message().setText(buf.str());
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::startWithFatalError(const string& message)
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
void Debugger::quit(bool exitrom)
{
  if(exitrom)
    myOSystem.eventHandler().handleEvent(Event::LauncherMode, 1);
  else
    myOSystem.eventHandler().leaveDebugMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::autoExec(StringList* history)
{
  ostringstream buf;

  // autoexec.script is always run
  FilesystemNode autoexec(myOSystem.baseDir() + "autoexec.script");
  buf << "autoExec():" << endl
      << myParser->exec(autoexec, history) << endl;

  // Also, "romname.script" if present
  FilesystemNode romname(myOSystem.romFile().getPathWithExt(".script"));
  buf << myParser->exec(romname, history) << endl;

  // Init builtins
  for(uInt32 i = 0; i < NUM_BUILTIN_FUNCS; ++i)
  {
    // TODO - check this for memory leaks
    int res = YaccParser::parse(ourBuiltinFunctions[i].defn);
    if(res == 0)
      addFunction(ourBuiltinFunctions[i].name, ourBuiltinFunctions[i].defn,
                  YaccParser::getResult(), true);
    else
      cerr << "ERROR in builtin function!" << endl;
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PackedBitArray& Debugger::breakPoints() const
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
const string Debugger::run(const string& command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::invIfChanged(int reg, int oldReg)
{
  string ret;

  bool changed = reg != oldReg;
  if(changed) ret += "\177";
  ret += Common::Base::toString(reg, Common::Base::F_16_2);
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
string Debugger::setRAM(IntArray& args)
{
  ostringstream buf;

  int count = int(args.size());
  int address = args[0];
  for(int i = 1; i < count; ++i)
    mySystem.poke(address++, args[i]);

  buf << "changed " << (count-1) << " location";
  if(count != 2)
    buf << "s";
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveState(int state)
{
  // Saving a state is implicitly a read-only operation, so we keep the
  // system locked, so no changes can occur
  myOSystem.state().saveState(state);
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
int Debugger::step()
{
  saveOldState();

  uInt64 startCycle = mySystem.cycles();

  unlockSystem();
  myOSystem.console().tia().updateScanlineByStep().flushLineCache();
  lockSystem();

  addState("step");
  return int(mySystem.cycles() - startCycle);
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
  if(mySystem.peek(myCpuDebug->pc()) == 32)
  {
    saveOldState();

    uInt64 startCycle = mySystem.cycles();
    int targetPC = myCpuDebug->pc() + 3; // return address

    unlockSystem();
    myOSystem.console().tia().updateScanlineByTrace(targetPC).flushLineCache();
    lockSystem();

    addState("trace");
    return int(mySystem.cycles() - startCycle);
  }
  else
    return step();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleBreakPoint(uInt16 bp)
{
  breakPoints().initialize();
  breakPoints().toggle(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setBreakPoint(uInt16 bp, bool set)
{
  breakPoints().initialize();
  if(set) breakPoints().set(bp);
  else    breakPoints().clear(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::breakPoint(uInt16 bp)
{
  return breakPoints().isSet(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addReadTrap(uInt16 t)
{
  readTraps().initialize();
  readTraps().add(t);
}

void Debugger::addWriteTrap(uInt16 t)
{
  writeTraps().initialize();
  writeTraps().add(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addTrap(uInt16 t)
{
  addReadTrap(t);
  addWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::removeReadTrap(uInt16 t)
{
  readTraps().initialize();
  readTraps().remove(t);
}

void Debugger::removeWriteTrap(uInt16 t)
{
  writeTraps().initialize();
  writeTraps().remove(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::removeTrap(uInt16 t)
{
  removeReadTrap(t);
  removeWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::readTrap(uInt16 t)
{
  return readTraps().isInitialized() && readTraps().isSet(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::writeTrap(uInt16 t)
{
  return writeTraps().isInitialized() && writeTraps().isSet(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::peek(uInt16 addr, uInt8 flags)
{
  return mySystem.peek(addr, flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::dpeek(uInt16 addr, uInt8 flags)
{
  return uInt16(mySystem.peek(addr, flags) | (mySystem.peek(addr+1, flags) << 8));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::poke(uInt16 addr, uInt8 value, uInt8 flags)
{
  mySystem.poke(addr, value, flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502& Debugger::m6502() const
{
  return mySystem.m6502();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::peekAsInt(int addr, uInt8 flags)
{
  return mySystem.peek(uInt16(addr), flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::dpeekAsInt(int addr, uInt8 flags)
{
  return mySystem.peek(uInt16(addr), flags) |
      (mySystem.peek(uInt16(addr+1), flags) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getAccessFlags(uInt16 addr) const
{
  return mySystem.getAccessFlags(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setAccessFlags(uInt16 addr, uInt8 flags)
{
  mySystem.setAccessFlags(addr, flags);
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
  ostringstream buf;
  buf << "scanline + " << lines;

  saveOldState();

  unlockSystem();
  while(lines)
  {
    myOSystem.console().tia().updateScanline();
    --lines;
  }
  lockSystem();

  addState(buf.str());
  myOSystem.console().tia().flushLineCache();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames)
{
  ostringstream buf;
  buf << "frame + " << frames;

  saveOldState();

  unlockSystem();
  while(frames)
  {
    myOSystem.console().tia().update();
    --frames;
  }
  lockSystem();

  addState(buf.str());
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

  uInt64 startCycles = myOSystem.console().tia().cycles();
  uInt16 winds = r.windStates(numStates, unwind);
  message = r.getUnitString(myOSystem.console().tia().cycles() - startCycles);

  lockSystem();

  updateRewindbuttons(r);
  return winds;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::rewindStates(const uInt16 numStates, string& message)
{
  return windStates(numStates, false, message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Debugger::unwindStates(const uInt16 numStates, string& message)
{
  return windStates(numStates, true, message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints()
{
  breakPoints().clearAll();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllTraps()
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
int Debugger::stringToValue(const string& stringval)
{
  return myParser->decipher_arg(stringval);
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
void Debugger::addState(string rewindMsg)
{
  // Add another rewind level to the Time Machine buffer
  RewindManager& r = myOSystem.state().rewindManager();
  r.addState(rewindMsg);
  updateRewindbuttons(r);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setStartState()
{
  // Lock the bus each time the debugger is entered, so we don't disturb anything
  lockSystem();

  // Save initial state and add it to the rewind list (except when in currently rewinding)
  RewindManager& r = myOSystem.state().rewindManager();
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
  saveOldState();

  // Bus must be unlocked for normal operation when leaving debugger mode
  unlockSystem();

  // execute one instruction on quit. If we're
  // sitting at a breakpoint/trap, this will get us past it.
  // Somehow this feels like a hack to me, but I don't know why
  //	if(breakPoints().isSet(myCpuDebug->pc()))
  mySystem.m6502().execute(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::addFunction(const string& name, const string& definition,
                           Expression* exp, bool builtin)
{
  myFunctions.emplace(name, unique_ptr<Expression>(exp));
  myFunctionDefs.emplace(name, definition);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::isBuiltinFunction(const string& name)
{
  for(uInt32 i = 0; i < NUM_BUILTIN_FUNCS; ++i)
    if(name == ourBuiltinFunctions[i].name)
      return true;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::delFunction(const string& name)
{
  const auto& iter = myFunctions.find(name);
  if(iter == myFunctions.end())
    return false;

  // We never want to delete built-in functions
  if(isBuiltinFunction(name))
      return false;

  myFunctions.erase(name);

  const auto& def_iter = myFunctionDefs.find(name);
  if(def_iter == myFunctionDefs.end())
    return false;

  myFunctionDefs.erase(name);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Expression& Debugger::getFunction(const string& name) const
{
  const auto& iter = myFunctions.find(name);
  return iter != myFunctions.end() ? *(iter->second.get()) : EmptyExpression;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Debugger::getFunctionDef(const string& name) const
{
  const auto& iter = myFunctionDefs.find(name);
  return iter != myFunctionDefs.end() ? iter->second : EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FunctionDefMap Debugger::getFunctionDefMap() const
{
  return myFunctionDefs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::builtinHelp() const
{
  ostringstream buf;
  uInt16 len, c_maxlen = 0, i_maxlen = 0;

  // Get column widths for aligned output (functions)
  for(uInt32 i = 0; i < NUM_BUILTIN_FUNCS; ++i)
  {
    len = ourBuiltinFunctions[i].name.size();
    if(len > c_maxlen)  c_maxlen = len;
    len = ourBuiltinFunctions[i].defn.size();
    if(len > i_maxlen)  i_maxlen = len;
  }

  buf << std::setfill(' ') << endl << "Built-in functions:" << endl;
  for(uInt32 i = 0; i < NUM_BUILTIN_FUNCS; ++i)
  {
    buf << std::setw(c_maxlen) << std::left << ourBuiltinFunctions[i].name
        << std::setw(2) << std::right << "{"
        << std::setw(i_maxlen) << std::left << ourBuiltinFunctions[i].defn
        << std::setw(4) << "}"
        << ourBuiltinFunctions[i].help
        << endl;
  }

  // Get column widths for aligned output (pseudo-registers)
  c_maxlen = 0;
  for(uInt32 i = 0; i < NUM_PSEUDO_REGS; ++i)
  {
    len = ourPseudoRegisters[i].name.size();
    if(len > c_maxlen)  c_maxlen = len;
  }

  buf << endl << "Pseudo-registers:" << endl;
  for(uInt32 i = 0; i < NUM_PSEUDO_REGS; ++i)
  {
    buf << std::setw(c_maxlen) << std::left << ourPseudoRegisters[i].name
        << std::setw(2) << " "
        << std::setw(i_maxlen) << std::left << ourPseudoRegisters[i].help
        << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::getCompletions(const char* in, StringList& list) const
{
  // skip if filter equals "_" only
  if(!BSPF::equalsIgnoreCase(in, "_"))
  {
    for(const auto& iter : myFunctions)
    {
      const char* l = iter.first.c_str();
      if(BSPF::matches(l, in))
        list.push_back(l);
    }

    for(uInt32 i = 0; i < NUM_PSEUDO_REGS; ++i)
      if(BSPF::matches(ourPseudoRegisters[i].name, in))
        list.push_back(ourPseudoRegisters[i].name);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::lockSystem()
{
  mySystem.lockDataBus();
  myConsole.cartridge().lockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::unlockSystem()
{
  mySystem.unlockDataBus();
  myConsole.cartridge().unlockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::canExit() const
{
  return myDialogStack.top() == baseDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::BuiltinFunction Debugger::ourBuiltinFunctions[NUM_BUILTIN_FUNCS] = {
  // left joystick:
  { "_joy0left",    "!(*SWCHA & $40)", "Left joystick moved left" },
  { "_joy0right",   "!(*SWCHA & $80)", "Left joystick moved right" },
  { "_joy0up",      "!(*SWCHA & $10)", "Left joystick moved up" },
  { "_joy0down",    "!(*SWCHA & $20)", "Left joystick moved down" },
  { "_joy0button",  "!(*INPT4 & $80)", "Left joystick button pressed" },

  // right joystick:
  { "_joy1left",    "!(*SWCHA & $04)", "Right joystick moved left" },
  { "_joy1right",   "!(*SWCHA & $08)", "Right joystick moved right" },
  { "_joy1up",      "!(*SWCHA & $01)", "Right joystick moved up" },
  { "_joy1down",    "!(*SWCHA & $02)", "Right joystick moved down" },
  { "_joy1button",  "!(*INPT5 & $80)", "Right joystick button pressed" },

  // console switches:
  { "_select",    "!(*SWCHB & $02)",  "Game Select pressed" },
  { "_reset",     "!(*SWCHB & $01)",  "Game Reset pressed" },
  { "_color",     "*SWCHB & $08",     "Color/BW set to Color" },
  { "_bw",        "!(*SWCHB & $08)",  "Color/BW set to BW" },
  { "_diff0b",    "!(*SWCHB & $40)",  "Left diff. set to B (easy)" },
  { "_diff0a",    "*SWCHB & $40",     "Left diff. set to A (hard)" },
  { "_diff1b",    "!(*SWCHB & $80)",  "Right diff. set to B (easy)" },
  { "_diff1a",    "*SWCHB & $80",     "Right diff. set to A (hard)" }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Names are defined here, but processed in YaccParser
Debugger::PseudoRegister Debugger::ourPseudoRegisters[NUM_PSEUDO_REGS] = {
  { "_bank",      "Currently selected bank" },
  { "_cclocks",   "Color clocks on current scanline" },
  { "_cycleshi",  "Higher 32 bits of number of cycles since emulation started" },
  { "_cycleslo",  "Lower 32 bits of number of cycles since emulation started" },
  { "_fcount",    "Number of frames since emulation started" },
  { "_fcycles",   "Number of cycles since frame started" },
  { "_icycles",    "Number of cycles of last instruction" },
  { "_rwport",    "Address at which a read from a write port occurred" },
  { "_scan",      "Current scanline count" },
  { "_scycles",   "Number of cycles in current scanline" },
  { "_vblank",    "Whether vertical blank is enabled (1 or 0)" },
  { "_vsync",     "Whether vertical sync is enabled (1 or 0)" }
  // CPU address access functions:
  /*{ "__lastread", "last CPU read address" },
  { "__lastwrite", "last CPU write address" },*/
};
