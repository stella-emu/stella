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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FSNode.hxx"
#include "Settings.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"
#include "StateManager.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "Cart.hxx"
#include "TIA.hxx"

#include "CartDebug.hxx"
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

#include "Debugger.hxx"

Debugger* Debugger::myStaticDebugger;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const char* builtin_functions[][3] = {
  // { "name", "definition", "help text" }

  // left joystick:
  { "_joy0left", "!(*SWCHA & $40)", "Left joystick moved left" },
  { "_joy0right", "!(*SWCHA & $80)", "Left joystick moved right" },
  { "_joy0up", "!(*SWCHA & $10)", "Left joystick moved up" },
  { "_joy0down", "!(*SWCHA & $20)", "Left joystick moved down" },
  { "_joy0button", "!(*INPT4 & $80)", "Left joystick button pressed" },

  // right joystick:
  { "_joy1left", "!(*SWCHA & $04)", "Right joystick moved left" },
  { "_joy1right", "!(*SWCHA & $08)", "Right joystick moved right" },
  { "_joy1up", "!(*SWCHA & $01)", "Right joystick moved up" },
  { "_joy1down", "!(*SWCHA & $02)", "Right joystick moved down" },
  { "_joy1button", "!(*INPT5 & $80)", "Right joystick button pressed" },

  // console switches:
  { "_select", "!(*SWCHB & $02)", "Game Select pressed" },
  { "_reset", "!(*SWCHB & $01)", "Game Reset pressed" },
  { "_color", "*SWCHB & $08", "Color/BW set to Color" },
  { "_bw", "!(*SWCHB & $08)", "Color/BW set to BW" },
  { "_diff0b", "!(*SWCHB & $40)", "Left diff. set to B (easy)" },
  { "_diff0a", "*SWCHB & $40", "Left diff. set to A (hard)" },
  { "_diff1b", "!(*SWCHB & $80)", "Right diff. set to B (easy)" },
  { "_diff1a", "*SWCHB & $80", "Right diff. set to A (hard)" },

  // empty string marks end of list, do not remove
  { 0, 0, 0 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Names are defined here, but processed in YaccParser
static const char* pseudo_registers[][2] = {
  // { "name", "help text" }

  { "_bank", "Currently selected bank" },
  { "_rwport", "Address at which a read from a write port occurred" },
  { "_scan", "Current scanline count" },
  { "_fcount", "Number of frames since emulation started" },
  { "_cclocks", "Color clocks on current scanline" },
  { "_vsync", "Whether vertical sync is enabled (1 or 0)" },
  { "_vblank", "Whether vertical blank is enabled (1 or 0)" },

  // empty string marks end of list, do not remove
  { 0, 0 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myDialog(NULL),
    myParser(NULL),
    myCartDebug(NULL),
    myCpuDebug(NULL),
    myRiotDebug(NULL),
    myTiaDebug(NULL),
    myBreakPoints(NULL),
    myReadTraps(NULL),
    myWriteTraps(NULL),
    myWidth(1050),
    myHeight(620),
    myRewindManager(NULL)
{
  // Init parser
  myParser = new DebuggerParser(this);
  myBreakPoints = new PackedBitArray(0x10000);
  myReadTraps = new PackedBitArray(0x10000);
  myWriteTraps = new PackedBitArray(0x10000);

  // Allow access to this object from any class
  // Technically this violates pure OO programming, but since I know
  // there will only be ever one instance of debugger in Stella,
  // I don't care :)
  myStaticDebugger = this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myParser;

  delete myCartDebug;
  delete myCpuDebug;
  delete myRiotDebug;
  delete myTiaDebug;

  delete myBreakPoints;
  delete myReadTraps;
  delete myWriteTraps;

  delete myRewindManager;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  // Get the dialog size
  int w, h;
  myOSystem->settings().getSize("debuggerres", w, h);
  myWidth = BSPF_max(w, 0);
  myHeight = BSPF_max(h, 0);
  myWidth = BSPF_max(myWidth, 1050u);
  myHeight = BSPF_max(myHeight, 620u);
  myOSystem->settings().setSize("debuggerres", myWidth, myHeight);

  const GUI::Rect& r = getDialogBounds();

  delete myBaseDialog;  myBaseDialog = myDialog = NULL;
  myDialog = new DebuggerDialog(myOSystem, this,
      r.left, r.top, r.width(), r.height());
  myBaseDialog = myDialog;

  myRewindManager = new RewindManager(*myOSystem, myDialog->rewindButton());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Debugger::initializeVideo()
{
  const GUI::Rect& r = getDialogBounds();

  string title = string("Stella ") + STELLA_VERSION + ": Debugger mode";
  return myOSystem->frameBuffer().initialize(title, r.width(), r.height());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setConsole(Console* console)
{
  assert(console);

  // Keep pointers to these items for efficiency
  myConsole = console;
  mySystem = &(myConsole->system());

  // Create debugger subsystems
  delete myCpuDebug;
  myCpuDebug = new CpuDebug(*this, *myConsole);

  delete myCartDebug;
  myCartDebug = new CartDebug(*this, *myConsole, *myOSystem);

  delete myRiotDebug;
  myRiotDebug = new RiotDebug(*this, *myConsole);

  delete myTiaDebug;
  myTiaDebug = new TIADebug(*this, *myConsole);

  // Initialize breakpoints to known state
  clearAllBreakPoints();
  clearAllTraps();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start(const string& message, int address)
{
  if(myOSystem->eventHandler().enterDebugMode())
  {
    // This must be done *after* we enter debug mode,
    // so the message isn't erased
    ostringstream buf;
    buf << message;
    if(address > -1)
      buf << valueToString(address);

    myDialog->message().setEditString(buf.str());
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::startWithFatalError(const string& message)
{
  if(myOSystem->eventHandler().enterDebugMode())
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
    myOSystem->eventHandler().handleEvent(Event::LauncherMode, 1);
  else
    myOSystem->eventHandler().leaveDebugMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::autoExec()
{
  ostringstream buf;

  // autoexec.stella is always run
  FilesystemNode autoexec(myOSystem->baseDir() + "autoexec.stella");
  buf << "autoExec():" << endl
      << myParser->exec(autoexec) << endl;

  // Also, "romname.stella" if present
  string file = myOSystem->romFile();
  string::size_type pos;
  if( (pos = file.find_last_of('.')) != string::npos )
    file.replace(pos, file.size(), ".stella");
  else
    file += ".stella";

  FilesystemNode romname(file);
  buf << myParser->exec(romname) << endl;

  // Init builtins
  for(int i = 0; builtin_functions[i][0] != 0; i++)
  {
    // TODO - check this for memory leaks
    int res = YaccParser::parse(builtin_functions[i][1]);
    if(res != 0) cerr << "ERROR in builtin function!" << endl;
    Expression* exp = YaccParser::getResult();
    addFunction(builtin_functions[i][0], builtin_functions[i][1], exp, true);
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::run(const string& command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::valueToString(int value, BaseFormat outputBase)
{
  char buf[32];

  if(outputBase == kBASE_DEFAULT)
    outputBase = myParser->base();

  switch(outputBase)
  {
    case kBASE_2:
      if(value < 0x100)
        Debugger::to_bin(value, 8, buf);
      else
        Debugger::to_bin(value, 16, buf);
      break;

    case kBASE_10:
      if(value < 0x100)
        sprintf(buf, "%3d", value);
      else
        sprintf(buf, "%5d", value);
      break;

    case kBASE_16_4:
      strcpy(buf, Debugger::to_hex_4(value));
      break;

    case kBASE_16:
    default:
      if(value < 0x100)
        sprintf(buf, "%02X", value);
      else if(value < 0x10000)
        sprintf(buf, "%04X", value);
      else
        sprintf(buf, "%08X", value);
      break;
  }

  return string(buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::invIfChanged(int reg, int oldReg)
{
  string ret;

  bool changed = reg != oldReg;
  if(changed) ret += "\177";
  ret += valueToString(reg);
  if(changed) ret += "\177";

  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reset()
{
  myCpuDebug->setPC(dpeek(0xfffc));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Element 0 of args is the address. The remaining elements are the data
   to poke, starting at the given address.
*/
const string Debugger::setRAM(IntArray& args)
{
  char buf[10];

  int count = args.size();
  int address = args[0];
  for(int i=1; i<count; i++)
    mySystem->poke(address++, args[i]);

  string ret = "changed ";
  sprintf(buf, "%d", count-1);
  ret += buf;
  ret += " location";
  if(count != 0)
    ret += "s";
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveState(int state)
{
  mySystem->clearDirtyPages();

  unlockBankswitchState();
  myOSystem->state().saveState(state);
  lockBankswitchState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadState(int state)
{
  mySystem->clearDirtyPages();

  unlockBankswitchState();
  myOSystem->state().loadState(state);
  lockBankswitchState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::step()
{
  saveOldState();
  mySystem->clearDirtyPages();

  int cyc = mySystem->cycles();

  unlockBankswitchState();
  myOSystem->console().tia().updateScanlineByStep();
  lockBankswitchState();

  return mySystem->cycles() - cyc;
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
  if(mySystem->peek(myCpuDebug->pc()) == 32)
  {
    saveOldState();
    mySystem->clearDirtyPages();

    int cyc = mySystem->cycles();
    int targetPC = myCpuDebug->pc() + 3; // return address

    unlockBankswitchState();
    myOSystem->console().tia().updateScanlineByTrace(targetPC);
    lockBankswitchState();

    return mySystem->cycles() - cyc;
  }
  else
    return step();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleBreakPoint(int bp)
{
  mySystem->m6502().setBreakPoints(myBreakPoints);
  if(bp < 0) bp = myCpuDebug->pc();
  myBreakPoints->toggle(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setBreakPoint(int bp, bool set)
{
  mySystem->m6502().setBreakPoints(myBreakPoints);
  if(bp < 0) bp = myCpuDebug->pc();
  if(set)
    myBreakPoints->set(bp);
  else
    myBreakPoints->clear(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::breakPoint(int bp)
{
  if(bp < 0) bp = myCpuDebug->pc();
  return myBreakPoints->isSet(bp) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleReadTrap(int t)
{
  mySystem->m6502().setTraps(myReadTraps, myWriteTraps);
  myReadTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleWriteTrap(int t)
{
  mySystem->m6502().setTraps(myReadTraps, myWriteTraps);
  myWriteTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleTrap(int t)
{
  toggleReadTrap(t);
  toggleWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::readTrap(int t)
{
  return myReadTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::writeTrap(int t)
{
  return myWriteTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::cycles()
{
  return mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextScanline(int lines)
{
  saveOldState();
  mySystem->clearDirtyPages();

  unlockBankswitchState();
  while(lines)
  {
    myOSystem->console().tia().updateScanline();
    --lines;
  }
  lockBankswitchState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames)
{
  saveOldState();
  mySystem->clearDirtyPages();

  unlockBankswitchState();
  while(frames)
  {
    myOSystem->console().tia().update();
    --frames;
  }
  lockBankswitchState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::rewindState()
{
  mySystem->clearDirtyPages();

  unlockBankswitchState();
  bool result = myRewindManager->rewindState();
  lockBankswitchState();

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints()
{
  delete myBreakPoints;
  myBreakPoints = new PackedBitArray(0x10000);
  mySystem->m6502().setBreakPoints(NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllTraps() 
{
  delete myReadTraps;
  delete myWriteTraps;
  myReadTraps = new PackedBitArray(0x10000);
  myWriteTraps = new PackedBitArray(0x10000);
  mySystem->m6502().setTraps(NULL, NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::showWatches()
{
  return myParser->showWatches();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reloadROM()
{
  myOSystem->createConsole();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setBank(int bank)
{
  if(myConsole->cartridge().bankCount() > 1)
  {
    myConsole->cartridge().unlockBank();
    bool status = myConsole->cartridge().bank(bank);
    myConsole->cartridge().lockBank();
    return status;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::patchROM(int addr, int value)
{
  return myConsole->cartridge().patch(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveOldState(bool addrewind)
{
  myCartDebug->saveOldState();
  myCpuDebug->saveOldState();
  myRiotDebug->saveOldState();
  myTiaDebug->saveOldState();

  // Add another rewind level to the Undo list
  if(addrewind)  myRewindManager->addState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setStartState()
{
  // Lock the bus each time the debugger is entered, so we don't disturb anything
  lockBankswitchState();

  // Start a new rewind list
  myRewindManager->clear();

  // Save initial state, but don't add it to the rewind list
  saveOldState(false);

  // Set the 're-disassemble' flag, but don't do it until the next scheduled time 
  myDialog->rom().invalidate(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setQuitState()
{
  // Bus must be unlocked for normal operation when leaving debugger mode
  unlockBankswitchState();

  // execute one instruction on quit. If we're
  // sitting at a breakpoint/trap, this will get us past it.
  // Somehow this feels like a hack to me, but I don't know why
  //	if(myBreakPoints->isSet(myCpuDebug->pc()))
  mySystem->m6502().execute(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect Debugger::getDialogBounds() const
{
  // The dialog bounds are the actual size of the entire dialog container
  GUI::Rect r(0, 0, myWidth, myHeight);
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect Debugger::getTiaBounds() const
{
  // The area showing the TIA image (NTSC and PAL supported, up to 260 lines)
  GUI::Rect r(0, 0, 320, 260);
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect Debugger::getRomBounds() const
{
  // The ROM area is the full area to the right of the tabs
  const GUI::Rect& dialog = getDialogBounds();
  const GUI::Rect& status = getStatusBounds();

  int x1 = status.right + 1;
  int y1 = 0;
  int x2 = dialog.right;
  int y2 = dialog.bottom;
  GUI::Rect r(x1, y1, x2, y2);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect Debugger::getStatusBounds() const
{
  // The status area is the full area to the right of the TIA image
  // extending as far as necessary
  // 30% of any space above 1030 pixels will be allocated to this area
  const GUI::Rect& dlg = getDialogBounds();
  const GUI::Rect& tia = getTiaBounds();

  int x1 = tia.right + 1;
  int y1 = 0;
  int x2 = tia.right + 225 + (dlg.width() > 1030 ?
           (int) (0.35 * (dlg.width() - 1030)) : 0);
  int y2 = tia.bottom;
  GUI::Rect r(x1, y1, x2, y2);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect Debugger::getTabBounds() const
{
  // The tab area is the full area below the TIA image
  const GUI::Rect& dialog = getDialogBounds();
  const GUI::Rect& tia    = getTiaBounds();
  const GUI::Rect& status = getStatusBounds();

  int x1 = 0;
  int y1 = tia.bottom + 1;
  int x2 = status.right + 1;
  int y2 = dialog.bottom;
  GUI::Rect r(x1, y1, x2, y2);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::addFunction(const string& name, const string& definition,
                           Expression* exp, bool builtin)
{
  functions.insert(make_pair(name, exp));
  if(!builtin)
    functionDefs.insert(make_pair(name, definition));

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::delFunction(const string& name)
{
  FunctionMap::iterator iter = functions.find(name);
  if(iter == functions.end())
    return false;

  functions.erase(name);
  delete iter->second;

  FunctionDefMap::iterator def_iter = functionDefs.find(name);
  if(def_iter == functionDefs.end())
    return false;

  functionDefs.erase(name);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Expression* Debugger::getFunction(const string& name) const
{
  FunctionMap::const_iterator iter = functions.find(name);
  if(iter == functions.end())
    return 0;
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Debugger::getFunctionDef(const string& name) const
{
  FunctionDefMap::const_iterator iter = functionDefs.find(name);
  if(iter == functionDefs.end())
    return EmptyString;
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FunctionDefMap Debugger::getFunctionDefMap() const
{
  return functionDefs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::builtinHelp() const
{
  ostringstream buf;
  uInt16 len, c_maxlen = 0, i_maxlen = 0;

  // Get column widths for aligned output (functions)
  for(int i = 0; builtin_functions[i][0] != 0; ++i)
  {
    len = strlen(builtin_functions[i][0]);
    if(len > c_maxlen)  c_maxlen = len;
    len = strlen(builtin_functions[i][1]);
    if(len > i_maxlen)  i_maxlen = len;
  }

  buf << setfill(' ') << endl << "Built-in functions:" << endl;
  for(int i = 0; builtin_functions[i][0] != 0; ++i)
  {
    buf << setw(c_maxlen) << left << builtin_functions[i][0]
        << setw(2) << right << "{"
        << setw(i_maxlen) << left << builtin_functions[i][1]
        << setw(4) << "}"
        << builtin_functions[i][2]
        << endl;
  }

  // Get column widths for aligned output (pseudo-registers)
  c_maxlen = 0;
  for(int i = 0; pseudo_registers[i][0] != 0; ++i)
  {
    len = strlen(pseudo_registers[i][0]);
    if(len > c_maxlen)  c_maxlen = len;
  }

  buf << endl << "Pseudo-registers:" << endl;
  for(int i = 0; pseudo_registers[i][0] != 0; ++i)
  {
    buf << setw(c_maxlen) << left << pseudo_registers[i][0]
        << setw(2) << " "
        << setw(i_maxlen) << left << pseudo_registers[i][1]
        << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::getCompletions(const char* in, StringList& list) const
{
  FunctionMap::const_iterator iter;
  for(iter = functions.begin(); iter != functions.end(); ++iter)
  {
    const char* l = iter->first.c_str();
    if(BSPF_strncasecmp(l, in, strlen(in)) == 0)
      list.push_back(l);
  }

  for(int i = 0; pseudo_registers[i][0] != 0; ++i)
    if(BSPF_strncasecmp(pseudo_registers[i][0], in, strlen(in)) == 0)
      list.push_back(pseudo_registers[i][0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::saveROM(const string& filename) const
{
  string path = AbstractFilesystemNode::getAbsolutePath(filename, "~", "a26");
  FilesystemNode node(path);

  ofstream out(node.getPath(true).c_str(), ios::out | ios::binary);
  if(out.is_open() && myConsole->cartridge().save(out))
    return node.getPath(false);
  else
    return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::lockBankswitchState()
{
  mySystem->lockDataBus();
  myConsole->cartridge().lockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::unlockBankswitchState()
{
  mySystem->unlockDataBus();
  myConsole->cartridge().unlockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::RewindManager::RewindManager(OSystem& system, ButtonWidget& button)
  : myOSystem(system),
    myRewindButton(button),
    mySize(0),
    myTop(0)
{
  for(int i = 0; i < MAX_SIZE; ++i)
    myStateList[i] = (Serializer*) NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::RewindManager::~RewindManager()
{
  for(int i = 0; i < MAX_SIZE; ++i)
    delete myStateList[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::RewindManager::addState()
{
  // Create a new Serializer object if we need one
  if(myStateList[myTop] == NULL)
    myStateList[myTop] = new Serializer();
  Serializer& s = *(myStateList[myTop]);

  if(s.isValid())
  {
    s.reset();
    if(myOSystem.state().saveState(s) && myOSystem.console().tia().saveDisplay(s))
    {
      // Are we still within the allowable size, or are we overwriting an item?
      mySize++; if(mySize > MAX_SIZE) mySize = MAX_SIZE;

      myTop = (myTop + 1) % MAX_SIZE;
      myRewindButton.setEnabled(true);
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::RewindManager::rewindState()
{
  if(mySize > 0)
  {
    mySize--;
    myTop = myTop == 0 ? MAX_SIZE - 1 : myTop - 1;
    Serializer& s = *(myStateList[myTop]);

    s.reset();
    myOSystem.state().loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    if(mySize == 0)
      myRewindButton.setEnabled(false);

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::RewindManager::isEmpty()
{
  return mySize == 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::RewindManager::clear()
{
  for(int i = 0; i < MAX_SIZE; ++i)
    if(myStateList[i] != NULL)
      myStateList[i]->reset();

  myTop = mySize = 0;

  // We use Widget::clearFlags here instead of Widget::setEnabled(),
  // since the latter implies an immediate draw/update, but this method
  // might be called before any UI exists
  // TODO - fix this deficiency in the UI core; we shouldn't have to worry
  //        about such things at this level
  myRewindButton.clearFlags(WIDGET_ENABLED);
}
