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
#include "Settings.hxx"
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
    myCommand(0),
    argCount(0),
    execDepth(0),
    execPrefix("")
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
      {
        myCommand = i;
        commands[i].executor(this);
      }

      if(commands[i].refreshRequired)
        debugger.myBaseDialog->loadConfig();

      return commandResult.str();
    }
  }

  return red("No such command (try \"help\")");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::exec(const FilesystemNode& file, StringList* history)
{
  if(file.exists())
  {
    ifstream in(file.getPath());
    if(!in.is_open())
      return red("script file \'" + file.getShortPath() + "\' not found");

    ostringstream buf;
    int count = 0;
    string command;
    while( !in.eof() )
    {
      if(!getline(in, command))
        break;

      run(command);
      if (history != nullptr)
        history->push_back(command);
      count++;
    }
    buf << "\nExecuted " << count << " commands from \""
        << file.getShortPath() << "\"";

    return buf.str();
  }
  else
    return red("script file \'" + file.getShortPath() + "\' not found");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::outputCommandError(const string& errorMsg, int command)
{
  string example = commands[command].extendedDesc.substr(commands[command].extendedDesc.find("Example:"));

  commandResult << red(errorMsg);
  if(!example.empty())
    commandResult << endl << example;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Completion-related stuff:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::getCompletions(const char* in, StringList& completions) const
{
  // cerr << "Attempting to complete \"" << in << "\"" << endl;
  for(int i = 0; i < kNumCommands; ++i)
  {
    if(BSPF::matches(commands[i].cmdString, in))
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
  if(arg == "a" && str != "$a") result = state.A;
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
  for(uInt32 i = 0; i < myWatches.size(); ++i)
  {
    if(myWatches[i] != "")
    {
      // Clear the args, since we're going to pass them to eval()
      argStrings.clear();
      args.clear();

      argCount = 1;
      argStrings.push_back(myWatches[i]);
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
      commandResult.str();
      outputCommandError("missing required argument(s)", cmd);
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
      case kARG_DWORD:
      #if 0   // TODO - do we need error checking at all here?
        if(curArgInt > 0xffffffff)
        {
          commandResult.str(red("invalid word argument (must be 0-$ffffffff)"));
          return false;
        }
      #endif
        break;

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
    commandResult.str();
    outputCommandError("missing required argument(s)", cmd);
    return false;
  }
  else if(argCount > curCount)
  {
    commandResult.str();
    outputCommandError("too many arguments", cmd);
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
    if(args[i] < 0x10000)
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
    }

    buf << "$" << Base::toString(args[i], Base::F_16);

    if(args[i] < 0x10000)
      buf << " %" << Base::toString(args[i], Base::F_2);

    buf << " #" << int(args[i]);
    if(i != argCount - 1)
      buf << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::listTraps(bool listCond)
{
  StringList names = debugger.m6502().getCondTrapNames();

  commandResult << (listCond ? "trapifs:" : "traps:") << endl;
  for(uInt32 i = 0; i < names.size(); i++)
  {
    bool hasCond = names[i] != "";
    if(hasCond == listCond)
    {
      commandResult << Base::toString(i) << ": ";
      if(myTraps[i]->read && myTraps[i]->write)
        commandResult << "read|write";
      else if(myTraps[i]->read)
        commandResult << "read      ";
      else if(myTraps[i]->write)
        commandResult << "     write";
      else
        commandResult << "none";

      if(hasCond)
        commandResult << " " << names[i];
      commandResult << " " << debugger.cartDebug().getLabel(myTraps[i]->begin, true, 4);
      if(myTraps[i]->begin != myTraps[i]->end)
        commandResult << " " << debugger.cartDebug().getLabel(myTraps[i]->end, true, 4);
      commandResult << trapStatus(*myTraps[i]);
      commandResult << " + mirrors";
      if(i != (names.size() - 1)) commandResult << endl;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::trapStatus(const Trap& trap)
{
  stringstream result;
  string lblb = debugger.cartDebug().getLabel(trap.begin, !trap.write);
  string lble = debugger.cartDebug().getLabel(trap.end, !trap.write);

  if(lblb != "") {
    result << " (";
    result << lblb;
  }

  if(trap.begin != trap.end)
  {
    if(lble != "")
    {
      if (lblb != "")
        result << " ";
      else
        result << " (";
      result << lble;
    }
  }
  if (lblb != "" || lble != "")
    result << ")";

  return result.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::saveScriptFile(string file)
{
  // Append 'script' extension when necessary
  if(file.find_last_of('.') == string::npos)
    file += ".script";

  FilesystemNode node(debugger.myOSystem.defaultSaveDir() + file);
  ofstream out(node.getPath());
  if(!out.is_open())
    return "Unable to save script to " + node.getShortPath();

  FunctionDefMap funcs = debugger.getFunctionDefMap();
  for(const auto& f: funcs)
    if (!debugger.isBuiltinFunction(f.first))
      out << "function " << f.first << " {" << f.second << "}" << endl;

  for(const auto& w: myWatches)
    out << "watch " << w << endl;

  for(uInt32 i = 0; i < 0x10000; ++i)
    if(debugger.breakPoint(i))
      out << "break " << Base::toString(i) << endl;

  StringList conds = debugger.m6502().getCondBreakNames();
  for(const auto& cond : conds)
    out << "breakif {" << cond << "}" << endl;

  conds = debugger.m6502().getCondSaveStateNames();
  for(const auto& cond : conds)
    out << "savestateif {" << cond << "}" << endl;

  StringList names = debugger.m6502().getCondTrapNames();
  for(uInt32 i = 0; i < myTraps.size(); ++i)
  {
    bool read = myTraps[i]->read;
    bool write = myTraps[i]->write;
    bool hasCond = names[i] != "";

    if(read && write)
      out << "trap";
    else if(read)
      out << "trapread";
    else if(write)
      out << "trapwrite";
    if(hasCond)
      out << "if {" << names[i] << "}";
    out << " " << Base::toString(myTraps[i]->begin);
    if(myTraps[i]->begin != myTraps[i]->end)
      out << " " << Base::toString(myTraps[i]->end);
    out << endl;
  }

  return "saved " + node.getShortPath() + " OK";
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

  commandResult << "default number base set to ";
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
    commandResult << "set";
  else
    commandResult << "cleared";

  commandResult << " breakpoint at " << Base::toString(bp);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breakif"
void DebuggerParser::executeBreakif()
{
  int res = YaccParser::parse(argStrings[0].c_str());
  if(res == 0)
  {
    string condition = argStrings[0];
    for(uInt32 i = 0; i < debugger.m6502().getCondBreakNames().size(); i++)
    {
      if(condition == debugger.m6502().getCondBreakNames()[i])
      {
        args[0] = i;
        executeDelbreakif();
        return;
      }
    }
    uInt32 ret = debugger.m6502().addCondBreak(
                 YaccParser::getResult(), argStrings[0] );
    commandResult << "added breakif " << Base::toString(ret);
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
    outputCommandError("missing cheat code", myCommand);
    return;
  }

  for(uInt32 arg = 0; arg < argCount; ++arg)
  {
    const string& cheat = argStrings[arg];
    if(debugger.myOSystem.cheat().add("DBG", cheat))
      commandResult << "cheat code " << cheat << " enabled" << endl;
    else
      commandResult << red("invalid cheat code ") << cheat << endl;
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
  debugger.m6502().clearCondBreaks();
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
// "clearbreaks"
void DebuggerParser::executeClearsavestateifs()
{
  debugger.m6502().clearCondSaveStates();
  commandResult << "all savestate points cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "cleartraps"
void DebuggerParser::executeCleartraps()
{
  debugger.clearAllTraps();
  debugger.m6502().clearCondTraps();
  myTraps.clear();
  commandResult << "all traps cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearwatches"
void DebuggerParser::executeClearwatches()
{
  myWatches.clear();
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
    outputCommandError("specify start and end of range only", myCommand);
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
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
    outputCommandError("specify start and end of range only", myCommand);
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
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
  if (debugger.m6502().delCondBreak(args[0]))
    commandResult << "removed breakif " << Base::toString(args[0]);
  else
    commandResult << red("no such breakif");
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
// "delsavestateif"
void DebuggerParser::executeDelsavestateif()
{
  if(debugger.m6502().delCondSaveState(args[0]))
    commandResult << "removed savestateif " << Base::toString(args[0]);
  else
    commandResult << red("no such savestateif");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "deltrap"
void DebuggerParser::executeDeltrap()
{
  int index = args[0];

  if(debugger.m6502().delCondTrap(index))
  {
    for(uInt32 addr = myTraps[index]->begin; addr <= myTraps[index]->end; ++addr)
      executeTrapRW(addr, myTraps[index]->read, myTraps[index]->write, false);
    // @sa666666: please check this:
    Vec::removeAt(myTraps, index);
    commandResult << "removed trap " << Base::toString(index);
  }
  else
    commandResult << red("no such trap");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delwatch"
void DebuggerParser::executeDelwatch()
{
  int which = args[0] - 1;
  if(which >= 0 && which < int(myWatches.size()))
  {
    Vec::removeAt(myWatches, which);
    commandResult << "removed watch";
  }
  else
    commandResult << red("no such watch");
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
    outputCommandError("wrong number of arguments", myCommand);
    return;
  }

  commandResult << debugger.cartDebug().disassemble(start, lines);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "dump"
void DebuggerParser::executeDump()
{
  auto dump = [&](ostream& os, int start, int end)
  {
    for(int i = start; i <= end; i += 16)
    {
      // Print label every 16 bytes
      os << Base::toString(i) << ": ";

      for(int j = i; j < i+16 && j <= end; ++j)
      {
        os << Base::toString(debugger.peek(j)) << " ";
        if(j == i+7 && j != end) os << "- ";
      }
      os << endl;
    }
  };

  // Error checking
  if( argCount == 0 || argCount > 3)
  {
    outputCommandError("wrong number of arguments", myCommand);
    return;
  }
  if(argCount > 1 && args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
    return;
  }

  if(argCount == 1)
    dump(commandResult, args[0], args[0] + 127);
  else if(argCount == 2 || args[2] == 0)
    dump(commandResult, args[0], args[1]);
  else
  {
    ostringstream file;
    file << debugger.myOSystem.defaultSaveDir() << debugger.myOSystem.console().properties().get(Cartridge_Name) << "_dbg_";
    if(execDepth > 0)
    {
      file << execPrefix;
    }
    else
    {
      file << std::hex << std::setw(8) << std::setfill('0') << uInt32(debugger.myOSystem.getTicks() / 1000);
    }
    file << ".dump";
    FilesystemNode node(file.str());
    // cout << "dump " << args[0] << "-" << args[1] << " to " << file.str() << endl;
    ofstream ofs(node.getPath(), ofstream::out | ofstream::app);
    if(!ofs.is_open())
    {
      outputCommandError("Unable to append dump to file " + node.getShortPath(), myCommand);
      return;
    }
    if((args[2] & 0x07) != 0)
      commandResult << "dumped ";
    if((args[2] & 0x01) != 0)
    {
      // dump memory
      dump(ofs, args[0], args[1]);
      commandResult << "bytes from $" << hex << args[0] << " to $" << hex << args[1];
      if((args[2] & 0x06) != 0)
        commandResult << ", ";
    }
    if((args[2] & 0x02) != 0)
    {
      // dump CPU state
      CpuDebug& cpu = debugger.cpuDebug();
      ofs << "   <PC>PC SP  A  X  Y  -  -    N  V  B  D  I  Z  C  -\n";
      ofs << "XC: "
        << Base::toString(cpu.pc() & 0xff) << " "    // PC lsb
        << Base::toString(cpu.pc() >> 8) << " "    // PC msb
        << Base::toString(cpu.sp()) << " "    // SP
        << Base::toString(cpu.a()) << " "    // A
        << Base::toString(cpu.x()) << " "    // X
        << Base::toString(cpu.y()) << " "    // Y
        << Base::toString(0) << " "    // unused
        << Base::toString(0) << " - "  // unused
        << Base::toString(cpu.n()) << " "    // N (flag)
        << Base::toString(cpu.v()) << " "    // V (flag)
        << Base::toString(cpu.b()) << " "    // B (flag)
        << Base::toString(cpu.d()) << " "    // D (flag)
        << Base::toString(cpu.i()) << " "    // I (flag)
        << Base::toString(cpu.z()) << " "    // Z (flag)
        << Base::toString(cpu.c()) << " "    // C (flag)
        << Base::toString(0) << " "    // unused
        << endl;
      commandResult << "CPU state";
      if((args[2] & 0x04) != 0)
        commandResult << ", ";
    }
    if((args[2] & 0x04) != 0)
    {
      // dump SWCHx/INPTx state
      ofs << "   SWA - SWB  - IT  -  -  -   I0 I1 I2 I3 I4 I5 -  -\n";
      ofs << "XS: "
        << Base::toString(debugger.peek(0x280)) << " "    // SWCHA
        << Base::toString(0) << " "    // unused
        << Base::toString(debugger.peek(0x282)) << " "    // SWCHB
        << Base::toString(0) << " "    // unused
        << Base::toString(debugger.peek(0x284)) << " "    // INTIM
        << Base::toString(0) << " "    // unused
        << Base::toString(0) << " "    // unused
        << Base::toString(0) << " - "  // unused
        << Base::toString(debugger.peek(TIARegister::INPT0)) << " "
        << Base::toString(debugger.peek(TIARegister::INPT1)) << " "
        << Base::toString(debugger.peek(TIARegister::INPT2)) << " "
        << Base::toString(debugger.peek(TIARegister::INPT3)) << " "
        << Base::toString(debugger.peek(TIARegister::INPT4)) << " "
        << Base::toString(debugger.peek(TIARegister::INPT5)) << " "
        << Base::toString(0) << " "    // unused
        << Base::toString(0) << " "    // unused
        << endl;
      commandResult << "switches and fire buttons";
    }
    if((args[2] & 0x07) != 0)
      commandResult << " to file " << node.getShortPath();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "exec"
void DebuggerParser::executeExec()
{
  // Append 'script' extension when necessary
  string file = argStrings[0];
  if(file.find_last_of('.') == string::npos)
    file += ".script";
  FilesystemNode node(file);
  if (!node.exists()) {
    node = FilesystemNode(debugger.myOSystem.defaultSaveDir() + file);
  }

  if (argCount == 2) {
    execPrefix = argStrings[1];
  }
  else {
    ostringstream prefix;
    prefix << std::hex << std::setw(8) << std::setfill('0') << uInt32(debugger.myOSystem.getTicks()/1000);
    execPrefix = prefix.str();
  }
  execDepth++;
  commandResult << exec(node);
  execDepth--;
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
    commandResult << "added function " << argStrings[0] << " -> " << argStrings[1];
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
    outputCommandError("specify start and end of range only", myCommand);
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
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
// "joy0up"
void DebuggerParser::executeJoy0Up()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Left);
  if(argCount == 0)
    controller.set(Controller::One, !controller.read(Controller::One));
  else if(argCount == 1)
    controller.set(Controller::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0down"
void DebuggerParser::executeJoy0Down()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Left);
  if(argCount == 0)
    controller.set(Controller::Two, !controller.read(Controller::Two));
  else if(argCount == 1)
    controller.set(Controller::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0left"
void DebuggerParser::executeJoy0Left()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Left);
  if(argCount == 0)
    controller.set(Controller::Three, !controller.read(Controller::Three));
  else if(argCount == 1)
    controller.set(Controller::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0right"
void DebuggerParser::executeJoy0Right()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Left);
  if(argCount == 0)
    controller.set(Controller::Four, !controller.read(Controller::Four));
  else if(argCount == 1)
    controller.set(Controller::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0fire"
void DebuggerParser::executeJoy0Fire()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Left);
  if(argCount == 0)
    controller.set(Controller::Six, !controller.read(Controller::Six));
  else if(argCount == 1)
    controller.set(Controller::Six, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1up"
void DebuggerParser::executeJoy1Up()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Right);
  if(argCount == 0)
    controller.set(Controller::One, !controller.read(Controller::One));
  else if(argCount == 1)
    controller.set(Controller::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1down"
void DebuggerParser::executeJoy1Down()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Right);
  if(argCount == 0)
    controller.set(Controller::Two, !controller.read(Controller::Two));
  else if(argCount == 1)
    controller.set(Controller::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1left"
void DebuggerParser::executeJoy1Left()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Right);
  if(argCount == 0)
    controller.set(Controller::Three, !controller.read(Controller::Three));
  else if(argCount == 1)
    controller.set(Controller::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1right"
void DebuggerParser::executeJoy1Right()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Right);
  if(argCount == 0)
    controller.set(Controller::Four, !controller.read(Controller::Four));
  else if(argCount == 1)
    controller.set(Controller::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1fire"
void DebuggerParser::executeJoy1Fire()
{
  Controller& controller = debugger.riotDebug().controller(Controller::Right);
  if(argCount == 0)
    controller.set(Controller::Six, !controller.read(Controller::Six));
  else if(argCount == 1)
    controller.set(Controller::Six, args[0] != 0);
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
      if(! (++count % 8) ) buf << endl;
    }
  }
  if(count)
    commandResult << "breaks:" << endl << buf.str();

  StringList conds = debugger.m6502().getCondBreakNames();
  if(conds.size() > 0)
  {
    if(count)
      commandResult << endl;
    commandResult << "breakifs:" << endl;
    for(uInt32 i = 0; i < conds.size(); i++)
    {
      commandResult << Base::toString(i) << ": " << conds[i];
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
// "listsavestateifs"
void DebuggerParser::executeListsavestateifs()
{
  ostringstream buf;

  StringList conds = debugger.m6502().getCondSaveStateNames();
  if(conds.size() > 0)
  {
    commandResult << "savestateif:" << endl;
    for(uInt32 i = 0; i < conds.size(); i++)
    {
      commandResult << Base::toString(i) << ": " << conds[i];
      if(i != (conds.size() - 1)) commandResult << endl;
    }
  }

  if(commandResult.str() == "")
    commandResult << "no savestateifs defined";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listtraps"
void DebuggerParser::executeListtraps()
{
  StringList names = debugger.m6502().getCondTrapNames();

  if(myTraps.size() != names.size())
  {
    commandResult << "Internal error! Different trap sizes.";
    return;
  }

  if (names.size() > 0)
  {
    bool trapFound = false, trapifFound = false;
    for(uInt32 i = 0; i < names.size(); i++)
      if(names[i] == "")
        trapFound = true;
      else
        trapifFound = true;

    if(trapFound)
      listTraps(false);
    if(trapifFound)
      listTraps(true);
  }
  else
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
    outputCommandError("specify start and end of range only", myCommand);
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
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
  debugger.riotDebug().controller(Controller::Left).set(Controller::One, true);
  debugger.riotDebug().controller(Controller::Left).set(Controller::Two, true);
  debugger.riotDebug().controller(Controller::Left).set(Controller::Three, true);
  debugger.riotDebug().controller(Controller::Left).set(Controller::Four, true);
  debugger.riotDebug().controller(Controller::Left).set(Controller::Six, true);
  debugger.riotDebug().controller(Controller::Right).set(Controller::One, true);
  debugger.riotDebug().controller(Controller::Right).set(Controller::Two, true);
  debugger.riotDebug().controller(Controller::Right).set(Controller::Three, true);
  debugger.riotDebug().controller(Controller::Right).set(Controller::Four, true);
  debugger.riotDebug().controller(Controller::Right).set(Controller::Six, true);
  commandResult << "reset system";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "rewind"
void DebuggerParser::executeRewind()
{
  executeWinds(false);
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
    outputCommandError("specify start and end of range only", myCommand);
    return;
  }
  else if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
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
  commandResult << saveScriptFile(argStrings[0]);
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
  // Create a file named with the current date and time
  time_t currtime;
  struct tm* timeinfo;
  char buffer[80];

  time(&currtime);
  timeinfo = localtime(&currtime);
  strftime(buffer, 80, "session_%F_%H-%M-%S.txt", timeinfo);

  FilesystemNode file(debugger.myOSystem.defaultSaveDir() + buffer);
  if(debugger.prompt().saveBuffer(file))
    commandResult << "saved " + file.getShortPath() + " OK";
  else
    commandResult << "unable to save session";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "savesnap"
void DebuggerParser::executeSavesnap()
{
  debugger.tiaOutput().saveSnapshot(execDepth, execPrefix);
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
// "savestateif"
void DebuggerParser::executeSavestateif()
{
  int res = YaccParser::parse(argStrings[0].c_str());
  if(res == 0)
  {
    string condition = argStrings[0];
    for(uInt32 i = 0; i < debugger.m6502().getCondSaveStateNames().size(); i++)
    {
      if(condition == debugger.m6502().getCondSaveStateNames()[i])
      {
        args[0] = i;
        executeDelsavestateif();
        return;
      }
    }
    uInt32 ret = debugger.m6502().addCondSaveState(
      YaccParser::getResult(), argStrings[0]);
    commandResult << "added savestateif " << Base::toString(ret);
  }
  else
    commandResult << red("invalid expression");
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
// "stepwhile"
void DebuggerParser::executeStepwhile()
{
  int res = YaccParser::parse(argStrings[0].c_str());
  if(res != 0) {
    commandResult << red("invalid expression");
    return;
  }
  Expression* expr = YaccParser::getResult();
  int ncycles = 0;
  do {
    ncycles += debugger.step();
  } while (expr->evaluate());
  commandResult << "executed " << ncycles << " cycles";
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
  executeTraps(true, true, "trap");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapif"
void DebuggerParser::executeTrapif()
{
  executeTraps(true, true, "trapif", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapread"
void DebuggerParser::executeTrapread()
{
  executeTraps(true, false, "trapread");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapreadif"
void DebuggerParser::executeTrapreadif()
{
  executeTraps(true, false, "trapreadif", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapwrite"
void DebuggerParser::executeTrapwrite()
{
  executeTraps(false, true, "trapwrite");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapwriteif"
void DebuggerParser::executeTrapwriteif()
{
  executeTraps(false, true, "trapwriteif", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Wrapper function for trap(if)s
void DebuggerParser::executeTraps(bool read, bool write, const string& command,
                                  bool hasCond)
{
  uInt32 ofs = hasCond ? 1 : 0;
  uInt32 begin = args[ofs];
  uInt32 end = argCount == 2 + ofs ? args[1 + ofs] : begin;

  if(argCount < 1 + ofs)
  {
    outputCommandError("missing required argument(s)", myCommand);
    return;
  }
  if(argCount > 2 + ofs)
  {
    outputCommandError("too many arguments", myCommand);
    return;
  }
  if(begin > 0xFFFF || end > 0xFFFF)
  {
    commandResult << red("invalid word argument(s) (must be 0-$ffff)");
    return;
  }
  if(begin > end)
  {
    commandResult << red("start address must be <= end address");
    return;
  }

  // base addresses of mirrors
  uInt32 beginRead = debugger.getBaseAddress(begin, true);
  uInt32 endRead = debugger.getBaseAddress(end, true);
  uInt32 beginWrite = debugger.getBaseAddress(begin, false);
  uInt32 endWrite = debugger.getBaseAddress(end, false);
  stringstream conditionBuf;

  // parenthesize provided and address range condition(s) (begin)
  if(hasCond)
    conditionBuf << "(" << argStrings[0] << ")&&(";

  // add address range condition(s) to provided condition
  if(read)
  {
    if(beginRead != endRead)
      conditionBuf << "__lastread>=" << Base::toString(beginRead) << "&&__lastread<=" << Base::toString(endRead);
    else
      conditionBuf << "__lastread==" << Base::toString(beginRead);
  }
  if(read && write)
    conditionBuf << "||";
  if(write)
  {
    if(beginWrite != endWrite)
      conditionBuf << "__lastwrite>=" << Base::toString(beginWrite) << "&&__lastwrite<=" << Base::toString(endWrite);
    else
      conditionBuf << "__lastwrite==" << Base::toString(beginWrite);
  }
  // parenthesize provided condition (end)
  if(hasCond)
    conditionBuf << ")";

  const string condition = conditionBuf.str();

  int res = YaccParser::parse(condition.c_str());
  if(res == 0)
  {
    // duplicates will remove each other
    bool add = true;
    for(uInt32 i = 0; i < myTraps.size(); i++)
    {
      if(myTraps[i]->begin == begin && myTraps[i]->end == end &&
         myTraps[i]->read == read && myTraps[i]->write == write &&
         myTraps[i]->condition == condition)
      {
        if(debugger.m6502().delCondTrap(i))
        {
          add = false;
          // @sa666666: please check this:
          Vec::removeAt(myTraps, i);
          commandResult << "removed trap " << Base::toString(i);
          break;
        }
        commandResult << "Internal error! Duplicate trap removal failed!";
        return;
      }
    }
    if(add)
    {
      uInt32 ret = debugger.m6502().addCondTrap(
        YaccParser::getResult(), hasCond ? argStrings[0] : "");
      commandResult << "added trap " << Base::toString(ret);

      // @sa666666: please check this:
      myTraps.emplace_back(new Trap{ read, write, begin, end, condition });
    }

    for(uInt32 addr = begin; addr <= end; ++addr)
      executeTrapRW(addr, read, write, add);
  }
  else
  {
    commandResult << red("invalid expression");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// wrapper function for trap(if)/trapread(if)/trapwrite(if) commands
void DebuggerParser::executeTrapRW(uInt32 addr, bool read, bool write, bool add)
{
  switch(debugger.cartDebug().addressType(addr))
  {
    case CartDebug::ADDR_TIA:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1080) == 0x0000)
        {
          // @sa666666: This seems wrong. E.g. trapread 40 4f will never trigger
          if(read && (i & 0x000F) == (addr & 0x000F))
            add ? debugger.addReadTrap(i) : debugger.removeReadTrap(i);
          if(write && (i & 0x003F) == (addr & 0x003F))
            add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
        }
      }
      break;
    }
    case CartDebug::ADDR_IO:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1280) == 0x0280 && (i & 0x029F) == (addr & 0x029F))
        {
          if(read)
            add ? debugger.addReadTrap(i) : debugger.removeReadTrap(i);
          if(write)
            add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
        }
      }
      break;
    }
    case CartDebug::ADDR_ZPRAM:
    {
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1280) == 0x0080 && (i & 0x00FF) == (addr & 0x00FF))
        {
          if(read)
            add ? debugger.addReadTrap(i) : debugger.removeReadTrap(i);
          if(write)
            add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
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
            if(read)
              add ? debugger.addReadTrap(i) : debugger.removeReadTrap(i);
            if(write)
              add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
          }
        }
      }
      break;
    }
  }
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
// "unwind"
void DebuggerParser::executeUnwind()
{
  executeWinds(true);
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
  myWatches.push_back(argStrings[0]);
  commandResult << "added watch \"" << argStrings[0] << "\"";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// wrapper function for rewind/unwind commands
void DebuggerParser::executeWinds(bool unwind)
{
  uInt16 states;
  string type = unwind ? "unwind" : "rewind";
  string message;

  if(argCount == 0)
    states = 1;
  else
    states = args[0];

  uInt16 winds = unwind ? debugger.unwindStates(states, message) : debugger.rewindStates(states, message);
  if(winds > 0)
  {
    debugger.rom().invalidate();
    commandResult << type << " by " << winds << " state" << (winds > 1 ? "s" : "");
    commandResult << " (~" << message << ")";
  }
  else
    commandResult << "no states left to " << type;
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
    "Set default number base to <base>",
    "Base is #2, #10, #16, bin, dec or hex\nExample: base hex",
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
    "Set/clear breakpoint on <condition>",
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
    "clearsavestateifs",
    "Clear all savestate points",
    "Example: clearsavestateifss (no parameters)",
    false,
    true,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearsavestateifs)
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
    "Decimal Flag: set (0 or 1), or toggle (no arg)",
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
    "delsavestateif",
    "Delete conditional savestate point <xx>",
    "Example: delsavestateif 0",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelsavestateif)
  },

  {
    "deltrap",
    "Delete trap <xx>",
    "Example: deltrap 0",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDeltrap)
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
    "Dump data at address <xx> [to yy] [1: memory; 2: CPU state; 4: input regs]",
    "Example:\n"
    "  dump f000 - dumps 128 bytes @ f000\n"
    "  dump f000 f0ff - dumps all bytes from f000 to f0ff\n"
    "  dump f000 f0ff 7 - dumps all bytes from f000 to f0ff, CPU state and input registers into a file",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeDump)
  },

  {
    "exec",
    "Execute script file <xx> [prefix]",
    "Example: exec script.dat, exec auto.txt",
    true,
    true,
    { kARG_FILE, kARG_LABEL, kARG_MULTI_BYTE },
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
    "Example: function FUNC1 { ... }",
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
    "joy0up",
    "Set joystick 0 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0up 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Up)
  },

  {
    "joy0down",
    "Set joystick 0 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0down 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Down)
  },

  {
    "joy0left",
    "Set joystick 0 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0left 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Left)
  },

  {
    "joy0right",
    "Set joystick 0 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0left 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Right)
  },

  {
    "joy0fire",
    "Set joystick 0 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0fire 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Fire)
  },

  {
    "joy1up",
    "Set joystick 1 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1up 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Up)
  },

  {
    "joy1down",
    "Set joystick 1 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1down 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Down)
  },

  {
    "joy1left",
    "Set joystick 1 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1left 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Left)
  },

  {
    "joy1right",
    "Set joystick 1 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1left 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Right)
  },

  {
    "joy1fire",
    "Set joystick 1 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1fire 0",
    false,
    true,
    { kARG_BOOL, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Fire)
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
    "listsavestateifs",
    "List savestate points",
    "Example: listsavestateifs (no parameters)",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListsavestateifs)
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
    { kARG_DWORD, kARG_END_ARGS },
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
    "Rewind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: rewind, rewind 5",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
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
    "Save breaks, watches, traps and functions to file xx",
    "Example: save commands.script",
    true,
    false,
    { kARG_FILE, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSave)
  },

  {
    "saveconfig",
    "Save Distella config file (with default name)",
    "Example: saveconfig",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveconfig)
  },

  {
    "savedis",
    "Save Distella disassembly (with default name)",
    "Example: savedis\n"
    "NOTE: saves to default save location",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavedisassembly)
  },

  {
    "saverom",
    "Save (possibly patched) ROM (with default name)",
    "Example: saverom\n"
    "NOTE: saves to default save location",
    false,
    false,
    { kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaverom)
  },

  {
    "saveses",
    "Save console session (with default name)",
    "Example: saveses\n"
    "NOTE: saves to default save location",
    false,
    false,
    { kARG_END_ARGS },
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
    "savestateif",
    "Create savestate on <condition>",
    "Condition can include multiple items, see documentation\nExample: savestateif pc==f000",
    true,
    false,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavestateif)
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
    "stepwhile",
    "Single step CPU while <condition> is true",
    "Example: stepwhile pc!=$f2a9",
    true,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeStepwhile)
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
    "Set/clear a R/W trap on the given address(es) and all mirrors\n"
    "Example: trap f000, trap f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrap)
  },

  {
    "trapif",
    "On <condition> trap R/W access to address(es) xx [yy]",
    "Set/clear a conditional R/W trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapif _scan>#100 GRP0, trapif _bank==1 f000 f100",
      true,
      false,
      { kARG_WORD, kARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapif)
  },

  {
    "trapread",
    "Trap read access to address(es) xx [yy]",
    "Set/clear a read trap on the given address(es) and all mirrors\n"
    "Example: trapread f000, trapread f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapread)
  },

  {
    "trapreadif",
    "On <condition> trap read access to address(es) xx [yy]",
    "Set/clear a conditional read trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapreadif _scan>#100 GRP0, trapreadif _bank==1 f000 f100",
      true,
      false,
      { kARG_WORD, kARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapreadif)
  },

  {
    "trapwrite",
    "Trap write access to address(es) xx [yy]",
    "Set/clear a write trap on the given address(es) and all mirrors\n"
    "Example: trapwrite f000, trapwrite f000 f100",
    true,
    false,
    { kARG_WORD, kARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapwrite)
  },

  {
    "trapwriteif",
    "On <condition> trap write access to address(es) xx [yy]",
    "Set/clear a conditional write trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapwriteif _scan>#100 GRP0, trapwriteif _bank==1 f000 f100",
      true,
      false,
      { kARG_WORD, kARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapwriteif)
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
    "unwind",
    "Unwind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: unwind, unwind 5",
    false,
    true,
    { kARG_WORD, kARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUnwind)
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
