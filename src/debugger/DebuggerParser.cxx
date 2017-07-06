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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <fstream>
#include "bspf.hxx"

#include "Dialog.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"
#include "TiaOutputWidget.hxx"
#include "DebuggerParser.hxx"
#include "YaccParser.hxx"
#include "M6502.hxx"
#include "Expression.hxx"
#include "FSNode.hxx"
#include "PromptWidget.hxx"
#include "RomWidget.hxx"
#include "ProgressDialog.hxx"
#include "PackedBitArray.hxx"
#include "Vec.hxx"

#include "Base.hxx"
using Common::Base;
using std::hex;
using std::dec;
using std::setfill;
using std::setw;
using std::right;

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif

#include "DebuggerParser.hxx"


// TODO - use C++ streams instead of nasty C-strings and pointers

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerParser::DebuggerParser(Debugger& d, Settings& s)
  : debugger(d),
    settings(s),
    argCount(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// main entry point: PromptWidget calls this method.
string DebuggerParser::run(const string& command)
{
#if 0
  // this was our parser test code. Left for reference.
  static Expression *lastExpression;

  // special case: parser testing
  if(strncmp(command.c_str(), "expr ", 5) == 0) {
    delete lastExpression;
    commandResult = "parser test: status==";
    int status = YaccParser::parse(command.c_str() + 5);
    commandResult += debugger.valueToString(status);
    commandResult += ", result==";
    if(status == 0) {
      lastExpression = YaccParser::getResult();
      commandResult += debugger.valueToString(lastExpression->evaluate());
    } else {
      //  delete lastExpression; // NO! lastExpression isn't valid (not 0 either)
                                // It's the result of casting the last token
                                // to Expression* (because of yacc's union).
                                // As such, we can't and don't need to delete it
                                // (However, it means yacc leaks memory on error)
      commandResult += "ERROR - ";
      commandResult += YaccParser::errorMessage();
    }
    return commandResult;
  }

  if(command == "expr") {
    if(lastExpression)
      commandResult = "result==" + debugger.valueToString(lastExpression->evaluate());
    else
      commandResult = "no valid expr";
    return commandResult;
  }
#endif

  string verb;
  getArgs(command, verb);
  commandResult.str("");

  for(int i = 0; i < kNumCommands; ++i)
  {
    if(BSPF::equalsIgnoreCase(verb, commands[i].cmdString))
    {
      if(validateArgs(i))
        commands[i].executor(this);

      if(commands[i].refreshRequired)
        debugger.myBaseDialog->loadConfig();

      return commandResult.str();
    }
  }

  return "No such command (try \"help\")";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::exec(const FilesystemNode& file)
{
  if(file.exists())
  {
    ifstream in(file.getPath());
    if(!in.is_open())
      return red("autoexec file \'" + file.getShortPath() + "\' not found");

    ostringstream buf;
    int count = 0;
    string command;
    while( !in.eof() )
    {
      if(!getline(in, command))
        break;

      run(command);
      count++;
    }
    buf << "Executed " << count << " commands from \""
        << file.getShortPath() << "\"";

    return buf.str();
  }
  else
    return red("autoexec file \'" + file.getShortPath() + "\' not found");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Completion-related stuff:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::getCompletions(const char* in, StringList& completions) const
{
  // cerr << "Attempting to complete \"" << in << "\"" << endl;
  for(int i = 0; i < kNumCommands; ++i)
  {
    if(BSPF::startsWithIgnoreCase(commands[i].cmdString.c_str(), in))
      completions.push_back(commands[i].cmdString);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Evaluate expression. Expressions always evaluate to a 16-bit value if
// they're valid, or -1 if they're not.
// decipher_arg may be called by the GUI as needed. It is also called
// internally by DebuggerParser::run()
int DebuggerParser::decipher_arg(const string& str)
{
  bool derefByte=false, derefWord=false, lobyte=false, hibyte=false, bin=false, dec=false;
  int result;
  string arg = str;

  Base::Format defaultBase = Base::format();

  if(defaultBase == Base::F_2) {
    bin=true; dec=false;
  } else if(defaultBase == Base::F_10) {
    bin=false; dec=true;
  } else {
    bin=false; dec=false;
  }

  if(arg.substr(0, 1) == "*") {
    derefByte = true;
    arg.erase(0, 1);
  } else if(arg.substr(0, 1) == "@") {
    derefWord = true;
    arg.erase(0, 1);
  }

  if(arg.substr(0, 1) == "<") {
    lobyte = true;
    arg.erase(0, 1);
  } else if(arg.substr(0, 1) == ">") {
    hibyte = true;
    arg.erase(0, 1);
  }

  if(arg.substr(0, 1) == "\\") {
    dec = false;
    bin = true;
    arg.erase(0, 1);
  } else if(arg.substr(0, 1) == "#") {
    dec = true;
    bin = false;
    arg.erase(0, 1);
  } else if(arg.substr(0, 1) == "$") {
    dec = false;
    bin = false;
    arg.erase(0, 1);
  }

  // Special cases (registers):
  const CpuState& state = static_cast<const CpuState&>(debugger.cpuDebug().getState());
  if(arg == "a") result = state.A;
  else if(arg == "x") result = state.X;
  else if(arg == "y") result = state.Y;
  else if(arg == "p") result = state.PS;
  else if(arg == "s") result = state.SP;
  else if(arg == "pc" || arg == ".") result = state.PC;
  else { // Not a special, must be a regular arg: check for label first
    const char* a = arg.c_str();
    result = debugger.cartDebug().getAddress(arg);

    if(result < 0) { // if not label, must be a number
      if(bin) { // treat as binary
        result = 0;
        while(*a != '\0') {
          result <<= 1;
          switch(*a++) {
            case '1':
              result++;
              break;

            case '0':
              break;

            default:
              return -1;
          }
        }
      } else if(dec) {
        result = 0;
        while(*a != '\0') {
          int digit = (*a++) - '0';
          if(digit < 0 || digit > 9)
            return -1;

          result = (result * 10) + digit;
        }
      } else { // must be hex.
        result = 0;
        while(*a != '\0') {
          int hex = -1;
          char d = *a++;
          if(d >= '0' && d <= '9')  hex = d - '0';
          else if(d >= 'a' && d <= 'f') hex = d - 'a' + 10;
          else if(d >= 'A' && d <= 'F') hex = d - 'A' + 10;
          if(hex < 0)
            return -1;

          result = (result << 4) + hex;
        }
      }
    }
  }

  if(lobyte) result &= 0xff;
  else if(hibyte) result = (result >> 8) & 0xff;

  // dereference if we're supposed to:
  if(derefByte) result = debugger.peek(result);
  if(derefWord) result = debugger.dpeek(result);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::showWatches()
{
  ostringstream buf;
  for(uInt32 i = 0; i < watches.size(); i++)
  {
    if(watches[i] != "")
    {
      // Clear the args, since we're going to pass them to eval()
      argStrings.clear();
      args.clear();

      argCount = 1;
      argStrings.push_back(watches[i]);
      args.push_back(decipher_arg(argStrings[0]));
      if(args[0] < 0)
        buf << "BAD WATCH " << (i+1) << ": " << argStrings[0] << endl;
      else
        buf << " watch #" << (i+1) << " (" << argStrings[0] << ") -> " << eval() << endl;
    }
  }
  return buf.str();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private methods below
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DebuggerParser::getArgs(const string& command, string& verb)
{
  int state = kIN_COMMAND, i = 0, length = int(command.length());
  string curArg = "";
  verb = "";

  argStrings.clear();
  args.clear();

  // cerr << "Parsing \"" << command << "\"" << ", length = " << command.length() << endl;

  // First, pick apart string into space-separated tokens.
  // The first token is the command verb, the rest go in an array
  do
  {
    char c = command[i++];
    switch(state)
    {
      case kIN_COMMAND:
        if(c == ' ')
          state = kIN_SPACE;
        else
          verb += c;
        break;
      case kIN_SPACE:
        if(c == '{')
          state = kIN_BRACE;
        else if(c != ' ') {
          state = kIN_ARG;
          curArg += c;
        }
        break;
      case kIN_BRACE:
        if(c == '}') {
          state = kIN_SPACE;
          argStrings.push_back(curArg);
          //  cerr << "{" << curArg << "}" << endl;
          curArg = "";
        } else {
          curArg += c;
        }
        break;
      case kIN_ARG:
        if(c == ' ') {
          state = kIN_SPACE;
          argStrings.push_back(curArg);
          curArg = "";
        } else {
          curArg += c;
        }
        break;
    }  // switch(state)
  }
  while(i < length);

  // Take care of the last argument, if there is one
  if(curArg != "")
    argStrings.push_back(curArg);

  argCount = uInt32(argStrings.size());

  for(uInt32 arg = 0; arg < argCount; ++arg)
  {
    if(!YaccParser::parse(argStrings[arg].c_str()))
    {
      unique_ptr<Expression> expr(YaccParser::getResult());
      args.push_back(expr->evaluate());
    }
    else
      args.push_back(-1);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DebuggerParser::validateArgs(int cmd)
{
  // cerr << "entering validateArgs(" << cmd << ")" << endl;
  bool required = commands[cmd].parmsRequired;
  parameters* p = commands[cmd].parms;

  if(argCount == 0)
  {
    if(required)
    {
      commandResult.str(red("missing required argument(s)"));
      return false; // needed args. didn't get 'em.
    }
    else
      return true;  // no args needed, no args got
  }

  // Figure out how many arguments are required by the command
  uInt32 count = 0, argRequiredCount = 0;
  while(*p != kARG_END_ARGS && *p != kARG_MULTI_BYTE)
  {
    count++;
    p++;
  }

  // Evil hack: some commands intentionally take multiple arguments
  // In this case, the required number of arguments is unbounded
  argRequiredCount = (*p == kARG_END_ARGS) ? count : argCount;

  p = commands[cmd].parms;
  uInt32 curCount = 0;

  do {
    if(curCount >= argCount)
      break;

    uInt32 curArgInt  = args[curCount];
    string& curArgStr = argStrings[curCount];

    switch(*p)
    {
      case kARG_WORD:
        if(curArgInt > 0xffff)
        {
          commandResult.str(red("invalid word argument (must be 0-$ffff)"));
          return false;
        }
        break;

      case kARG_BYTE:
        if(curArgInt > 0xff)
        {
          commandResult.str(red("invalid byte argument (must be 0-$ff)"));
          return false;
        }
        break;

      case kARG_BOOL:
        if(curArgInt != 0 && curArgInt != 1)
        {
          commandResult.str(red("invalid boolean argument (must be 0 or 1)"));
          return false;
        }
        break;

      case kARG_BASE_SPCL:
        if(curArgInt != 2 && curArgInt != 10 && curArgInt != 16
           && curArgStr != "hex" && curArgStr != "dec" && curArgStr != "bin")
        {
          commandResult.str(red("invalid base (must be #2, #10, #16, \"bin\", \"dec\", or \"hex\")"));
          return false;
        }
        break;

      case kARG_LABEL:
      case kARG_FILE:
        break; // TODO: validate these (for now any string's allowed)

      case kARG_MULTI_BYTE:
      case kARG_MULTI_WORD:
        break; // FIXME: validate these (for now, any number's allowed)

      case kARG_END_ARGS:
        break;
    }
    curCount++;
    p++;

  } while(*p != kARG_END_ARGS && curCount < argRequiredCount);

/*
cerr << "curCount         = " << curCount << endl
     << "argRequiredCount = " << argRequiredCount << endl
     << "*p               = " << *p << endl << endl;
*/

  if(curCount < argRequiredCount)
  {
    commandResult.str(red("missing required argument(s)"));
    return false;
  }
  else if(argCount > curCount)
  {
    commandResult.str(red("too many arguments"));
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::eval()
{
  ostringstream buf;
  for(uInt32 i = 0; i < argCount; ++i)
  {
    string rlabel = debugger.cartDebug().getLabel(args[i], true);
    string wlabel = debugger.cartDebug().getLabel(args[i], false);
    bool validR = rlabel != "" && rlabel[0] != '$',
         validW = wlabel != "" && wlabel[0] != '$';
    if(validR && validW)
    {
      if(rlabel == wlabel)
        buf << rlabel << "(R/W): ";
      else
        buf << rlabel << "(R) / " << wlabel << "(W): ";
    }
    else if(validR)
      buf << rlabel << "(R): ";
    else if(validW)
      buf << wlabel << "(W): ";

    if(args[i] < 0x100)
      buf << "$" << Base::toString(args[i], Base::F_16_2)
          << " %" << Base::toString(args[i], Base::F_2_8);
    else
      buf << "$" << Base::toString(args[i], Base::F_16_4)
          << " %" << Base::toString(args[i], Base::F_2_16);

    buf << " #" << int(args[i]);
    if(i != argCount - 1)
      buf << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::trapStatus(int addr)
{
  string result;
  result += Base::toString(addr);
  result += ": ";
  bool r = debugger.readTrap(addr);
  bool w = debugger.writeTrap(addr);
  if(r && w)
    result += "read|write";
  else if(r)
    result += "read";
  else if(w)
    result += "write";
  else
    result += "none";

  // TODO - technically, we should determine if the label is read or write
  const string& l = debugger.cartDebug().getLabel(addr, true);
  if(l != "") {
    result += "  (";
    result += l;
    result += ")";
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DebuggerParser::saveScriptFile(string file)
{
  if( file.find_last_of('.') == string::npos )
    file += ".stella";

  ofstream out(file);

  FunctionDefMap funcs = debugger.getFunctionDefMap();
  for(const auto& i: funcs)
    out << "function " << i.first << " { " << i.second << " }" << endl;

  for(const auto& i: watches)
    out << "watch " << i << endl;

  for(uInt32 i = 0; i < 0x10000; ++i)
    if(debugger.breakPoint(i))
      out << "break #" << i << endl;

  for(uInt32 i = 0; i < 0x10000; ++i)
  {
    bool r = debugger.readTrap(i);
    bool w = debugger.writeTrap(i);

    if(r && w)
      out << "trap #" << i << endl;
    else if(r)
      out << "trapread #" << i << endl;
    else if(w)
      out << "trapwrite #" << i << endl;
  }

  StringList conds = debugger.cpuDebug().m6502().getCondBreakNames();
  for(const auto& cond: conds)
    out << "breakif {" << cond << "}" << endl;

  return out.good();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// executor methods for commands[] array. All are void, no args.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "a"
void DebuggerParser::executeA()
{
  debugger.cpuDebug().setA(uInt8(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "base"
void DebuggerParser::executeBase()
{
  if(args[0] == 2 || argStrings[0] == "bin")
    Base::setFormat(Base::F_2);
  else if(args[0] == 10 || argStrings[0] == "dec")
    Base::setFormat(Base::F_10);
  else if(args[0] == 16 || argStrings[0] == "hex")
    Base::setFormat(Base::F_16);

  commandResult << "default base set to ";
  switch(Base::format()) {
    case Base::F_2:
      commandResult << "#2/bin";
      break;

    case Base::F_10:
      commandResult << "#10/dec";
      break;

    case Base::F_16:
      commandResult << "#16/hex";
      break;

    default:
      commandResult << red("UNKNOWN");
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "break"
void DebuggerParser::executeBreak()
{
  uInt16 bp;
  if(argCount == 0)
    bp = debugger.cpuDebug().pc();
  else
    bp = args[0];
  debugger.toggleBreakPoint(bp);
  debugger.rom().invalidate();

  if(debugger.breakPoint(bp))
    commandResult << "Set";
  else
    commandResult << "Cleared";

  commandResult << " breakpoint at " << Base::toString(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breakif"
void DebuggerParser::executeBreakif()
{
  int res = YaccParser::parse(argStrings[0].c_str());
  if(res == 0)
  {
    uInt32 ret = debugger.cpuDebug().m6502().addCondBreak(
                 YaccParser::getResult(), argStrings[0] );
    commandResult << "Added breakif " << Base::toString(ret);
  }
  else
    commandResult << red("invalid expression");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "c"
void DebuggerParser::executeC()
{
  if(argCount == 0)
    debugger.cpuDebug().toggleC();
  else if(argCount == 1)
    debugger.cpuDebug().setC(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "cheat"
// (see Stella manual for different cheat types)
void DebuggerParser::executeCheat()
{
#ifdef CHEATCODE_SUPPORT
  if(argCount == 0)
  {
    commandResult << red("Missing cheat code");
    return;
  }

  for(uInt32 arg = 0; arg < argCount; ++arg)
  {
    const string& cheat = argStrings[arg];
    if(debugger.myOSystem.cheat().add("DBG", cheat))
      commandResult << "Cheat code " << cheat << " enabled" << endl;
    else
      commandResult << red("Invalid cheat code ") << cheat << endl;
  }
#else
  commandResult << red("Cheat support not enabled\n");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearbreaks"
void DebuggerParser::executeClearbreaks()
{
  debugger.clearAllBreakPoints();
  debugger.cpuDebug().m6502().clearCondBreaks();
  commandResult << "all breakpoints cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearconfig"
void DebuggerParser::executeClearconfig()
{
  if(argCount == 1)
    commandResult << debugger.cartDebug().clearConfig(args[0]);
  else
    commandResult << debugger.cartDebug().clearConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "cleartraps"
void DebuggerParser::executeCleartraps()
{
  debugger.clearAllTraps();
  commandResult << "all traps cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearwatches"
void DebuggerParser::executeClearwatches()
{
  watches.clear();
  commandResult << "all watches cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "cls"
void DebuggerParser::executeCls()
{
  debugger.prompt().clearScreen();
  commandResult << "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "code"
void DebuggerParser::executeCode()
{
  if(argCount != 2)
  {
    commandResult << red("Specify start and end of range only");
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("Start address must be <= end address");
    return;
  }

  bool result = debugger.cartDebug().addDirective(
                  CartDebug::CODE, args[0], args[1]);
  commandResult << (result ? "added" : "removed") << " CODE directive on range $"
                << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "colortest"
void DebuggerParser::executeColortest()
{
  commandResult << "test color: "
                << char((args[0]>>1) | 0x80)
                << inverse("        ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "d"
void DebuggerParser::executeD()
{
  if(argCount == 0)
    debugger.cpuDebug().toggleD();
  else if(argCount == 1)
    debugger.cpuDebug().setD(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "data"
void DebuggerParser::executeData()
{
  if(argCount != 2)
  {
    commandResult << red("Specify start and end of range only");
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("Start address must be <= end address");
    return;
  }

  bool result = debugger.cartDebug().addDirective(
                  CartDebug::DATA, args[0], args[1]);
  commandResult << (result ? "added" : "removed") << " DATA directive on range $"
                << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "debugcolors"
void DebuggerParser::executeDebugColors()
{
  commandResult << debugger.tiaDebug().debugColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "define"
void DebuggerParser::executeDefine()
{
  // TODO: check if label already defined?
  debugger.cartDebug().addLabel(argStrings[0], args[1]);
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delbreakif"
void DebuggerParser::executeDelbreakif()
{
  debugger.cpuDebug().m6502().delCondBreak(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delfunction"
void DebuggerParser::executeDelfunction()
{
  if(debugger.delFunction(argStrings[0]))
    commandResult << "removed function " << argStrings[0];
  else
    commandResult << "function " << argStrings[0] << " built-in or not found";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delwatch"
void DebuggerParser::executeDelwatch()
{
  int which = args[0] - 1;
  if(which >= 0 && which < int(watches.size()))
  {
    Vec::removeAt(watches, which);
    commandResult << "removed watch";
  }
  else
    commandResult << "no such watch";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "disasm"
void DebuggerParser::executeDisasm()
{
  int start, lines = 20;

  if(argCount == 0) {
    start = debugger.cpuDebug().pc();
  } else if(argCount == 1) {
    start = args[0];
  } else if(argCount == 2) {
    start = args[0];
    lines = args[1];
  } else {
    commandResult << "wrong number of arguments";
    return;
  }

  commandResult << debugger.cartDebug().disassemble(start, lines);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "dump"
void DebuggerParser::executeDump()
{
  for(int i = 0; i < 8; ++i)
  {
    int start = args[0] + i*16;
    commandResult << Base::toString(start) << ": ";
    for(int j = 0; j < 16; ++j)
    {
      commandResult << Base::toString(debugger.peek(start+j)) << " ";
      if(j == 7) commandResult << "- ";
    }
    if(i != 7) commandResult << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "exec"
void DebuggerParser::executeExec()
{
  FilesystemNode file(argStrings[0]);
  commandResult << exec(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "exitrom"
void DebuggerParser::executeExitRom()
{
  debugger.quit(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "frame"
void DebuggerParser::executeFrame()
{
  int count = 1;
  if(argCount != 0) count = args[0];
  debugger.nextFrame(count);
  commandResult << "advanced " << dec << count << " frame(s)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "function"
void DebuggerParser::executeFunction()
{
  if(args[0] >= 0)
  {
    commandResult << red("name already in use");
    return;
  }

  int res = YaccParser::parse(argStrings[1].c_str());
  if(res == 0)
  {
    debugger.addFunction(argStrings[0], argStrings[1], YaccParser::getResult());
    commandResult << "Added function " << argStrings[0] << " -> " << argStrings[1];
  }
  else
    commandResult << red("invalid expression");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "gfx"
void DebuggerParser::executeGfx()
{
  if(argCount != 2)
  {
    commandResult << red("Specify start and end of range only");
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("Start address must be <= end address");
    return;
  }

  bool result = debugger.cartDebug().addDirective(
                  CartDebug::GFX, args[0], args[1]);
  commandResult << (result ? "added" : "removed") << " GFX directive on range $"
                << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "help"
void DebuggerParser::executeHelp()
{
  if(argCount == 0)  // normal help, show all commands
  {
    // Find length of longest command
    uInt16 clen = 0;
    for(int i = 0; i < kNumCommands; ++i)
    {
      uInt16 len = commands[i].cmdString.length();
      if(len > clen)  clen = len;
    }

    commandResult << setfill(' ');
    for(int i = 0; i < kNumCommands; ++i)
      commandResult << setw(clen) << right << commands[i].cmdString
                    << " - " << commands[i].description << endl;

    commandResult << debugger.builtinHelp();
  }
  else  // get help for specific command
  {
    for(int i = 0; i < kNumCommands; ++i)
    {
      if(argStrings[0] == commands[i].cmdString)
      {
        commandResult << "  " << red(commands[i].description) << endl
                      << commands[i].extendedDesc;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "jump"
void DebuggerParser::executeJump()
{
  int line = -1;
  int address = args[0];

  // The specific address we want may not exist (it may be part of a data section)
  // If so, scroll backward a little until we find it
  while(((line = debugger.cartDebug().addressToLine(address)) == -1) &&
        (address >= 0))
    address--;

  if(line >= 0 && address >= 0)
  {
    debugger.rom().scrollTo(line);
    commandResult << "disassembly scrolled to address $" << Base::HEX4 << address;
  }
  else
    commandResult << "address $" << Base::HEX4 << args[0] << " doesn't exist";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listbreaks"
void DebuggerParser::executeListbreaks()
{
  ostringstream buf;
  int count = 0;

  for(uInt32 i = 0; i <= 0xffff; ++i)
  {
    if(debugger.breakPoints().isSet(i))
    {
      buf << debugger.cartDebug().getLabel(i, true, 4) << " ";
      if(! (++count % 8) ) buf << "\n";
    }
  }

  /*
  if(count)
    return ret;
  else
    return "no breakpoints set";
    */
  if(count)
    commandResult << "breaks:\n" << buf.str();

  StringList conds = debugger.cpuDebug().m6502().getCondBreakNames();
  if(conds.size() > 0)
  {
    commandResult << "\nbreakifs:\n";
    for(uInt32 i = 0; i < conds.size(); i++)
    {
      commandResult << i << ": " << conds[i];
      if(i != (conds.size() - 1)) commandResult << endl;
    }
  }

  if(commandResult.str() == "")
    commandResult << "no breakpoints set";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listconfig"
void DebuggerParser::executeListconfig()
{
  if(argCount == 1)
    commandResult << debugger.cartDebug().listConfig(args[0]);
  else
    commandResult << debugger.cartDebug().listConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listfunctions"
void DebuggerParser::executeListfunctions()
{
  const FunctionDefMap& functions = debugger.getFunctionDefMap();

  if(functions.size() > 0)
  {
    for(const auto& iter: functions)
      commandResult << iter.first << " -> " << iter.second << endl;
  }
  else
    commandResult << "no user-defined functions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listtraps"
void DebuggerParser::executeListtraps()
{
  int count = 0;

  for(uInt32 i = 0; i <= 0xffff; ++i)
  {
    if(debugger.readTrap(i) || debugger.writeTrap(i))
    {
      commandResult << trapStatus(i) << " + mirrors" << endl;
      count++;
      break;
    }
  }

  if(!count)
    commandResult << "no traps set";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "loadconfig"
void DebuggerParser::executeLoadconfig()
{
  commandResult << debugger.cartDebug().loadConfigFile();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "loadstate"
void DebuggerParser::executeLoadstate()
{
  if(args[0] >= 0 && args[0] <= 9)
    debugger.loadState(args[0]);
  else
    commandResult << red("invalid slot (must be 0-9)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "n"
void DebuggerParser::executeN()
{
  if(argCount == 0)
    debugger.cpuDebug().toggleN();
  else if(argCount == 1)
    debugger.cpuDebug().setN(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "palette"
void DebuggerParser::executePalette()
{
  commandResult << debugger.tiaDebug().palette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pc"
void DebuggerParser::executePc()
{
  debugger.cpuDebug().setPC(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pgfx"
void DebuggerParser::executePGfx()
{
  if(argCount != 2)
  {
    commandResult << red("Specify start and end of range only");
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("Start address must be <= end address");
    return;
  }

  bool result = debugger.cartDebug().addDirective(
                  CartDebug::PGFX, args[0], args[1]);
  commandResult << (result ? "added" : "removed") << " PGFX directive on range $"
                << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "print"
void DebuggerParser::executePrint()
{
  commandResult << eval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "ram"
void DebuggerParser::executeRam()
{
  if(argCount == 0)
    commandResult << debugger.cartDebug().toString();
  else
    commandResult << debugger.setRAM(args);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "reset"
void DebuggerParser::executeReset()
{
  debugger.reset();
  debugger.rom().invalidate();
  commandResult << "reset system";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "rewind"
void DebuggerParser::executeRewind()
{
  if(debugger.rewindState())
  {
    debugger.rom().invalidate();
    commandResult << "rewind by one level";
  }
  else
    commandResult << "no states left to rewind";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "riot"
void DebuggerParser::executeRiot()
{
  commandResult << debugger.riotDebug().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "rom"
void DebuggerParser::executeRom()
{
  uInt16 addr = args[0];
  for(uInt32 i = 1; i < argCount; ++i)
  {
    if(!(debugger.patchROM(addr++, args[i])))
    {
      commandResult << red("patching ROM unsupported for this cart type");
      return;
    }
  }

  // Normally the run() method calls loadConfig() on the debugger,
  // which results in all child widgets being redrawn.
  // The RomWidget is a special case, since we don't want to re-disassemble
  // any more than necessary.  So we only do it by calling the following
  // method ...
  debugger.rom().invalidate();

  commandResult << "changed " << (args.size() - 1) << " location(s)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "row"
void DebuggerParser::executeRow()
{
  if(argCount != 2)
  {
    commandResult << red("Specify start and end of range only");
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("Start address must be <= end address");
    return;
  }

  bool result = debugger.cartDebug().addDirective(
                  CartDebug::ROW, args[0], args[1]);
  commandResult << (result ? "added" : "removed") << " ROW directive on range $"
                << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "run"
void DebuggerParser::executeRun()
{
  debugger.saveOldState();
  debugger.quit(false);
  commandResult << "_EXIT_DEBUGGER";  // See PromptWidget for more info
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "runto"
void DebuggerParser::executeRunTo()
{
  const CartDebug& cartdbg = debugger.cartDebug();
  const CartDebug::DisassemblyList& list = cartdbg.disassembly().list;

  uInt32 count = 0, max_iterations = uInt32(list.size());

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ostringstream buf;
  buf << "RunTo searching through " << max_iterations << " disassembled instructions";
  ProgressDialog progress(debugger.myBaseDialog, debugger.lfont(), buf.str());
  progress.setRange(0, max_iterations, 5);

  bool done = false;
  do {
    debugger.step();

    // Update romlist to point to current PC
    int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    if(pcline >= 0)
    {
      const string& next = list[pcline].disasm;
      done = (BSPF::findIgnoreCase(next, argStrings[0]) != string::npos);
    }
    // Update the progress bar
    progress.setProgress(count);
  } while(!done && ++count < max_iterations);

  progress.close();

  if(done)
    commandResult
      << "found " << argStrings[0] << " in " << dec << count
      << " disassembled instructions";
  else
    commandResult
      << argStrings[0] << " not found in " << dec << count
      << " disassembled instructions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "runtopc"
void DebuggerParser::executeRunToPc()
{
  const CartDebug& cartdbg = debugger.cartDebug();
  const CartDebug::DisassemblyList& list = cartdbg.disassembly().list;

  uInt32 count = 0;
  bool done = false;
  do {
    debugger.step();

    // Update romlist to point to current PC
    int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    done = (pcline >= 0) && (list[pcline].address == args[0]);
  } while(!done && ++count < list.size());

  if(done)
    commandResult
      << "set PC to " << Base::HEX4 << args[0] << " in "
      << dec << count << " disassembled instructions";
  else
    commandResult
      << "PC " << Base::HEX4 << args[0] << " not reached or found in "
      << dec << count << " disassembled instructions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "s"
void DebuggerParser::executeS()
{
  debugger.cpuDebug().setSP(uInt8(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "save"
void DebuggerParser::executeSave()
{
  if(saveScriptFile(argStrings[0]))
    commandResult << "saved script to file " << argStrings[0];
  else
    commandResult << red("I/O error");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveconfig"
void DebuggerParser::executeSaveconfig()
{
  commandResult << debugger.cartDebug().saveConfigFile();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "savedis"
void DebuggerParser::executeSavedisassembly()
{
  commandResult << debugger.cartDebug().saveDisassembly();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saverom"
void DebuggerParser::executeSaverom()
{
  commandResult << debugger.cartDebug().saveRom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveses"
void DebuggerParser::executeSaveses()
{
  if(debugger.prompt().saveBuffer(argStrings[0]))
    commandResult << "saved session to file " << argStrings[0];
  else
    commandResult << red("I/O error");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "savesnap"
void DebuggerParser::executeSavesnap()
{
  debugger.tiaOutput().saveSnapshot();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "savestate"
void DebuggerParser::executeSavestate()
{
  if(args[0] >= 0 && args[0] <= 9)
    debugger.saveState(args[0]);
  else
    commandResult << red("invalid slot (must be 0-9)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "scanline"
void DebuggerParser::executeScanline()
{
  int count = 1;
  if(argCount != 0) count = args[0];
  debugger.nextScanline(count);
  commandResult << "advanced " << dec << count << " scanline(s)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "step"
void DebuggerParser::executeStep()
{
  commandResult
    << "executed " << dec << debugger.step() << " cycles";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "tia"
void DebuggerParser::executeTia()
{
  commandResult << debugger.tiaDebug().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trace"
void DebuggerParser::executeTrace()
{
  commandResult << "executed " << dec << debugger.trace() << " cycles";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trap"
void DebuggerParser::executeTrap()
{
  if(argCount > 2)
  {
    commandResult << red("Command takes one or two arguments") << endl;
    return;
  }

  uInt32 beg = args[0];
  uInt32 end = argCount == 2 ? args[1] : beg;
  if(beg > 0xFFFF || end > 0xFFFF)
  {
    commandResult << red("One or more addresses are invalid") << endl;
    return;
  }

  for(uInt32 addr = beg; addr <= end; ++addr)
    executeTrapRW(addr, true, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapread"
void DebuggerParser::executeTrapread()
{
  if(argCount > 2)
  {
    commandResult << red("Command takes one or two arguments") << endl;
    return;
  }

  uInt32 beg = args[0];
  uInt32 end = argCount == 2 ? args[1] : beg;
  if(beg > 0xFFFF || end > 0xFFFF)
  {
    commandResult << red("One or more addresses are invalid") << endl;
    return;
  }

  for(uInt32 addr = beg; addr <= end; ++addr)
    executeTrapRW(addr, true, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapwrite"
void DebuggerParser::executeTrapwrite()
{
  if(argCount > 2)
  {
    commandResult << red("Command takes one or two arguments") << endl;
    return;
  }

  uInt32 beg = args[0];
  uInt32 end = argCount == 2 ? args[1] : beg;
  if(beg > 0xFFFF || end > 0xFFFF)
  {
    commandResult << red("One or more addresses are invalid") << endl;
    return;
  }

  for(uInt32 addr = beg; addr <= end; ++addr)
    executeTrapRW(addr, false, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// wrapper function for trap/trapread/trapwrite commands
void DebuggerParser::executeTrapRW(uInt32 addr, bool read, bool write)
{
  switch(debugger.cartDebug().addressType(addr))
  {
    case CartDebug::ADDR_TIA:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1080) == 0x0000)
        {
          if(read && (i & 0x000F) == addr)
            debugger.toggleReadTrap(i);
          if(write && (i & 0x003F) == addr)
            debugger.toggleWriteTrap(i);
        }
      }
      break;
    }
    case CartDebug::ADDR_IO:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1080) == 0x0080 && (i & 0x0200) != 0x0000 && (i & 0x02FF) == addr)
        {
          if(read)  debugger.toggleReadTrap(i);
          if(write) debugger.toggleWriteTrap(i);
        }
      }
      break;
    }
    case CartDebug::ADDR_ZPRAM:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1080) == 0x0080 && (i & 0x0200) == 0x0000 && (i & 0x00FF) == addr)
        {
          if(read)  debugger.toggleReadTrap(i);
          if(write) debugger.toggleWriteTrap(i);
        }
      }
      break;
    }
    case CartDebug::ADDR_ROM:
    {
      if(addr >= 0x1000 && addr <= 0xFFFF)
      {
        for(uInt32 i = 0x1000; i <= 0xFFFF; ++i)
        {
          if((i % 0x2000 >= 0x1000) && (i & 0x0FFF) == (addr & 0x0FFF))
          {
            if(read)  debugger.toggleReadTrap(i);
            if(write) debugger.toggleWriteTrap(i);
          }
        }
      }
      break;
    }
  }

  commandResult << trapStatus(addr) << " + mirrors" << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "type"
void DebuggerParser::executeType()
{
  uInt32 beg = args[0];
  uInt32 end = argCount >= 2 ? args[1] : beg;
  if(beg > end)  std::swap(beg, end);

  for(uInt32 i = beg; i <= end; ++i)
  {
    commandResult << Base::HEX4 << i << ": ";
    debugger.cartDebug().addressTypeAsString(commandResult, i);
    commandResult << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "uhex"
void DebuggerParser::executeUHex()
{
  bool enable = !Base::hexUppercase();
  Base::setHexUppercase(enable);

  settings.setValue("dbg.uhex", enable);
  debugger.rom().invalidate();

  commandResult << "uppercase HEX " << (enable ? "enabled" : "disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "undef"
void DebuggerParser::executeUndef()
{
  if(debugger.cartDebug().removeLabel(argStrings[0]))
  {
    debugger.rom().invalidate();
    commandResult << argStrings[0] + " now undefined";
  }
  else
    commandResult << red("no such label");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "v"
void DebuggerParser::executeV()
{
  if(argCount == 0)
    debugger.cpuDebug().toggleV();
  else if(argCount == 1)
    debugger.cpuDebug().setV(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "watch"
void DebuggerParser::executeWatch()
{
  watches.push_back(argStrings[0]);
  commandResult << "added watch \"" << argStrings[0] << "\"";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "x"
void DebuggerParser::executeX()
{
  debugger.cpuDebug().setX(uInt8(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "y"
void DebuggerParser::executeY()
{
  debugger.cpuDebug().setY(uInt8(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "z"
void DebuggerParser::executeZ()
{
  if(argCount == 0)
    debugger.cpuDebug().toggleZ();
  else if(argCount == 1)
    debugger.cpuDebug().setZ(args[0]);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// List of all commands available to the parser
DebuggerParser::Command DebuggerParser::commands[kNumCommands] = {
  {
    "a",
    "Set Accumulator to <value>",
    "Valid value is 0 - ff\nExample: a ff, a #10",
    true,
    true,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeA)
  },

  {
    "base",
    "Set default base to <base>",
    "Base is hex, dec, or bin\nExample: base hex",
    true,
    true,
    { kARG_BASE_SPCL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBase)
  },

  {
    "break",
    "Set/clear breakpoint at <address>",
    "Command is a toggle, default is current PC\nValid address is 0 - ffff\n"
    "Example: break, break f000",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBreak)
  },

  {
    "breakif",
    "Set breakpoint on <condition>",
    "Condition can include multiple items, see documentation\nExample: breakif _scan>100",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBreakif)
  },

  {
    "c",
    "Carry Flag: set (0 or 1), or toggle (no arg)",
    "Example: c, c 0, c 1",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeC)
  },

  {
    "cheat",
    "Use a cheat code (see manual for cheat types)",
    "Example: cheat 0040, cheat abff00",
    false,
    false,
    { kARG_LABEL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCheat)
  },

  {
    "clearbreaks",
    "Clear all breakpoints",
    "Example: clearbreaks (no parameters)",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearbreaks)
  },

  {
    "clearconfig",
    "Clear Distella config directives [bank xx]",
    "Example: clearconfig 0, clearconfig 1",
    false,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeClearconfig)
  },

  {
    "cleartraps",
    "Clear all traps",
    "All traps cleared, including any mirrored ones\nExample: cleartraps (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCleartraps)
  },

  {
    "clearwatches",
    "Clear all watches",
    "Example: clearwatches (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearwatches)
  },

  {
    "cls",
    "Clear prompt area of text",
    "Completely clears screen, but keeps history of commands",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCls)
  },

  {
    "code",
    "Mark 'CODE' range in disassembly",
    "Start and end of range required\nExample: code f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeCode)
  },

  {
    "colortest",
    "Show value xx as TIA color",
    "Shows a color swatch for the given value\nExample: colortest 1f",
    true,
    false,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeColortest)
  },

  {
    "d",
    "Carry Flag: set (0 or 1), or toggle (no arg)",
    "Example: d, d 0, d 1",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeD)
  },

  {
    "data",
    "Mark 'DATA' range in disassembly",
    "Start and end of range required\nExample: data f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeData)
  },

  {
    "debugcolors",
    "Show Fixed Debug Colors information",
    "Example: debugcolors (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDebugColors)
  },

  {
    "define",
    "Define label xx for address yy",
    "Example: define LABEL1 f100",
    true,
    true,
    { kARG_LABEL, kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDefine)
  },

  {
    "delbreakif",
    "Delete conditional breakif <xx>",
    "Example: delbreakif 0",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelbreakif)
  },

  {
    "delfunction",
    "Delete function with label xx",
    "Example: delfunction FUNC1",
    true,
    false,
    { kARG_LABEL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelfunction)
  },

  {
    "delwatch",
    "Delete watch <xx>",
    "Example: delwatch 0",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelwatch)
  },

  {
    "disasm",
    "Disassemble address xx [yy lines] (default=PC)",
    "Disassembles from starting address <xx> (default=PC) for <yy> lines\n"
    "Example: disasm, disasm f000 100",
    false,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeDisasm)
  },

  {
    "dump",
    "Dump 128 bytes of memory at address <xx>",
    "Example: dump f000",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDump)
  },

  {
    "exec",
    "Execute script file <xx>",
    "Example: exec script.dat, exec auto.txt",
    true,
    true,
    { kARG_FILE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeExec)
  },

  {
    "exitrom",
    "Exit emulator, return to ROM launcher",
    "Self-explanatory",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeExitRom)
  },

  {
    "frame",
    "Advance emulation by <xx> frames (default=1)",
    "Example: frame, frame 100",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeFrame)
  },

  {
    "function",
    "Define function name xx for expression yy",
    "Example: define FUNC1 { ... }",
    true,
    false,
    { kARG_LABEL, kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeFunction)
  },

  {
    "gfx",
    "Mark 'GFX' range in disassembly",
    "Start and end of range required\nExample: gfx f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeGfx)
  },

  {
    "help",
    "help <command>",
    "Show all commands, or give function for help on that command\n"
    "Example: help, help code",
    false,
    false,
    { kARG_LABEL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeHelp)
  },

  {
    "jump",
    "Scroll disassembly to address xx",
    "Moves disassembly listing to address <xx>\nExample: jump f400",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJump)
  },

  {
    "listbreaks",
    "List breakpoints",
    "Example: listbreaks (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListbreaks)
  },

  {
    "listconfig",
    "List Distella config directives [bank xx]",
    "Example: listconfig 0, listconfig 1",
    false,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeListconfig)
  },

  {
    "listfunctions",
    "List user-defined functions",
    "Example: listfunctions (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListfunctions)
  },

  {
    "listtraps",
    "List traps",
    "Lists all traps (read and/or write)\nExample: listtraps (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListtraps)
  },

  {
    "loadconfig",
    "Load Distella config file",
    "Example: loadconfig file.cfg",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeLoadconfig)
  },

  {
    "loadstate",
    "Load emulator state xx (0-9)",
    "Example: loadstate 0, loadstate 9",
    true,
    true,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeLoadstate)
  },

  {
    "n",
    "Negative Flag: set (0 or 1), or toggle (no arg)",
    "Example: n, n 0, n 1",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeN)
  },

  {
    "palette",
    "Show current TIA palette",
    "Example: palette (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePalette)
  },

  {
    "pc",
    "Set Program Counter to address xx",
    "Example: pc f000",
    true,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePc)
  },

  {
    "pgfx",
    "Mark 'PGFX' range in disassembly",
    "Start and end of range required\nExample: pgfx f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executePGfx)
  },

  {
    "print",
    "Evaluate/print expression xx in hex/dec/binary",
    "Almost anything can be printed (constants, expressions, registers)\n"
    "Example: print pc, print f000",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePrint)
  },

  {
    "ram",
    "Show ZP RAM, or set address xx to yy1 [yy2 ...]",
    "Example: ram, ram 80 00 ...",
    false,
    true,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRam)
  },

  {
    "reset",
    "Reset system to power-on state",
    "System is completely reset, just as if it was just powered on",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeReset)
  },

  {
    "rewind",
    "Rewind state to last step/trace/scanline/frame",
    "Rewind currently only works in the debugger",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRewind)
  },

  {
    "riot",
    "Show RIOT timer/input status",
    "Display text-based output of the contents of the RIOT tab",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRiot)
  },

  {
    "rom",
    "Set ROM address xx to yy1 [yy2 ...]",
    "What happens here depends on the current bankswitching scheme\n"
    "Example: rom f000 00 01 ff ...",
    true,
    true,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRom)
  },

  {
    "row",
    "Mark 'ROW' range in disassembly",
    "Start and end of range required\nExample: row f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRow)
  },

  {
    "run",
    "Exit debugger, return to emulator",
    "Self-explanatory",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRun)
  },

  {
    "runto",
    "Run until string xx in disassembly",
    "Advance until the given string is detected in the disassembly\n"
    "Example: runto lda",
    true,
    true,
    { kARG_LABEL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRunTo)
  },

  {
    "runtopc",
    "Run until PC is set to value xx",
    "Example: runtopc f200",
    true,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRunToPc)
  },

  {
    "s",
    "Set Stack Pointer to value xx",
    "Accepts 8-bit value, Example: s f0",
    true,
    true,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeS)
  },

  {
    "save",
    "Save breaks, watches, traps to file xx",
    "Example: save commands.txt",
    true,
    false,
    { kARG_FILE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSave)
  },

  {
    "saveconfig",
    "Save Distella config file",
    "Example: saveconfig file.cfg",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveconfig)
  },

  {
    "savedis",
    "Save Distella disassembly",
    "Example: savedis file.asm",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavedisassembly)
  },

  {
    "saverom",
    "Save (possibly patched) ROM",
    "Example: savedrom file.bin",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaverom)
  },

  {
    "saveses",
    "Save console session to file xx",
    "Example: saveses session.txt",
    true,
    false,
    { kARG_FILE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveses)
  },

  {
    "savesnap",
    "Save current TIA image to PNG file",
    "Save snapshot to current snapshot save directory\n"
    "Example: savesnap (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavesnap)
  },

  {
    "savestate",
    "Save emulator state xx (valid args 0-9)",
    "Example: savestate 0, savestate 9",
    true,
    false,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavestate)
  },

  {
    "scanline",
    "Advance emulation by <xx> scanlines (default=1)",
    "Example: scanline, scanline 100",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeScanline)
  },

  {
    "step",
    "Single step CPU [with count xx]",
    "Example: step, step 100",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeStep)
  },

  {
    "tia",
    "Show TIA state",
    "Display text-based output of the contents of the TIA tab",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeTia)
  },

  {
    "trace",
    "Single step CPU over subroutines [with count xx]",
    "Example: trace, trace 100",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeTrace)
  },

  {
    "trap",
    "Trap read/write access to address(es) xx [yy]",
    "Set a R/W trap on the given address(es) and all mirrors\n"
    "Example: trap f000, trap f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrap)
  },

  {
    "trapread",
    "Trap read access to address(es) xx [yy]",
    "Set a read trap on the given address(es) and all mirrors\n"
    "Example: trapread f000, trapread f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapread)
  },

  {
    "trapwrite",
    "Trap write access to address(es) xx [yy]",
    "Set a write trap on the given address(es) and all mirrors\n"
    "Example: trapwrite f000, trapwrite f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapwrite)
  },

  {
    "type",
    "Show disassembly type for address xx [yy]",
    "Example: type f000, type f000 f010",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeType)
  },

  {
    "uhex",
    "Toggle upper/lowercase HEX display",
    "Note: not all hex output can be changed\n"
    "Example: uhex (no parameters)",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUHex)
  },

  {
    "undef",
    "Undefine label xx (if defined)",
    "Example: undef LABEL1",
    true,
    true,
    { kARG_LABEL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUndef)
  },

  {
    "v",
    "Overflow Flag: set (0 or 1), or toggle (no arg)",
    "Example: v, v 0, v 1",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeV)
  },

  {
    "watch",
    "Print contents of address xx before every prompt",
    "Example: watch ram_80",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeWatch)
  },

  {
    "x",
    "Set X Register to value xx",
    "Valid value is 0 - ff\nExample: x ff, x #10",
    true,
    true,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeX)
  },

  {
    "y",
    "Set Y Register to value xx",
    "Valid value is 0 - ff\nExample: y ff, y #10",
    true,
    true,
    { kARG_BYTE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeY)
  },

  {
    "z",
    "Zero Flag: set (0 or 1), or toggle (no arg)",
    "Example: z, z 0, z 1",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeZ)
  }
};
