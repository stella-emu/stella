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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
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
#include "Settings.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"
#include "StateManager.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "Cart.hxx"

#include "EquateList.hxx"
#include "CpuDebug.hxx"
#include "RamDebug.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"

#include "TiaInfoWidget.hxx"
#include "TiaOutputWidget.hxx"
#include "TiaZoomWidget.hxx"
#include "EditTextWidget.hxx"

#include "RomWidget.hxx"
#include "Expression.hxx"

#include "YaccParser.hxx"

#include "Debugger.hxx"

Debugger* Debugger::myStaticDebugger;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static const string builtin_functions[][3] = {
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
  { "_diff0b", "!(*SWCHB & $40)", "Left difficulty set to B (easy)" },
  { "_diff0a", "*SWCHB & $40", "Left difficulty set to A (hard)" },
  { "_diff1b", "!(*SWCHB & $80)", "Right difficulty set to B (easy)" },
  { "_diff1a", "*SWCHB & $80", "Right difficulty set to A (hard)" },

  // empty string marks end of list, do not remove
  { "", "", "" }
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myParser(NULL),
    myCpuDebug(NULL),
    myRamDebug(NULL),
    myRiotDebug(NULL),
    myTiaDebug(NULL),
    myTiaInfo(NULL),
    myTiaOutput(NULL),
    myTiaZoom(NULL),
    myRom(NULL),
    myEquateList(NULL),
    myBreakPoints(NULL),
    myReadTraps(NULL),
    myWriteTraps(NULL),
    myWidth(1050),
    myHeight(620)
{
  // Get the dialog size
  int w, h;
  myOSystem->settings().getSize("debuggerres", w, h);
  myWidth = BSPF_max(w, 0);
  myHeight = BSPF_max(h, 0);
  myWidth = BSPF_max(myWidth, 1050u);
  myHeight = BSPF_max(myHeight, 620u);
  myOSystem->settings().setSize("debuggerres", myWidth, myHeight);

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

  delete myCpuDebug;
  delete myRamDebug;
  delete myRiotDebug;
  delete myTiaDebug;

  delete myEquateList;
  delete myBreakPoints;
  delete myReadTraps;
  delete myWriteTraps;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  const GUI::Rect& r = getDialogBounds();

  delete myBaseDialog;
  DebuggerDialog *dd = new DebuggerDialog(myOSystem, this,
                           r.left, r.top, r.width(), r.height());
  myBaseDialog = dd;

  myPrompt    = dd->prompt();
  myTiaInfo   = dd->tiaInfo();
  myTiaOutput = dd->tiaOutput();
  myTiaZoom   = dd->tiaZoom();
  myRom       = dd->rom();
  myMessage   = dd->message();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::initializeVideo()
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

  delete myRamDebug;
  myRamDebug = new RamDebug(*this, *myConsole);

  // Register any RAM areas in the Cartridge
  // Zero-page RAM is automatically recognized by RamDebug
  const Cartridge::RamAreaList& areas = myConsole->cartridge().ramAreas();
  for(Cartridge::RamAreaList::const_iterator i = areas.begin(); i != areas.end(); ++i)
    myRamDebug->addRamArea(i->start, i->size, i->roffset, i->woffset);

  delete myRiotDebug;
  myRiotDebug = new RiotDebug(*this, *myConsole);

  delete myTiaDebug;
  myTiaDebug = new TIADebug(*this, *myConsole);

  // Initialize equates and breakpoints to known state
  delete myEquateList;
  myEquateList = new EquateList();
  clearAllBreakPoints();
  clearAllTraps();

  autoLoadSymbols(myOSystem->romFile());
  loadListFile();

  // Make sure cart RAM is added before this is called,
  // otherwise the debugger state won't know about it
  saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start(const string& message, int address)
{
  bool result = myOSystem->eventHandler().enterDebugMode();

  // This must be done *after* we enter debug mode,
  // so the message isn't erased
  ostringstream buf;
  buf << message;
  if(address > -1)
    buf << valueToString(address);

  myMessage->setEditString(buf.str());

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::quit()
{
  myOSystem->eventHandler().leaveDebugMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::autoLoadSymbols(string fileName)
{
  string file = fileName;

  string::size_type pos;
  if( (pos = file.find_last_of('.')) != string::npos )
    file.replace(pos, file.size(), ".sym");
  else
    file += ".sym";

  string ret = myEquateList->loadFile(file);
  //  cerr << "loading syms from file " << file << ": " << ret << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::loadListFile(string f)
{
  char buffer[255];

  if(f == "")
  {
    f = myOSystem->romFile();

    string::size_type pos;
    if( (pos = f.find_last_of('.')) != string::npos )
      f.replace(pos, f.size(), ".lst");
    else
      f += ".lst";
  }

  ifstream in(f.c_str());
  if(!in.is_open())
    return "Unable to read listing from " + f;

  sourceLines.clear();
  int count = 0;
  while( !in.eof() )
  {
    if(!in.getline(buffer, 255))
      break;

    if(strlen(buffer) >= 14 &&
       buffer[0] == ' '     &&
       buffer[7] == ' '     &&
       buffer[8] == ' '     &&
       isxdigit(buffer[9])  &&
       isxdigit(buffer[12]) &&
       BSPF_isblank(buffer[13]))
    {
      count++;
      char addr[5];
      for(int i=0; i<4; i++)
        addr[i] = buffer[9+i];

      for(char *c = buffer; *c != '\0'; c++)
        if(*c == '\t') *c = ' ';

      addr[4] = '\0';
      string a = addr;
      string b = buffer;
      sourceLines.insert(make_pair(a, b));
    }
  }
  in.close();

  return valueToString(count) + " lines loaded from " + f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::getSourceLines(int addr) const
{
  if(sourceLines.size() == 0)
    return "";

  string ret;
  string want = to_hex_16(addr);

  bool found = false;
  pair<ListIter, ListIter> lines = sourceLines.equal_range(want);
  for(ListIter i = lines.first; i != lines.second; i++)
  {
    found = true;
    ret += i->second;
    ret += "\n";
  }

  if(found)
    return ret;
  else
    return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::autoExec()
{
  // autoexec.stella is always run
  const string& autoexec = myOSystem->baseDir() + BSPF_PATH_SEPARATOR +
                           "autoexec.stella";
  myPrompt->print("autoExec():\n" + myParser->exec(autoexec) + "\n");

  // Also, "romname.stella" if present
  string file = myOSystem->romFile();

  string::size_type pos;
  if( (pos = file.find_last_of('.')) != string::npos )
    file.replace(pos, file.size(), ".stella");
  else
    file += ".stella";

  myPrompt->print("autoExec():\n" + myParser->exec(file) + "\n");
  myPrompt->printPrompt();

  // Init builtins
  for(int i = 0; builtin_functions[i][0] != ""; i++)
  {
    // TODO - check this for memory leaks
    int res = YaccParser::parse(builtin_functions[i][1].c_str());
    if(res != 0) cerr << "ERROR in builtin function!" << endl;
    Expression* exp = YaccParser::getResult();
    addFunction(builtin_functions[i][0], builtin_functions[i][1], exp, true);
  }
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
        sprintf(buf, "%02x", value);
      else if(value < 0x10000)
        sprintf(buf, "%04x", value);
      else
        sprintf(buf, "%08x", value);
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
  int pc = myCpuDebug->dPeek(0xfffc);
  myCpuDebug->setPC(pc);
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
  myOSystem->state().saveState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadState(int state)
{
  myOSystem->state().loadState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::step()
{
  saveOldState();

  int cyc = mySystem->cycles();

  unlockState();
  myOSystem->console().tia().updateScanlineByStep();
  lockState();

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

    int cyc = mySystem->cycles();
    int targetPC = myCpuDebug->pc() + 3; // return address

    unlockState();
    myOSystem->console().tia().updateScanlineByTrace(targetPC);
    lockState();

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
const string& Debugger::disassemble(int start, int lines)
{
  static string result;
  ostringstream buffer;
  string cpubuf;

  do {
    buffer << myEquateList->getLabel(start, true, 4) << ": ";

    int count = myCpuDebug->disassemble(start, cpubuf, *myEquateList);
    for(int i = 0; i < count; i++)
      buffer << hex << setw(2) << setfill('0') << peek(start++) << dec;

    if(count < 3) buffer << "   ";
    if(count < 2) buffer << "   ";

    buffer << " " << cpubuf << "\n";
  } while(--lines > 0 && start <= 0xffff);

  result = buffer.str();
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::disassemble(IntArray& addr, StringList& addrLabel,
                           StringList& bytes, StringList& data,
                           int start, int lines)
{
  string cpubuf, tmp;
  char buf[255];

  do
  {
    addrLabel.push_back(myEquateList->getLabel(start, true, 4) + ":");
    addr.push_back(start);

    cpubuf = "";
    int count = myCpuDebug->disassemble(start, cpubuf, *myEquateList);

    tmp = "";
    for(int i=0; i<count; i++) {
      sprintf(buf, "%02x ", peek(start++));
      tmp += buf;
    }
    bytes.push_back(tmp);

    data.push_back(cpubuf);
  }
  while(--lines > 0 && start <= 0xffff);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextScanline(int lines)
{
  saveOldState();

  unlockState();
  myTiaOutput->advanceScanline(lines);
  lockState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames)
{
  saveOldState();

  unlockState();
  myTiaOutput->advance(frames);
  lockState();
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
int Debugger::peek(int addr)
{
  return mySystem->peek(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::dpeek(int addr)
{
  return mySystem->peek(addr) | (mySystem->peek(addr+1) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::showWatches()
{
  return myParser->showWatches();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addLabel(string label, int address)
{
  myEquateList->addEquate(label, address);
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
    myConsole->cartridge().bank(bank);
    myConsole->cartridge().lockBank();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getBank()
{
  return myConsole->cartridge().bank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::bankCount()
{
  return myConsole->cartridge().bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::getCartType()
{
  return myConsole->cartridge().name();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::patchROM(int addr, int value)
{
  return myConsole->cartridge().patch(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveOldState()
{
  myCpuDebug->saveOldState();
  myRamDebug->saveOldState();
  myRiotDebug->saveOldState();
  myTiaDebug->saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setStartState()
{
  // Lock the bus each time the debugger is entered, so we don't disturb anything
  lockState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setQuitState()
{
  // Bus must be unlocked for normal operation when leaving debugger mode
  unlockState();

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
           (int) (0.3 * (dlg.width() - 1030)) : 0);
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
void Debugger::addFunction(const string& name, const string& definition,
                           Expression* exp, bool builtin)
{
  functions.insert(make_pair(name, exp));
  if(!builtin)
    functionDefs.insert(make_pair(name, definition));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::delFunction(const string& name)
{
  FunctionMap::iterator iter = functions.find(name);
  if(iter == functions.end())
    return;

  functions.erase(name);
  delete iter->second;

  FunctionDefMap::iterator def_iter = functionDefs.find(name);
  if(def_iter == functionDefs.end())
    return;

  functionDefs.erase(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Expression* Debugger::getFunction(const string& name)
{
  FunctionMap::iterator iter = functions.find(name);
  if(iter == functions.end())
    return 0;
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::getFunctionDef(const string& name)
{
  FunctionDefMap::iterator iter = functionDefs.find(name);
  if(iter == functionDefs.end())
    return "";
  else
    return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FunctionDefMap Debugger::getFunctionDefMap() const
{
  return functionDefs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::builtinHelp() const
{
  string result;

  for(int i=0; builtin_functions[i][0] != ""; i++)
  {
    result += builtin_functions[i][0];
    result += " {";
    result += builtin_functions[i][1];
    result += "}\n";
    result += "     ";
    result += builtin_functions[i][2];
    result += "\n";
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::saveROM(const string& filename) const
{
  // TODO: error checking
  ofstream *out = new ofstream(filename.c_str(), ios::out | ios::binary);
  bool res = myConsole->cartridge().save(*out);
  delete out;
  return res;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::lockState()
{
  mySystem->lockDataBus();
  myConsole->cartridge().lockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::unlockState()
{
  mySystem->unlockDataBus();
  myConsole->cartridge().unlockBank();
}
