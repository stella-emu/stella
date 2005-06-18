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
// $Id: Debugger.cxx,v 1.16 2005-06-18 15:45:05 urchlay Exp $
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
#include "D6502.hxx"

#include "Debugger.hxx"
#include "EquateList.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::Debugger(OSystem* osystem)
  : DialogContainer(osystem),
    myConsole(NULL),
    mySystem(NULL),
    myParser(NULL),
    myDebugger(NULL)
{
  // Init parser
  myParser = new DebuggerParser(this);
  equateList = new EquateList();
  breakPoints = new PackedBitArray(0x10000);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myParser;
  delete myDebugger;
  delete equateList;
  delete breakPoints;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  int x = 0,
      y = myConsole->mediaSource().height(),
      w = kDebuggerWidth,
      h = kDebuggerHeight - y;

  delete myBaseDialog;
  DebuggerDialog *dd = new DebuggerDialog(myOSystem, this, x, y, w, h);
  myPrompt = dd->prompt();
  myBaseDialog = dd;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initializeVideo()
{
  string title = string("Stella version ") + STELLA_VERSION + ": Debugger mode";
  myOSystem->frameBuffer().initialize(title, kDebuggerWidth, kDebuggerHeight, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setConsole(Console* console)
{
  assert(console);

  // Keep pointers to these items for efficiency
  myConsole = console;
  mySystem = &(myConsole->system());

  // Create a new 6502 debugger for this console
  delete myDebugger;
  myDebugger = new D6502(mySystem);

  autoLoadSymbols(myOSystem->romFile());
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
const string Debugger::state()
{
  string result;
  char buf[255];

  //cerr << "state(): pc is " << myDebugger->pc() << endl;
  result += "\nPC=";
  result += to_hex_16(myDebugger->pc());
  result += " A=";
  result += to_hex_8(myDebugger->a());
  result += " X=";
  result += to_hex_8(myDebugger->x());
  result += " Y=";
  result += to_hex_8(myDebugger->y());
  result += " S=";
  result += to_hex_8(myDebugger->sp());
  result += " P=";
  result += to_hex_8(myDebugger->ps());
  result += "/";
  formatFlags(myDebugger->ps(), buf);
  result += buf;
  result += "  Cycle ";
  sprintf(buf, "%d", mySystem->cycles());
  result += buf;
  result += "\n  ";

  result += disassemble(myDebugger->pc(), 1);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::reset() {
	int pc = myDebugger->dPeek(0xfffc);
	myDebugger->pc(pc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::formatFlags(int f, char *out) {
	// NV-BDIZC

	if(f & 128)
		out[0] = 'N';
	else
		out[0] = 'n';

	if(f & 64)
		out[1] = 'V';
	else
		out[1] = 'v';

	out[2] = '-';

	if(f & 16)
		out[3] = 'B';
	else
		out[3] = 'b';

	if(f & 8)
		out[4] = 'D';
	else
		out[4] = 'd';

	if(f & 4)
		out[5] = 'I';
	else
		out[5] = 'i';

	if(f & 2)
		out[6] = 'Z';
	else
		out[6] = 'z';

	if(f & 1)
		out[7] = 'C';
	else
		out[7] = 'c';

	out[8] = '\0';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Debugger::readRAM(uInt16 addr)
{
  return mySystem->peek(addr + kRamStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::writeRAM(uInt16 addr, uInt8 value)
{
  mySystem->poke(addr + kRamStart, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::setRAM(int argCount, int *args) {
  char buf[10];

  int address = args[0];
  for(int i=1; i<argCount; i++)
    mySystem->poke(address++, args[i]);

  string ret = "changed ";
  sprintf(buf, "%d", argCount-1);
  ret += buf;
  ret += " location";
  if(argCount > 2)
    ret += "s";
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string Debugger::dumpRAM(uInt16 start)
{
  string result;
  char buf[128];

  for (uInt8 i = 0x00; i < kRamStart; i += 0x10)
  {
    sprintf(buf, "%.4x: ", start+i);
    result += buf;

    for (uInt8 j = 0; j < 0x010; j++)
    {
      sprintf(buf, "%.2x ", mySystem->peek(start+i+j));
      result += buf;

      if(j == 0x07) result += "- ";
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

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::start()
{
  if(myOSystem->eventHandler().state() != EventHandler::S_DEBUGGER) {
    myOSystem->eventHandler().enterDebugMode();
    myPrompt->printPrompt();
    return true;
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::quit()
{
  if(myOSystem->eventHandler().state() == EventHandler::S_DEBUGGER) {
    // execute one instruction on quit, IF we're
    // sitting at a breakpoint. This will get us past it.
    // Somehow this feels like a hack to me, but I don't know why
    if(breakPoints->isSet(myDebugger->pc()))
      mySystem->m6502().execute(1);
    myOSystem->eventHandler().leaveDebugMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::step()
{
  mySystem->m6502().execute(1);
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

void Debugger::trace()
{
  // 32 is the 6502 JSR instruction:
  if(mySystem->peek(myDebugger->pc()) == 32) {
    int targetPC = myDebugger->pc() + 3; // return address
    while(myDebugger->pc() != targetPC)
      mySystem->m6502().execute(1);
  } else {
    step();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setA(int a) {
  myDebugger->a(a);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setX(int x) {
  myDebugger->x(x);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setY(int y) {
  myDebugger->y(y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setS(int sp) {
  myDebugger->sp(sp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::setPC(int pc) {
  myDebugger->pc(pc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // NV-BDIZC
void Debugger::toggleC() {
  myDebugger->ps( myDebugger->ps() ^ 0x01 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleZ() {
  myDebugger->ps( myDebugger->ps() ^ 0x02 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleD() {
  myDebugger->ps( myDebugger->ps() ^ 0x08 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleV() {
  myDebugger->ps( myDebugger->ps() ^ 0x40 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleN() {
  myDebugger->ps( myDebugger->ps() ^ 0x80 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EquateList *Debugger::equates() {
  return equateList;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::toggleBreakPoint(int bp) {
  mySystem->m6502().setBreakPoints(breakPoints);
  if(bp < 0) bp = myDebugger->pc();
  breakPoints->toggle(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Debugger::breakPoint(int bp) {
  if(bp < 0) bp = myDebugger->pc();
  return breakPoints->isSet(bp) != 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Debugger::getPC() {
  return myDebugger->pc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Debugger::disassemble(int start, int lines) {
  char buf[255], bbuf[255];
  string result;

  do {
    char *label = equateList->getFormatted(start, 4);

    result += label;
    result += ": ";

    int count = myDebugger->disassemble(start, buf, equateList);

    for(int i=0; i<count; i++) {
      sprintf(bbuf, "%02x ", readRAM(start++));
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
void Debugger::nextFrame() {
  myOSystem->frameBuffer().advance();
  myBaseDialog->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::clearAllBreakPoints() {
  delete breakPoints;
  breakPoints = new PackedBitArray(0x10000);
  mySystem->m6502().setBreakPoints(NULL);
}
