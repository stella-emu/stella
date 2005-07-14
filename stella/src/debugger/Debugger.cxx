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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.cxx,v 1.63 2005-07-14 15:13:14 stephena Exp $
//============================================================================

#include "bspf.hxx"

#include <sstream>

#include "Version.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "DebuggerDialog.hxx"
#include "DebuggerParser.hxx"

#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"

#include "EquateList.hxx"
#include "CpuDebug.hxx"
#include "RamDebug.hxx"
#include "TIADebug.hxx"

#include "Debugger.hxx"

Debugger* Debugger::myStaticDebugger;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myParser(NULL),
    myCpuDebug(NULL),
    myRamDebug(NULL),
    myTiaDebug(NULL),
    equateList(NULL),
    breakPoints(NULL),
    readTraps(NULL),
    writeTraps(NULL)
{
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
  int x, y, w, h;
  getDialogBounds(&x, &y, &w, &h);

  delete myBaseDialog;
  DebuggerDialog *dd = new DebuggerDialog(myOSystem, this, x, y, w, h);
  myPrompt = dd->prompt();
  myBaseDialog = dd;

  // set up any breakpoint that was on the command line
  // (and remove the key from the settings, so they won't get set again)
  string initBreak = myOSystem->settings().getString("break");
  if(initBreak != "") run("break " + initBreak);
  myOSystem->settings().setString("break", "", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initializeVideo()
{
  int x, y, w, h;
  getDialogBounds(&x, &y, &w, &h);

  string title = string("Stella ") + STELLA_VERSION + ": Debugger mode";
  myOSystem->frameBuffer().initialize(title, w, y+h, false);
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

  // Let the parser know about the new subsystems (so YaccParser can use them)
// FIXME
//  myParser->updateDebugger(this, myConsole);

  autoLoadSymbols(myOSystem->romFile());

  saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start()
{
  return myOSystem->eventHandler().enterDebugMode();
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
const string Debugger::invIfChanged(int reg, int oldReg) {
	string ret;

	bool changed = !(reg == oldReg);
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
  result += "\n  Cyc:";
  sprintf(buf, "%d", mySystem->cycles());
  result += buf;
  result += " Scan:";
  sprintf(buf, "%d", myTiaDebug->scanlines());
  result += buf;
  result += " Frame:";
  sprintf(buf, "%d", myTiaDebug->frameCount());
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

	if(b[7])
		out[0] = 'N';
	else
		out[0] = 'n';

	if(b[6])
		out[1] = 'V';
	else
		out[1] = 'v';

	out[2] = '-';

	if(b[4])
		out[3] = 'B';
	else
		out[3] = 'b';

	if(b[3])
		out[4] = 'D';
	else
		out[4] = 'd';

	if(b[2])
		out[5] = 'I';
	else
		out[5] = 'i';

	if(b[1])
		out[6] = 'Z';
	else
		out[6] = 'z';

	if(b[0])
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
  myOSystem->eventHandler().saveState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::loadState(int state)
{
  myOSystem->eventHandler().loadState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::step()
{
  saveOldState();

  int cyc = mySystem->cycles();
  mySystem->unlockDataBus();
  mySystem->m6502().execute(1);
  mySystem->lockDataBus();

  // FIXME - this doesn't work yet, pending a partial rewrite of TIA class
  myTiaDebug->updateTIA();
  myOSystem->frameBuffer().refreshTIA(true);
  ///////////////////////////////////////////////////////

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

// FIXME: TIA framebuffer should be updated during tracing!

int Debugger::trace()
{

  // 32 is the 6502 JSR instruction:
  if(mySystem->peek(myCpuDebug->pc()) == 32) {
    saveOldState();

    int cyc = mySystem->cycles();
    int targetPC = myCpuDebug->pc() + 3; // return address

    mySystem->unlockDataBus();

    while(myCpuDebug->pc() != targetPC)
      mySystem->m6502().execute(1);

    mySystem->lockDataBus();

    // FIXME - this doesn't work yet, pending a partial rewrite of TIA class
    myTiaDebug->updateTIA();
    myOSystem->frameBuffer().refreshTIA(true);
    ///////////////////////////////////////////////////////

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
  } while(--lines > 0);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::nextFrame(int frames) {
  saveOldState();
  mySystem->unlockDataBus();
  myOSystem->frameBuffer().advance(frames);
  mySystem->lockDataBus();
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
int Debugger::setHeight(int height)
{
  if(height < kDebuggerLines)
    height = kDebuggerLines;

  myOSystem->settings().setInt("debugheight", height);

  // Inform the debugger dialog about the new size
  quit();
  resizeDialog();
  start();

  return height;
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
void Debugger::reloadROM() {
  myOSystem->createConsole( myOSystem->romFile() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::setBank(int bank) {
  if(myConsole->cartridge().bankCount() > 1) {
    myConsole->cartridge().bank(bank);
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
  return myConsole->cartridge().name();
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
  mySystem->lockDataBus();

// FIXME - do we want this to print each time?
//         I think it was needed before because of some bugs I've fixed
//  myPrompt->printPrompt();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setQuitState()
{
  // Bus must be unlocked for normal operation when leaving debugger mode
  mySystem->unlockDataBus();

  // execute one instruction on quit, IF we're
  // sitting at a breakpoint. This will get us past it.
  // Somehow this feels like a hack to me, but I don't know why
  // FIXME: do this for traps, too
  if(breakPoints->isSet(myCpuDebug->pc()))
    mySystem->m6502().execute(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::getDialogBounds(int* x, int* y, int* w, int* h)
{
  int userHeight = myOSystem->settings().getInt("debugheight");
  if(userHeight < kDebuggerLines)
  {
    userHeight = kDebuggerLines;
    myOSystem->settings().setInt("debugheight", userHeight);
  }
  userHeight = (userHeight + 3) * kDebuggerLineHeight - 8;

  *x = 0;
  *y = myConsole->mediaSource().height();
  *w = kDebuggerWidth;
  *h = userHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::resizeDialog()
{
cerr << "Debugger::resizeDialog()\n";
}
