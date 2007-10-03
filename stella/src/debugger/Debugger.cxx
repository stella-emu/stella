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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.cxx,v 1.117 2007-10-03 21:41:17 stephena Exp $
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

#include "EquateList.hxx"
#include "CpuDebug.hxx"
#include "RamDebug.hxx"
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
	{ "_diff0a", "!(*SWCHB & $40)", "Right difficulty set to A (easy)" },
	{ "_diff0b", "*SWCHB & $40", "Right difficulty set to B (hard)" },
	{ "_diff1a", "!(*SWCHB & $80)", "Right difficulty set to A (easy)" },
	{ "_diff1b", "*SWCHB & $80", "Right difficulty set to B (hard)" },

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
    myTiaDebug(NULL),
    myTiaInfo(NULL),
    myTiaOutput(NULL),
    myTiaZoom(NULL),
    myRom(NULL),
    equateList(NULL),
    breakPoints(NULL),
    readTraps(NULL),
    writeTraps(NULL),
    myWidth(1030),
    myHeight(690)
{
  // Get the dialog size
  int w, h;
  myOSystem->settings().getSize("debuggerres", w, h);
  myWidth = BSPF_max(w, 0);
  myHeight = BSPF_max(h, 0);
  myWidth = BSPF_max(myWidth, 1030u);
  myHeight = BSPF_max(myHeight, 690u);
  myOSystem->settings().setSize("debuggerres", myWidth, myHeight);

  // Init parser
  myParser = new DebuggerParser(this);
  equateList = new EquateList();
  breakPoints = new PackedBitArray(0x10000);
  readTraps = new PackedBitArray(0x10000);
  writeTraps = new PackedBitArray(0x10000);

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
  delete myTiaDebug;

  delete equateList;
  delete breakPoints;
  delete readTraps;
  delete writeTraps;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  GUI::Rect r = getDialogBounds();

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
void Debugger::initializeVideo()
{
  GUI::Rect r = getDialogBounds();

  string title = string("Stella ") + STELLA_VERSION + ": Debugger mode";
  myOSystem->frameBuffer().initialize(title, r.width(), r.height());
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
  myCpuDebug = new CpuDebug(this, myConsole);

  delete myRamDebug;
  myRamDebug = new RamDebug(this, myConsole);

  delete myTiaDebug;
  myTiaDebug = new TIADebug(this, myConsole);

  autoLoadSymbols(myOSystem->romFile());
  loadListFile();

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
void Debugger::autoLoadSymbols(string fileName) {
	string file = fileName;

	string::size_type pos;
	if( (pos = file.find_last_of('.')) != string::npos ) {
		file.replace(pos, file.size(), ".sym");
	} else {
		file += ".sym";
	}
	string ret = equateList->loadFile(file);
	//	cerr << "loading syms from file " << file << ": " << ret << endl;
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
const string Debugger::getSourceLines(int addr)
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
void Debugger::autoExec() {
	// autoexec.stella is always run
	myPrompt->print("autoExec():\n" +
			myParser->exec(
				myOSystem->baseDir() +
				BSPF_PATH_SEPARATOR +
				"autoexec.stella") +
			"\n");

	// also, "romname.stella" if present
	string file = myOSystem->romFile();

	string::size_type pos;
	if( (pos = file.find_last_of('.')) != string::npos ) {
		file.replace(pos, file.size(), ".stella");
	} else {
		file += ".stella";
	}
	myPrompt->print("autoExec():\n" + myParser->exec(file) + "\n");
	myPrompt->printPrompt();

	// init builtins
	for(int i=0; builtin_functions[i][0] != ""; i++) {
		string f = builtin_functions[i][1];
		int res = YaccParser::parse(f.c_str());
		if(res != 0) cerr << "ERROR in builtin function!" << endl;
		Expression *exp = YaccParser::getResult();
		addFunction(builtin_functions[i][0], builtin_functions[i][1], exp, true);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::run(const string& command)
{
  return myParser->run(command);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::valueToString(int value, BaseFormat outputBase)
{
  char rendered[32];

  if(outputBase == kBASE_DEFAULT)
    outputBase = myParser->base();

  switch(outputBase)
  {
    case kBASE_2:
      if(value < 0x100)
        sprintf(rendered, Debugger::to_bin_8(value));
      else
        sprintf(rendered, Debugger::to_bin_16(value));
      break;

    case kBASE_10:
      if(value < 0x100)
        sprintf(rendered, "%3d", value);
      else
        sprintf(rendered, "%5d", value);
      break;

    case kBASE_16_4:
      sprintf(rendered, Debugger::to_hex_4(value));
      break;

    case kBASE_16:
    default:
      if(value < 0x100)
        sprintf(rendered, Debugger::to_hex_8(value));
      else
        sprintf(rendered, Debugger::to_hex_16(value));
      break;
  }

  return string(rendered);
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
const string Debugger::cpuState()
{
  string result;
  char buf[255];

  CpuState state    = (CpuState&) myCpuDebug->getState();
  CpuState oldstate = (CpuState&) myCpuDebug->getOldState();

  result += "\nPC=";
  result += invIfChanged(state.PC, oldstate.PC);
  result += " A=";
  result += invIfChanged(state.A, oldstate.A);
  result += " X=";
  result += invIfChanged(state.X, oldstate.X);
  result += " Y=";
  result += invIfChanged(state.Y, oldstate.Y);
  result += " S=";
  result += invIfChanged(state.SP, oldstate.SP);
  result += " P=";
  result += invIfChanged(state.PS, oldstate.PS);
  result += "/";
  formatFlags(state.PSbits, buf);
  result += buf;
  result += "\n  FrameCyc:";
  sprintf(buf, "%d", mySystem->cycles());
  result += buf;
  result += " Frame:";
  sprintf(buf, "%d", myTiaDebug->frameCount());
  result += buf;
  result += " ScanLine:";
  sprintf(buf, "%d", myTiaDebug->scanlines());
  result += buf;
  result += " Clk/Pix/Cyc:";
  int clk = myTiaDebug->clocksThisLine();
  sprintf(buf, "%d/%d/%d", clk, clk-68, clk/3);
  result += buf;
  result += " 6502Ins:";
  sprintf(buf, "%d", mySystem->m6502().totalInstructionCount());
  result += buf;
  result += "\n  ";

  result += disassemble(state.PC, 1);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* The timers, joysticks, and switches can be read via peeks, so
   I didn't write a separate RIOTDebug class. */
const string Debugger::riotState() {
	string ret;

	// TODO: inverse video for changed regs. Core needs to track this.
	// TODO: keyboard controllers?

	for(int i=0x280; i<0x284; i++) {
		ret += valueToString(i);
		ret += "/";
		ret += equates()->getFormatted(i, 2);
		ret += "=";
		ret += valueToString(mySystem->peek(i));
		ret += " ";
	}
	ret += "\n";

	// These are squirrely: some symbol files will define these as
	// 0x284-0x287. Doesn't actually matter, these registers repeat
   // every 16 bytes.
	ret += valueToString(0x294);
	ret += "/TIM1T=";
	ret += valueToString(mySystem->peek(0x294));
	ret += " ";

	ret += valueToString(0x295);
	ret += "/TIM8T=";
	ret += valueToString(mySystem->peek(0x295));
	ret += " ";

	ret += valueToString(0x296);
	ret += "/TIM64T=";
	ret += valueToString(mySystem->peek(0x296));
	ret += " ";

	ret += valueToString(0x297);
	ret += "/TIM1024T=";
	ret += valueToString(mySystem->peek(0x297));
	ret += "\n";

	ret += "Left/P0diff: ";
	ret += (mySystem->peek(0x282) & 0x40) ? "hard/A" : "easy/B";
	ret += "   ";

	ret += "Right/P1diff: ";
	ret += (mySystem->peek(0x282) & 0x80) ? "hard/A" : "easy/B";
	ret += "\n";

	ret += "TVType: ";
	ret += (mySystem->peek(0x282) & 0x8) ? "Color" : "B&W";
	ret += "   Switches: ";
	ret += (mySystem->peek(0x282) & 0x2) ? "-" : "+";
	ret += "select  ";
	ret += (mySystem->peek(0x282) & 0x1) ? "-" : "+";
	ret += "reset";
	ret += "\n";

	// Yes, the fire buttons are in the TIA, but we might as well
	// show them here for convenience.
	ret += "Left/P0 stick:  ";
	ret += (mySystem->peek(0x280) & 0x80) ? "" : "right ";
	ret += (mySystem->peek(0x280) & 0x40) ? "" : "left ";
	ret += (mySystem->peek(0x280) & 0x20) ? "" : "down ";
	ret += (mySystem->peek(0x280) & 0x10) ? "" : "up ";
	ret += ((mySystem->peek(0x280) & 0xf0) == 0xf0) ? "(no directions) " : "";
	ret += (mySystem->peek(0x03c) & 0x80) ? "" : "(button) ";
	ret += "\n";
	ret += "Right/P1 stick: ";
	ret += (mySystem->peek(0x280) & 0x08) ? "" : "right ";
	ret += (mySystem->peek(0x280) & 0x04) ? "" : "left ";
	ret += (mySystem->peek(0x280) & 0x02) ? "" : "down ";
	ret += (mySystem->peek(0x280) & 0x01) ? "" : "up ";
	ret += ((mySystem->peek(0x280) & 0x0f) == 0x0f) ? "(no directions) " : "";
	ret += (mySystem->peek(0x03d) & 0x80) ? "" : "(button) ";

	//ret += "\n"; // caller will add

	return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reset() {
	int pc = myCpuDebug->dPeek(0xfffc);
	myCpuDebug->setPC(pc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::formatFlags(BoolArray& b, char *out) {
	// NV-BDIZC

	if(myCpuDebug->n())
		out[0] = 'N';
	else
		out[0] = 'n';

	if(myCpuDebug->v())
		out[1] = 'V';
	else
		out[1] = 'v';

	out[2] = '-';

	if(myCpuDebug->b())
		out[3] = 'B';
	else
		out[3] = 'b';

	if(myCpuDebug->d())
		out[4] = 'D';
	else
		out[4] = 'd';

	if(myCpuDebug->i())
		out[5] = 'I';
	else
		out[5] = 'i';

	if(myCpuDebug->z())
		out[6] = 'Z';
	else
		out[6] = 'z';

	if(myCpuDebug->c())
		out[7] = 'C';
	else
		out[7] = 'c';

	out[8] = '\0';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Element 0 of args is the address. The remaining elements are the data
   to poke, starting at the given address.
*/
const string Debugger::setRAM(IntArray& args) {
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
/* Warning: this method really is for dumping *RAM*, not ROM or I/O! */
const string Debugger::dumpRAM()
{
  string result;
  char buf[128];
  int bytesPerLine;
  int start = kRamStart, len = kRamSize;

  switch(myParser->base())
  {
    case kBASE_16:
    case kBASE_10:
      bytesPerLine = 0x10;
      break;

    case kBASE_2:
      bytesPerLine = 0x04;
      break;

    case kBASE_DEFAULT:
    default:
      return DebuggerParser::red("invalid base, this is a BUG");
  }

  RamState state    = (RamState&) myRamDebug->getState();
  RamState oldstate = (RamState&) myRamDebug->getOldState();
  for (uInt8 i = 0x00; i < len; i += bytesPerLine)
  {
    sprintf(buf, "%.2x: ", start+i);
    result += buf;

    for (uInt8 j = 0; j < bytesPerLine; j++)
    {
      result += invIfChanged(state.ram[i+j], oldstate.ram[i+j]);
      result += " ";

      if(j == 0x07) result += " ";
    }
    result += "\n";
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::dumpTIA()
{
  string result;
  char buf[128];

  sprintf(buf, "%.2x: ", 0);
  result += buf;
  for (uInt8 j = 0; j < 0x010; j++)
  {
    sprintf(buf, "%.2x ", mySystem->peek(j));
    result += buf;

    if(j == 0x07) result += "- ";
  }

  result += "\n";
  result += myTiaDebug->state();

  return result;
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
  //	mySystem->unlockDataBus();
  unlockState();
  myOSystem->console().mediaSource().updateScanlineByStep();
  //	mySystem->lockDataBus();
  unlockState();

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
  if(mySystem->peek(myCpuDebug->pc()) == 32) {
    saveOldState();

    int cyc = mySystem->cycles();
    int targetPC = myCpuDebug->pc() + 3; // return address

    //	mySystem->unlockDataBus();
	 unlockState();
    myOSystem->console().mediaSource().updateScanlineByTrace(targetPC);
    //	mySystem->lockDataBus();
	 lockState();

    return mySystem->cycles() - cyc;
  } else {
    return step();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList *Debugger::equates() {
  return equateList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleBreakPoint(int bp) {
  mySystem->m6502().setBreakPoints(breakPoints);
  if(bp < 0) bp = myCpuDebug->pc();
  breakPoints->toggle(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setBreakPoint(int bp, bool set)
{
  mySystem->m6502().setBreakPoints(breakPoints);
  if(bp < 0) bp = myCpuDebug->pc();
  if(set)
    breakPoints->set(bp);
  else
    breakPoints->clear(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::breakPoint(int bp) {
  if(bp < 0) bp = myCpuDebug->pc();
  return breakPoints->isSet(bp) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleReadTrap(int t) {
  mySystem->m6502().setTraps(readTraps, writeTraps);
  readTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleWriteTrap(int t) {
  mySystem->m6502().setTraps(readTraps, writeTraps);
  writeTraps->toggle(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleTrap(int t) {
  toggleReadTrap(t);
  toggleWriteTrap(t);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::readTrap(int t) {
  return readTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::writeTrap(int t) {
  return writeTraps->isSet(t) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::cycles() {
  return mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Debugger::disassemble(int start, int lines) {
  char buf[255], bbuf[255];
  static string result;

  result = "";
  do {
    const char *label = equateList->getFormatted(start, 4);

    result += label;
    result += ": ";

    int count = myCpuDebug->disassemble(start, buf, equateList);

    for(int i=0; i<count; i++) {
      sprintf(bbuf, "%02x ", peek(start++));
      result += bbuf;
    }

    if(count < 3) result += "   ";
    if(count < 2) result += "   ";

    result += " ";
    result += buf;
    result += "\n";
  } while(--lines > 0 && start <= 0xffff);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::disassemble(IntArray& addr, StringList& addrLabel, 
                           StringList& bytes, StringList& data,
                           int start, int lines)
{
  char buf[255], bbuf[255];
  string tmp;

  do
  {
    tmp = equateList->getFormatted(start, 4);
    addrLabel.push_back(tmp + ":");
    addr.push_back(start);

    int count = myCpuDebug->disassemble(start, buf, equateList);

    tmp = "";
    for(int i=0; i<count; i++) {
      sprintf(bbuf, "%02x ", peek(start++));
      tmp += bbuf;
    }
    bytes.push_back(tmp);

    data.push_back(buf);
  }
  while(--lines > 0 && start <= 0xffff);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextScanline(int lines)
{
  saveOldState();
  // mySystem->unlockDataBus();
  unlockState();
  myTiaOutput->advanceScanline(lines);
  // mySystem->lockDataBus();
  lockState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames)
{
  saveOldState();
  //	mySystem->unlockDataBus();
  unlockState();
  myTiaOutput->advance(frames);
  //	mySystem->lockDataBus();
  lockState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints() {
  delete breakPoints;
  breakPoints = new PackedBitArray(0x10000);
  mySystem->m6502().setBreakPoints(NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllTraps() {
  delete readTraps;
  delete writeTraps;
  readTraps = new PackedBitArray(0x10000);
  writeTraps = new PackedBitArray(0x10000);
  mySystem->m6502().setTraps(NULL, NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::peek(int addr) {
  return mySystem->peek(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::dpeek(int addr) {
  return mySystem->peek(addr) | (mySystem->peek(addr+1) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::showWatches() {
	return myParser->showWatches();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addLabel(string label, int address) {
	equateList->addEquate(label, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reloadROM()
{
  myOSystem->createConsole();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setBank(int bank) {
  if(myConsole->cartridge().bankCount() > 1) {
    myConsole->cartridge().unlockBank();
    myConsole->cartridge().bank(bank);
    myConsole->cartridge().lockBank();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getBank() {
  return myConsole->cartridge().bank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::bankCount() {
  return myConsole->cartridge().bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char *Debugger::getCartType() {
  return myConsole->cartridge().name().c_str();
// FIXME - maybe whatever is calling this should use a string instead
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::patchROM(int addr, int value) {
  return myConsole->cartridge().patch(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::saveOldState()
{
  myCpuDebug->saveOldState();
  myRamDebug->saveOldState();
  myTiaDebug->saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setStartState()
{
  // Lock the bus each time the debugger is entered, so we don't disturb anything
  //	mySystem->lockDataBus();
  lockState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setQuitState()
{
  // Bus must be unlocked for normal operation when leaving debugger mode
  //	mySystem->unlockDataBus();
  unlockState();

  // execute one instruction on quit. If we're
  // sitting at a breakpoint/trap, this will get us past it.
  // Somehow this feels like a hack to me, but I don't know why
  //	if(breakPoints->isSet(myCpuDebug->pc()))
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
  GUI::Rect dialog = getDialogBounds();
  GUI::Rect status = getStatusBounds();

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
  GUI::Rect dlg = getDialogBounds();
  GUI::Rect tia = getTiaBounds();

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
  GUI::Rect dialog = getDialogBounds();
  GUI::Rect tia    = getTiaBounds();
  GUI::Rect status = getStatusBounds();

  int x1 = 0;
  int y1 = tia.bottom + 1;
  int x2 = status.right + 1;
  int y2 = dialog.bottom;
  GUI::Rect r(x1, y1, x2, y2);

  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::addFunction(string name, string definition, Expression *exp, bool builtin) {
	functions.insert(make_pair(name, exp));
	if(!builtin) functionDefs.insert(make_pair(name, definition));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::delFunction(string name) {
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
Expression *Debugger::getFunction(string name) {
	FunctionMap::iterator iter = functions.find(name);
	if(iter == functions.end())
		return 0;
	else
		return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::getFunctionDef(string name) {
	FunctionDefMap::iterator iter = functionDefs.find(name);
	if(iter == functionDefs.end())
		return "";
	else
		return iter->second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FunctionDefMap Debugger::getFunctionDefMap() {
	return functionDefs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::builtinHelp() {
  string result;

  for(int i=0; builtin_functions[i][0] != ""; i++) {
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
bool Debugger::saveROM(string filename)
{
  // TODO: error checking
  ofstream *out = new ofstream(filename.c_str(), ios::out | ios::binary);
  bool res = myConsole->cartridge().save(*out);
  delete out;
  return res;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::lockState() {
  mySystem->lockDataBus();
  myConsole->cartridge().lockBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::unlockState() {
  mySystem->unlockDataBus();
  myConsole->cartridge().unlockBank();
}
