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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
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
#include "ControlLowLevel.hxx"
#include "TIADebug.hxx"
#include "TiaOutputWidget.hxx"
#include "DebuggerParser.hxx"
#include "YaccParser.hxx"
#include "M6502.hxx"
#include "Expression.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "PromptWidget.hxx"
#include "RomWidget.hxx"
#include "ProgressDialog.hxx"
#include "BrowserDialog.hxx"
#include "FrameBuffer.hxx"
#include "TimerManager.hxx"
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
  : debugger{d},
    settings{s}
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

  for(int i = 0; i < int(commands.size()); ++i)
  {
    if(BSPF::equalsIgnoreCase(verb, commands[i].cmdString))
    {
      if(validateArgs(i))
      {
        myCommand = i;
        if(commands[i].refreshRequired)
          debugger.baseDialog()->saveConfig();
        commands[i].executor(this);
      }

      if(commands[i].refreshRequired)
        debugger.baseDialog()->loadConfig();

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
    stringstream in;
    try        { file.read(in); }
    catch(...) { return red("script file \'" + file.getShortPath() + "\' not found"); }

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
  for(const auto& c: commands)
  {
    if(BSPF::matches(c.cmdString, in))
      completions.push_back(c.cmdString);
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

  Base::Fmt defaultBase = Base::format();

  if(defaultBase == Base::Fmt::_2) {
    bin=true; dec=false;
  } else if(defaultBase == Base::Fmt::_10) {
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
  ParseState state = ParseState::IN_COMMAND;
  uInt32 i = 0, length = uInt32(command.length());
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
      case ParseState::IN_COMMAND:
        if(c == ' ')
          state = ParseState::IN_SPACE;
        else
          verb += c;
        break;
      case ParseState::IN_SPACE:
        if(c == '{')
          state = ParseState::IN_BRACE;
        else if(c != ' ') {
          state = ParseState::IN_ARG;
          curArg += c;
        }
        break;
      case ParseState::IN_BRACE:
        if(c == '}') {
          state = ParseState::IN_SPACE;
          argStrings.push_back(curArg);
          //  cerr << "{" << curArg << "}" << endl;
          curArg = "";
        } else {
          curArg += c;
        }
        break;
      case ParseState::IN_ARG:
        if(c == ' ') {
          state = ParseState::IN_SPACE;
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
  Parameters* p = commands[cmd].parms.data();

  if(argCount == 0)
  {
    if(required)
    {
      void(commandResult.str());
      outputCommandError("missing required argument(s)", cmd);
      return false; // needed args. didn't get 'em.
    }
    else
      return true;  // no args needed, no args got
  }

  // Figure out how many arguments are required by the command
  uInt32 count = 0, argRequiredCount = 0;
  while(*p != Parameters::ARG_END_ARGS && *p != Parameters::ARG_MULTI_BYTE)
  {
    ++count;
    ++p;
  }

  // Evil hack: some commands intentionally take multiple arguments
  // In this case, the required number of arguments is unbounded
  argRequiredCount = (*p == Parameters::ARG_END_ARGS) ? count : argCount;

  p = commands[cmd].parms.data();
  uInt32 curCount = 0;

  do {
    if(curCount >= argCount)
      break;

    uInt32 curArgInt  = args[curCount];
    string& curArgStr = argStrings[curCount];

    switch(*p)
    {
      case Parameters::ARG_DWORD:
      #if 0   // TODO - do we need error checking at all here?
        if(curArgInt > 0xffffffff)
        {
          commandResult.str(red("invalid word argument (must be 0-$ffffffff)"));
          return false;
        }
      #endif
        break;

      case Parameters::ARG_WORD:
        if(curArgInt > 0xffff)
        {
          commandResult.str(red("invalid word argument (must be 0-$ffff)"));
          return false;
        }
        break;

      case Parameters::ARG_BYTE:
        if(curArgInt > 0xff)
        {
          commandResult.str(red("invalid byte argument (must be 0-$ff)"));
          return false;
        }
        break;

      case Parameters::ARG_BOOL:
        if(curArgInt != 0 && curArgInt != 1)
        {
          commandResult.str(red("invalid boolean argument (must be 0 or 1)"));
          return false;
        }
        break;

      case Parameters::ARG_BASE_SPCL:
        if(curArgInt != 2 && curArgInt != 10 && curArgInt != 16
           && curArgStr != "hex" && curArgStr != "dec" && curArgStr != "bin")
        {
          commandResult.str(red("invalid base (must be #2, #10, #16, \"bin\", \"dec\", or \"hex\")"));
          return false;
        }
        break;

      case Parameters::ARG_LABEL:
      case Parameters::ARG_FILE:
        break; // TODO: validate these (for now any string's allowed)

      case Parameters::ARG_MULTI_BYTE:
      case Parameters::ARG_MULTI_WORD:
        break; // FIXME: validate these (for now, any number's allowed)

      case Parameters::ARG_END_ARGS:
        break;
    }
    ++curCount;
    ++p;

  } while(*p != Parameters::ARG_END_ARGS && curCount < argRequiredCount);

/*
cerr << "curCount         = " << curCount << endl
     << "argRequiredCount = " << argRequiredCount << endl
     << "*p               = " << *p << endl << endl;
*/

  if(curCount < argRequiredCount)
  {
    void(commandResult.str());
    outputCommandError("missing required argument(s)", cmd);
    return false;
  }
  else if(argCount > curCount)
  {
    void(commandResult.str());
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

    buf << "$" << Base::toString(args[i], Base::Fmt::_16);

    if(args[i] < 0x10000)
      buf << " %" << Base::toString(args[i], Base::Fmt::_2);

    buf << " #" << int(args[i]);
    if(i != argCount - 1)
      buf << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& DebuggerParser::cartName() const
{
  return debugger.myOSystem.console().properties().get(PropType::Cart_Name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::listTraps(bool listCond)
{
  StringList names = debugger.m6502().getCondTrapNames();

  commandResult << (listCond ? "trapifs:" : "traps:") << endl;
  for(uInt32 i = 0; i < names.size(); ++i)
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
  stringstream out;
  Debugger::FunctionDefMap funcs = debugger.getFunctionDefMap();
  for(const auto& [name, cmd]: funcs)
    if (!debugger.isBuiltinFunction(name))
      out << "function " << name << " {" << cmd << "}" << endl;

  for(const auto& w: myWatches)
    out << "watch " << w << endl;

  for(const auto& bp: debugger.breakPoints().getBreakpoints())
    out << "break " << Base::toString(bp.addr) << " " << Base::toString(bp.bank) << endl;

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

  // Append 'script' extension when necessary
  if(file.find_last_of('.') == string::npos)
    file += ".script";

  // Use user dir if no path is provided
  if(file.find_first_of(FilesystemNode::PATH_SEPARATOR) == string::npos)
    file = debugger.myOSystem.userDir().getPath() + file;

  FilesystemNode node(file);

  try
  {
    node.write(out);
  }
  catch(...)
  {
    return "Unable to save script to " + node.getShortPath();
  }

  return "saved " + node.getShortPath() + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::saveDump(const FilesystemNode& node, const stringstream& out,
                              ostringstream& result)
{
  try
  {
    node.write(out);
    result << " to file " << node.getShortPath();
  }
  catch(...)
  {
    result.str(red("Unable to append dump to file " + node.getShortPath()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::executeDirective(Device::AccessType type)
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

  bool result = debugger.cartDebug().addDirective(type, args[0], args[1]);

  commandResult << (result ? "added " : "removed ");
  debugger.cartDebug().AccessTypeAsString(commandResult, type);
  commandResult << " directive on range $"
    << hex << args[0] << " $" << hex << args[1];
  debugger.rom().invalidate();
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
// "aud"
void DebuggerParser::executeAud()
{
  executeDirective(Device::AUD);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "base"
void DebuggerParser::executeBase()
{
  if(args[0] == 2 || argStrings[0] == "bin")
    Base::setFormat(Base::Fmt::_2);
  else if(args[0] == 10 || argStrings[0] == "dec")
    Base::setFormat(Base::Fmt::_10);
  else if(args[0] == 16 || argStrings[0] == "hex")
    Base::setFormat(Base::Fmt::_16);

  commandResult << "default number base set to ";
  switch(Base::format()) {
    case Base::Fmt::_2:
      commandResult << "#2/bin";
      break;

    case Base::Fmt::_10:
      commandResult << "#10/dec";
      break;

    case Base::Fmt::_16:
      commandResult << "#16/hex";
      break;

    default:
      commandResult << red("UNKNOWN");
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "bcol"
void DebuggerParser::executeBCol()
{
  executeDirective(Device::BCOL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "break"
void DebuggerParser::executeBreak()
{
  uInt16 addr;
  uInt8 bank;
  uInt32 romBankCount = debugger.cartDebug().romBankCount();

  if(argCount == 0)
    addr = debugger.cpuDebug().pc();
  else
    addr = args[0];

  if(argCount < 2)
    bank = debugger.cartDebug().getBank(addr);
  else
  {
    bank = args[1];
    if(bank >= romBankCount && bank != 0xff)
    {
      commandResult << red("invalid bank");
      return;
    }
  }
  if(bank != 0xff)
  {
    bool set = debugger.toggleBreakPoint(addr, bank);

    if(set)
      commandResult << "set";
    else
      commandResult << "cleared";

    commandResult << " breakpoint at $" << Base::HEX4 << addr << " + mirrors";
    if(romBankCount > 1)
      commandResult << " in bank #" << std::dec << int(bank);
  }
  else
  {
    for(int i = 0; i < debugger.cartDebug().romBankCount(); ++i)
    {
      bool set = debugger.toggleBreakPoint(addr, i);

      if(i)
        commandResult << endl;

      if(set)
        commandResult << "set";
      else
        commandResult << "cleared";

      commandResult << " breakpoint at $" << Base::HEX4 << addr << " + mirrors";
      if(romBankCount > 1)
        commandResult << " in bank #" << std::dec << int(bank);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breakif"
void DebuggerParser::executeBreakif()
{
  int res = YaccParser::parse(argStrings[0].c_str());
  if(res == 0)
  {
    string condition = argStrings[0];
    for(uInt32 i = 0; i < debugger.m6502().getCondBreakNames().size(); ++i)
    {
      if(condition == debugger.m6502().getCondBreakNames()[i])
      {
        args[0] = i;
        executeDelbreakif();
        return;
      }
    }
    uInt32 ret = debugger.m6502().addCondBreak(
                 YaccParser::getResult(), argStrings[0]);
    commandResult << "added breakif " << Base::toString(ret);
  }
  else
    commandResult << red("invalid expression");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breaklabel"
void DebuggerParser::executeBreaklabel()
{
  uInt16 addr;

  if(argCount == 0)
    addr = debugger.cpuDebug().pc();
  else
    addr = args[0];

  bool set = debugger.toggleBreakPoint(addr, BreakpointMap::ANY_BANK);

  commandResult << (set ? "set" : "cleared");
  commandResult << " breakpoint at $" << Base::HEX4 << addr << " (no mirrors)";
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
  executeDirective(Device::CODE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "col"
void DebuggerParser::executeCol()
{
  executeDirective(Device::COL);
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
  executeDirective(Device::DATA);
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

  commandResult << debugger.cartDebug().disassembleLines(start, lines);
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
  if(argCount == 0 || argCount > 4)
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
    if((args[2] & 0x07) == 0)
    {
      commandResult << red("dump flags must be 1..7");
      return;
    }
    if(argCount == 4 && argStrings[3] != "?")
    {
      commandResult << red("browser dialog parameter must be '?'");
      return;
    }

    ostringstream path;
    path << debugger.myOSystem.userDir() << cartName() << "_dbg_";
    if(execDepth > 0)
      path << execPrefix;
    else
      path << std::hex << std::setw(8) << std::setfill('0')
           << uInt32(TimerManager::getTicks() / 1000);
    path << ".dump";

    commandResult << "dumped ";

    stringstream out;
    if((args[2] & 0x01) != 0)
    {
      // dump memory
      dump(out, args[0], args[1]);
      commandResult << "bytes from $" << hex << args[0] << " to $" << hex << args[1];
      if((args[2] & 0x06) != 0)
        commandResult << ", ";
    }
    if((args[2] & 0x02) != 0)
    {
      // dump CPU state
      CpuDebug& cpu = debugger.cpuDebug();
      out << "   <PC>PC SP  A  X  Y  -  -    N  V  B  D  I  Z  C  -\n";
      out << "XC: "
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
      out << "   SWA - SWB  - IT  -  -  -   I0 I1 I2 I3 I4 I5 -  -\n";
      out << "XS: "
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

    if(argCount == 4)
    {
      // FIXME: C++ doesn't currently allow capture of stringstreams
      //        So we pass a copy of its contents, then re-create the
      //        stream inside the lambda
      //        Maybe this will change in a future version
      const string outStr = out.str();
      const string resultStr = commandResult.str();

      DebuggerDialog* dlg = debugger.myDialog;
      BrowserDialog::show(dlg, "Save Dump as", path.str(),
                          BrowserDialog::Mode::FileSave,
                          [this, dlg, outStr, resultStr]
                          (bool OK, const FilesystemNode& node)
      {
        if(OK)
        {
          stringstream  localOut(outStr);
          ostringstream localResult(resultStr, std::ios_base::app);

          saveDump(node, localOut, localResult);
          dlg->prompt().print(localResult.str() + '\n');
        }
        dlg->prompt().printPrompt();
      });
      // avoid printing a new prompt
      commandResult.str("_NO_PROMPT");
    }
    else
      saveDump(FilesystemNode(path.str()), out, commandResult);
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
  if (!node.exists())
    node = FilesystemNode(debugger.myOSystem.userDir().getPath() + file);

  if (argCount == 2) {
    execPrefix = argStrings[1];
  }
  else {
    ostringstream prefix;
    prefix << std::hex << std::setw(8) << std::setfill('0')
           << uInt32(TimerManager::getTicks()/1000);
    execPrefix = prefix.str();
  }

  // make sure the commands are added to prompt history
  StringList history;

  ++execDepth;
  commandResult << exec(node, &history);
  --execDepth;

  for(const auto& item : history)
    debugger.prompt().addToHistory(item.c_str());
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
  executeDirective(Device::GFX);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "help"
void DebuggerParser::executeHelp()
{
  if(argCount == 0)  // normal help, show all commands
  {
    // Find length of longest command
    uInt32 clen = 0;
    for(const auto& c: commands)
    {
      uInt32 len = uInt32(c.cmdString.length());
      if(len > clen)  clen = len;
    }

    commandResult << setfill(' ');
    for(const auto& c: commands)
      commandResult << setw(clen) << right << c.cmdString
                    << " - " << c.description << endl;

    commandResult << debugger.builtinHelp();
  }
  else  // get help for specific command
  {
    for(const auto& c: commands)
    {
      if(argStrings[0] == c.cmdString)
      {
        commandResult << "  " << red(c.description) << endl << c.extendedDesc;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0up"
void DebuggerParser::executeJoy0Up()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::One);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0down"
void DebuggerParser::executeJoy0Down()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Two);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0left"
void DebuggerParser::executeJoy0Left()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Three);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0right"
void DebuggerParser::executeJoy0Right()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Four);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0fire"
void DebuggerParser::executeJoy0Fire()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Six);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Six, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1up"
void DebuggerParser::executeJoy1Up()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::One);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1down"
void DebuggerParser::executeJoy1Down()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Two);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1left"
void DebuggerParser::executeJoy1Left()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Three);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1right"
void DebuggerParser::executeJoy1Right()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Four);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1fire"
void DebuggerParser::executeJoy1Fire()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Six);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Six, args[0] != 0);
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
  stringstream buf;
  int count = 0;
  uInt32 romBankCount = debugger.cartDebug().romBankCount();

  for(const auto& bp : debugger.breakPoints().getBreakpoints())
  {
    if(romBankCount == 1)
    {
      buf << debugger.cartDebug().getLabel(bp.addr, true, 4) << " ";
      if(!(++count % 8)) buf << endl;
    }
    else
    {
      if(count % 6)
        buf << ", ";
      buf << debugger.cartDebug().getLabel(bp.addr, true, 4);
      if(bp.bank != 255)
        buf << " #" << int(bp.bank);
      else
        buf << " *";
      if(!(++count % 6)) buf << endl;
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
    for(uInt32 i = 0; i < conds.size(); ++i)
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
  const Debugger::FunctionDefMap& functions = debugger.getFunctionDefMap();

  if(functions.size() > 0)
    for(const auto& [name, cmd]: functions)
      commandResult << name << " -> " << cmd << endl;
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
    for(uInt32 i = 0; i < conds.size(); ++i)
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
    for(uInt32 i = 0; i < names.size(); ++i)
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
// "loadallstates"
void DebuggerParser::executeLoadallstates()
{
  debugger.loadAllStates();
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
// "pcol"
void DebuggerParser::executePCol()
{
  executeDirective(Device::PCOL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pgfx"
void DebuggerParser::executePGfx()
{
  executeDirective(Device::PGFX);
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

  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  lport.resetDigitalPins();
  rport.resetDigitalPins();

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
  executeDirective(Device::ROW);
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

  debugger.saveOldState();

  uInt32 count = 0, max_iterations = uInt32(list.size());

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ostringstream buf;
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());

  buf << "RunTo searching through " << max_iterations << " disassembled instructions"
    << progress.ELLIPSIS;
  progress.setMessage(buf.str());
  progress.setRange(0, max_iterations, 5);
  progress.open();

  bool done = false;
  do {
    debugger.step(false);

    // Update romlist to point to current PC
    int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    if(pcline >= 0)
    {
      const string& next = list[pcline].disasm;
      done = (BSPF::findIgnoreCase(next, argStrings[0]) != string::npos);
    }
    // Update the progress bar
    progress.incProgress();
  } while(!done && ++count < max_iterations && !progress.isCancelled());

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

  debugger.saveOldState();

  uInt32 count = 0;
  bool done = false;
  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ostringstream buf;
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());

  buf << "        RunTo PC running" << progress.ELLIPSIS << "        ";
  progress.setMessage(buf.str());
  progress.setRange(0, 100000, 5);
  progress.open();

  do {
    debugger.step(false);

    // Update romlist to point to current PC
    int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    done = (pcline >= 0) && (list[pcline].address == args[0]);
    progress.incProgress();
    ++count;
  } while(!done && !progress.isCancelled());
  progress.close();

  if(done)
    commandResult
      << "Set PC to $" << Base::HEX4 << args[0] << " in "
      << dec << count << " instructions";
  else
    commandResult
      << "PC $" << Base::HEX4 << args[0] << " not reached or found in "
      << dec << count << " instructions";
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
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Workbench as",
                        dlg->instance().userDir().getPath() + cartName() + ".script",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FilesystemNode& node)
    {
      if(OK)
        dlg->prompt().print(saveScriptFile(node.getPath()) + '\n');
      dlg->prompt().printPrompt();
    });
    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
    commandResult << saveScriptFile(argStrings[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveaccess"
void DebuggerParser::executeSaveAccess()
{
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Access Counters as",
                        dlg->instance().userDir().getPath() + cartName() + ".csv",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FilesystemNode& node)
    {
      if(OK)
        dlg->prompt().print(debugger.cartDebug().saveAccessFile(node.getPath()) + '\n');
      dlg->prompt().printPrompt();
    });
    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
    commandResult << debugger.cartDebug().saveAccessFile();
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
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Disassembly as",
                        dlg->instance().userDir().getPath() + cartName() + ".asm",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FilesystemNode& node)
    {
      if(OK)
        dlg->prompt().print(debugger.cartDebug().saveDisassembly(node.getPath()) + '\n');
      dlg->prompt().printPrompt();
    });
    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
    commandResult << debugger.cartDebug().saveDisassembly();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saverom"
void DebuggerParser::executeSaverom()
{
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save ROM as",
                        dlg->instance().userDir().getPath() + cartName() + ".a26",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FilesystemNode& node)
    {
      if(OK)
        dlg->prompt().print(debugger.cartDebug().saveRom(node.getPath()) + '\n');
      dlg->prompt().printPrompt();
    });
    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
    commandResult << debugger.cartDebug().saveRom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveses"
void DebuggerParser::executeSaveses()
{
  ostringstream filename;
  auto timeinfo = BSPF::localTime();
  filename << std::put_time(&timeinfo, "session_%F_%H-%M-%S.txt");

  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Session as",
                        dlg->instance().userDir().getPath() + filename.str(),
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FilesystemNode& node)
    {
      if(OK)
        dlg->prompt().print(debugger.prompt().saveBuffer(node) + '\n');
      dlg->prompt().printPrompt();
    });
    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
  {
    ostringstream path;

    if(argCount)
      path << argStrings[0];
    else
      path << debugger.myOSystem.userDir() << filename.str();

    commandResult << debugger.prompt().saveBuffer(FilesystemNode(path.str()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "savesnap"
void DebuggerParser::executeSavesnap()
{
  debugger.tiaOutput().saveSnapshot(execDepth, execPrefix);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveallstates"
void DebuggerParser::executeSaveallstates()
{
  debugger.saveAllStates();
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
    for(uInt32 i = 0; i < debugger.m6502().getCondSaveStateNames().size(); ++i)
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
  uInt32 count = 0;

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ostringstream buf;
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());

  buf << "stepwhile running through disassembled instructions"
    << progress.ELLIPSIS;
  progress.setMessage(buf.str());
  progress.setRange(0, 100000, 5);
  progress.open();

  do {
    ncycles += debugger.step(false);

    progress.incProgress();
    ++count;
  } while (expr->evaluate() && !progress.isCancelled());

  progress.close();
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
      conditionBuf << "__lastbaseread>=" << Base::toString(beginRead) << "&&__lastbaseread<=" << Base::toString(endRead);
    else
      conditionBuf << "__lastbaseread==" << Base::toString(beginRead);
  }
  if(read && write)
    conditionBuf << "||";
  if(write)
  {
    if(beginWrite != endWrite)
      conditionBuf << "__lastbasewrite>=" << Base::toString(beginWrite) << "&&__lastbasewrite<=" << Base::toString(endWrite);
    else
      conditionBuf << "__lastbasewrite==" << Base::toString(beginWrite);
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
    for(uInt32 i = 0; i < myTraps.size(); ++i)
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

      myTraps.emplace_back(make_unique<Trap>(read, write, begin, end, condition));
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
    case CartDebug::AddrType::TIA:
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
    case CartDebug::AddrType::IO:
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
    case CartDebug::AddrType::ZPRAM:
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
    case CartDebug::AddrType::ROM:
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
    debugger.cartDebug().accessTypeAsString(commandResult, i);
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
std::array<DebuggerParser::Command, 100> DebuggerParser::commands = { {
  {
    "a",
    "Set Accumulator to <value>",
    "Valid value is 0 - ff\nExample: a ff, a #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeA)
  },

  {
    "aud",
    "Mark 'AUD' range in disassembly",
    "Start and end of range required\nExample: aud f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeAud)
  },

  {
    "base",
    "Set default number base to <base>",
    "Base is #2, #10, #16, bin, dec or hex\nExample: base hex",
    true,
    true,
    { Parameters::ARG_BASE_SPCL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBase)
  },

  {
    "bcol",
    "Mark 'BCOL' range in disassembly",
    "Start and end of range required\nExample: bcol f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeBCol)
  },


  {
    "break",
    "Break at <address> and <bank>",
    "Set/clear breakpoint on address (and all mirrors) and bank\nDefault are current PC and bank, valid address is 0 - ffff\n"
    "Example: break, break f000, break 7654 3\n         break ff00 ff (= all banks)",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeBreak)
  },

  {
    "breakif",
    "Set/clear breakpoint on <condition>",
    "Condition can include multiple items, see documentation\nExample: breakif _scan>100",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBreakif)
  },

  {
    "breaklabel",
    "Set/clear breakpoint on <address> (no mirrors, all banks)",
    "Example: breaklabel, breaklabel MainLoop",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeBreaklabel)
  },

  {
    "c",
    "Carry Flag: set (0 or 1), or toggle (no arg)",
    "Example: c, c 0, c 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeC)
  },

  {
    "cheat",
    "Use a cheat code (see manual for cheat types)",
    "Example: cheat 0040, cheat abff00",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCheat)
  },

  {
    "clearbreaks",
    "Clear all breakpoints",
    "Example: clearbreaks (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearbreaks)
  },

  {
    "clearconfig",
    "Clear Distella config directives [bank xx]",
    "Example: clearconfig 0, clearconfig 1",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeClearconfig)
  },

  {
    "clearsavestateifs",
    "Clear all savestate points",
    "Example: clearsavestateifss (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearsavestateifs)
  },

  {
    "cleartraps",
    "Clear all traps",
    "All traps cleared, including any mirrored ones\nExample: cleartraps (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCleartraps)
  },

  {
    "clearwatches",
    "Clear all watches",
    "Example: clearwatches (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeClearwatches)
  },

  {
    "cls",
    "Clear prompt area of text",
    "Completely clears screen, but keeps history of commands",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeCls)
  },

  {
    "code",
    "Mark 'CODE' range in disassembly",
    "Start and end of range required\nExample: code f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeCode)
  },

  {
    "col",
    "Mark 'COL' range in disassembly",
    "Start and end of range required\nExample: col f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeCol)
  },

  {
    "colortest",
    "Show value xx as TIA color",
    "Shows a color swatch for the given value\nExample: colortest 1f",
    true,
    false,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeColortest)
  },

  {
    "d",
    "Decimal Flag: set (0 or 1), or toggle (no arg)",
    "Example: d, d 0, d 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeD)
  },

  {
    "data",
    "Mark 'DATA' range in disassembly",
    "Start and end of range required\nExample: data f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeData)
  },

  {
    "debugcolors",
    "Show Fixed Debug Colors information",
    "Example: debugcolors (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDebugColors)
  },

  {
    "define",
    "Define label xx for address yy",
    "Example: define LABEL1 f100",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDefine)
  },

  {
    "delbreakif",
    "Delete conditional breakif <xx>",
    "Example: delbreakif 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelbreakif)
  },

  {
    "delfunction",
    "Delete function with label xx",
    "Example: delfunction FUNC1",
    true,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelfunction)
  },

  {
    "delsavestateif",
    "Delete conditional savestate point <xx>",
    "Example: delsavestateif 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelsavestateif)
  },

  {
    "deltrap",
    "Delete trap <xx>",
    "Example: deltrap 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDeltrap)
  },

  {
    "delwatch",
    "Delete watch <xx>",
    "Example: delwatch 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeDelwatch)
  },

  {
    "disasm",
    "Disassemble address xx [yy lines] (default=PC)",
    "Disassembles from starting address <xx> (default=PC) for <yy> lines\n"
    "Example: disasm, disasm f000 100",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeDisasm)
  },

  {
    "dump",
    "Dump data at address <xx> [to yy] [1: memory; 2: CPU state; 4: input regs] [?]",
    "Example:\n"
    "  dump f000 - dumps 128 bytes from f000\n"
    "  dump f000 f0ff - dumps all bytes from f000 to f0ff\n"
    "  dump f000 f0ff 7 - dumps all bytes from f000 to f0ff,\n"
    "    CPU state and input registers into a file in user dir,\n"
    "  dump f000 f0ff 7 ? - same, but with a browser dialog\n",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_WORD, Parameters::ARG_BYTE, Parameters::ARG_LABEL },
    std::mem_fn(&DebuggerParser::executeDump)
  },

  {
    "exec",
    "Execute script file <xx> [prefix]",
    "Example: exec script.dat, exec auto.txt",
    true,
    true,
    { Parameters::ARG_FILE, Parameters::ARG_LABEL, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeExec)
  },

  {
    "exitrom",
    "Exit emulator, return to ROM launcher",
    "Self-explanatory",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeExitRom)
  },

  {
    "frame",
    "Advance emulation by <xx> frames (default=1)",
    "Example: frame, frame 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeFrame)
  },

  {
    "function",
    "Define function name xx for expression yy",
    "Example: function FUNC1 { ... }",
    true,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeFunction)
  },

  {
    "gfx",
    "Mark 'GFX' range in disassembly",
    "Start and end of range required\nExample: gfx f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeGfx)
  },

  {
    "help",
    "help <command>",
    "Show all commands, or give function for help on that command\n"
    "Example: help, help code",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeHelp)
  },

  {
    "joy0up",
    "Set joystick 0 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0up 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Up)
  },

  {
    "joy0down",
    "Set joystick 0 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0down 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Down)
  },

  {
    "joy0left",
    "Set joystick 0 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Left)
  },

  {
    "joy0right",
    "Set joystick 0 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Right)
  },

  {
    "joy0fire",
    "Set joystick 0 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0fire 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy0Fire)
  },

  {
    "joy1up",
    "Set joystick 1 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1up 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Up)
  },

  {
    "joy1down",
    "Set joystick 1 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1down 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Down)
  },

  {
    "joy1left",
    "Set joystick 1 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Left)
  },

  {
    "joy1right",
    "Set joystick 1 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Right)
  },

  {
    "joy1fire",
    "Set joystick 1 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1fire 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJoy1Fire)
  },

  {
    "jump",
    "Scroll disassembly to address xx",
    "Moves disassembly listing to address <xx>\nExample: jump f400",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeJump)
  },

  {
    "listbreaks",
    "List breakpoints",
    "Example: listbreaks (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListbreaks)
  },

  {
    "listconfig",
    "List Distella config directives [bank xx]",
    "Example: listconfig 0, listconfig 1",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeListconfig)
  },

  {
    "listfunctions",
    "List user-defined functions",
    "Example: listfunctions (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListfunctions)
  },

  {
    "listsavestateifs",
    "List savestate points",
    "Example: listsavestateifs (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListsavestateifs)
  },

  {
    "listtraps",
    "List traps",
    "Lists all traps (read and/or write)\nExample: listtraps (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeListtraps)
  },

  {
    "loadconfig",
    "Load Distella config file",
    "Example: loadconfig",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeLoadconfig)
  },

  {
    "loadallstates",
    "Load all emulator states",
    "Example: loadallstates (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeLoadallstates)
  },

  {
    "loadstate",
    "Load emulator state xx (0-9)",
    "Example: loadstate 0, loadstate 9",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeLoadstate)
  },

  {
    "n",
    "Negative Flag: set (0 or 1), or toggle (no arg)",
    "Example: n, n 0, n 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeN)
  },

  {
    "palette",
    "Show current TIA palette",
    "Example: palette (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePalette)
  },

  {
    "pc",
    "Set Program Counter to address xx",
    "Example: pc f000",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePc)
  },

  {
    "pcol",
    "Mark 'PCOL' range in disassembly",
    "Start and end of range required\nExample: col f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executePCol)
  },

  {
    "pgfx",
    "Mark 'PGFX' range in disassembly",
    "Start and end of range required\nExample: pgfx f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executePGfx)
  },

  {
    "print",
    "Evaluate/print expression xx in hex/dec/binary",
    "Almost anything can be printed (constants, expressions, registers)\n"
    "Example: print pc, print f000",
    true,
    false,
    { Parameters::ARG_DWORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executePrint)
  },

  {
    "ram",
    "Show ZP RAM, or set address xx to yy1 [yy2 ...]",
    "Example: ram, ram 80 00 ...",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRam)
  },

  {
    "reset",
    "Reset system to power-on state",
    "System is completely reset, just as if it was just powered on",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeReset)
  },

  {
    "rewind",
    "Rewind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: rewind, rewind 5",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRewind)
  },

  {
    "riot",
    "Show RIOT timer/input status",
    "Display text-based output of the contents of the RIOT tab",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRiot)
  },

  {
    "rom",
    "Set ROM address xx to yy1 [yy2 ...]",
    "What happens here depends on the current bankswitching scheme\n"
    "Example: rom f000 00 01 ff ...",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRom)
  },

  {
    "row",
    "Mark 'ROW' range in disassembly",
    "Start and end of range required\nExample: row f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeRow)
  },

  {
    "run",
    "Exit debugger, return to emulator",
    "Self-explanatory",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRun)
  },

  {
    "runto",
    "Run until string xx in disassembly",
    "Advance until the given string is detected in the disassembly\n"
    "Example: runto lda",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRunTo)
  },

  {
    "runtopc",
    "Run until PC is set to value xx",
    "Example: runtopc f200",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeRunToPc)
  },

  {
    "s",
    "Set Stack Pointer to value xx",
    "Accepts 8-bit value, Example: s f0",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeS)
  },

  {
    "save",
    "Save breaks, watches, traps and functions to file <xx or ?>",
    "Example: save commands.script, save ?\n"
    "NOTE: saves to user dir by default",
    true,
    false,
    { Parameters::ARG_FILE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSave)
  },

  {
    "saveaccess",
    "Save the access counters to CSV file [?]",
    "Example: saveaccess, saveaccess ?\n"
    "NOTE: saves to user dir by default",
      false,
      false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
      std::mem_fn(&DebuggerParser::executeSaveAccess)
  },

  {
    "saveconfig",
    "Save Distella config file (with default name)",
    "Example: saveconfig",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveconfig)
  },

  {
    "savedis",
    "Save Distella disassembly to file [?]",
    "Example: savedis, savedis ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavedisassembly)
  },

  {
    "saverom",
    "Save (possibly patched) ROM to file [?]",
    "Example: saverom, saverom ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaverom)
  },

  {
    "saveses",
    "Save console session to file [?]",
    "Example: saveses, saveses ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveses)
  },

  {
    "savesnap",
    "Save current TIA image to PNG file",
    "Save snapshot to current snapshot save directory\n"
    "Example: savesnap (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavesnap)
  },

  {
    "saveallstates",
    "Save all emulator states",
    "Example: saveallstates (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSaveallstates)
  },

  {
    "savestate",
    "Save emulator state xx (valid args 0-9)",
    "Example: savestate 0, savestate 9",
    true,
    false,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavestate)
  },

  {
    "savestateif",
    "Create savestate on <condition>",
    "Condition can include multiple items, see documentation\nExample: savestateif pc==f000",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeSavestateif)
  },

  {
    "scanline",
    "Advance emulation by <xx> scanlines (default=1)",
    "Example: scanline, scanline 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeScanline)
  },

  {
    "step",
    "Single step CPU [with count xx]",
    "Example: step, step 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeStep)
  },

  {
    "stepwhile",
    "Single step CPU while <condition> is true",
    "Example: stepwhile pc!=$f2a9",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeStepwhile)
  },

  {
    "tia",
    "Show TIA state",
    "Display text-based output of the contents of the TIA tab",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeTia)
  },

  {
    "trace",
    "Single step CPU over subroutines [with count xx]",
    "Example: trace, trace 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeTrace)
  },

  {
    "trap",
    "Trap read/write access to address(es) xx [yy]",
    "Set/clear a R/W trap on the given address(es) and all mirrors\n"
    "Example: trap f000, trap f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrap)
  },

  {
    "trapif",
    "On <condition> trap R/W access to address(es) xx [yy]",
    "Set/clear a conditional R/W trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapif _scan>#100 GRP0, trapif _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapif)
  },

  {
    "trapread",
    "Trap read access to address(es) xx [yy]",
    "Set/clear a read trap on the given address(es) and all mirrors\n"
    "Example: trapread f000, trapread f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapread)
  },

  {
    "trapreadif",
    "On <condition> trap read access to address(es) xx [yy]",
    "Set/clear a conditional read trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapreadif _scan>#100 GRP0, trapreadif _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapreadif)
  },

  {
    "trapwrite",
    "Trap write access to address(es) xx [yy]",
    "Set/clear a write trap on the given address(es) and all mirrors\n"
    "Example: trapwrite f000, trapwrite f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeTrapwrite)
  },

  {
    "trapwriteif",
    "On <condition> trap write access to address(es) xx [yy]",
    "Set/clear a conditional write trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapwriteif _scan>#100 GRP0, trapwriteif _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      std::mem_fn(&DebuggerParser::executeTrapwriteif)
  },

  {
    "type",
    "Show access type for address xx [yy]",
    "Example: type f000, type f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    std::mem_fn(&DebuggerParser::executeType)
  },

  {
    "uhex",
    "Toggle upper/lowercase HEX display",
    "Note: not all hex output can be changed\n"
    "Example: uhex (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUHex)
  },

  {
    "undef",
    "Undefine label xx (if defined)",
    "Example: undef LABEL1",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUndef)
  },

  {
    "unwind",
    "Unwind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: unwind, unwind 5",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeUnwind)
  },

  {
    "v",
    "Overflow Flag: set (0 or 1), or toggle (no arg)",
    "Example: v, v 0, v 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeV)
  },

  {
    "watch",
    "Print contents of address xx before every prompt",
    "Example: watch ram_80",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeWatch)
  },

  {
    "x",
    "Set X Register to value xx",
    "Valid value is 0 - ff\nExample: x ff, x #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeX)
  },

  {
    "y",
    "Set Y Register to value xx",
    "Valid value is 0 - ff\nExample: y ff, y #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeY)
  },

  {
    "z",
    "Zero Flag: set (0 or 1), or toggle (no arg)",
    "Example: z, z 0, z 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    std::mem_fn(&DebuggerParser::executeZ)
  }
} };
