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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.cxx,v 1.6 2005-06-15 04:30:33 urchlay Exp $
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Debugger::~Debugger()
{
  delete myParser;
  delete myDebugger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Debugger::initialize()
{
  int x = 0,
      y = myConsole->mediaSource().height(),
      w = kDebuggerWidth,
      h = kDebuggerHeight - y;

  delete myBaseDialog;
  myBaseDialog = new DebuggerDialog(myOSystem, this, x, y, w, h);
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
  char buf[255], bbuf[255];

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
  int count = myDebugger->disassemble(myDebugger->pc(), buf);
  for(int i=0; i<count; i++) {
    sprintf(bbuf, "%02x ", readRAM(myDebugger->pc() + i));
    result += bbuf;
  }
  if(count < 3) result += "   ";
  if(count < 2) result += "   ";
  result += " ";
  result += buf;

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
void Debugger::quit()
{
  myOSystem->eventHandler().leaveDebugMode();
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
void Debugger::setEquateList(EquateList *l) {
  myDebugger->setEquateList(l);
}
