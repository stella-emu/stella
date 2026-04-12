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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "RiotDebug.hxx"
#include "ControlLowLevel.hxx"
#include "TIADebug.hxx"
#include "TiaOutputWidget.hxx"
#include "YaccParser.hxx"
#include "Expression.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "Settings.hxx"
#include "PromptWidget.hxx"
#include "RomWidget.hxx"
#include "ProgressDialog.hxx"
#include "BrowserDialog.hxx"
#include "FrameBuffer.hxx"
#include "TimerManager.hxx"
#include "Vec.hxx"
#include "bspf.hxx"

#include "Base.hxx"
using Common::Base;
using std::dec;
using std::setfill;
using std::setw;
using std::right;

#ifdef CHEATCODE_SUPPORT
  #include "Cheat.hxx"
  #include "CheatManager.hxx"
#endif

#include "DebuggerParser.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerParser::DebuggerParser(Debugger& d, Settings& s)
  : debugger{d},
    settings{s}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// main entry point: PromptWidget calls this method.
string DebuggerParser::run(string_view command)
{
  string verb;
  getArgs(command, verb);
  commandResult.str("");

  const auto it = std::ranges::find_if(commands,  // NOLINT(readability-qualified-auto)
    [&](const Command& cmd) { return BSPF::equalsIgnoreCase(verb, cmd.cmdString); });

  if(it == commands.end())
    return red("No such command (try \"help\")");

  const int i = static_cast<int>(it - commands.begin());
  if(validateArgs(i))
  {
    myCommand = i;
    if(it->refreshRequired)
      debugger.baseDialog()->saveConfig();
    (this->*it->executor)();
  }
  if(it->refreshRequired)
    debugger.baseDialog()->loadConfig();

  return commandResult.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::exec(const FSNode& file, StringList* history)
{
  if(!file.exists())
    return red("script file \'" + file.getShortPath() + "\' not found");

  std::stringstream in;
  try        { file.read(in); }
  catch(...) { return red("script file \'" + file.getShortPath() + "\' not found"); }

  const bool logExec = settings.getBool("dbg.logexec");
  string buf;     buf.reserve(256);
  string logBuf;  logBuf.reserve(4096);  // log output tends to be large
  int count = 0;

  string command;
  while(getline(in, command))
  {
    // Skip empty/comment lines
    command = BSPF::trim(command);
    if(command.empty() || command[0] == ';')
      continue;

    ++execDepth;
    if(logExec)
    {
      logBuf += "> ";
      logBuf += command;
      logBuf += '\n';
      const string result = run(command);
      if(!result.empty() && result != "_EXIT_DEBUGGER" && result != "_NO_PROMPT")
      {
        logBuf += result;
        logBuf += '\n';
      }
    }
    else
      run(command);
    --execDepth;

    if(history != nullptr)
      history->push_back(command);
    ++count;
  }

  // Write execution log if enabled
  if(logExec && !logBuf.empty())
  {
    const FSNode logNode(file.getPath() + ".output.txt");
    try        { logNode.write(logBuf); }
    catch(...) { buf += red("\nUnable to write exec output to file \'"
                            + logNode.getShortPath() + "\'\n"); }
  }

  buf += "\nExecuted ";
  buf += std::to_string(count);
  buf += count != 1 ? " commands from \"" : " command from \"";
  buf += file.getShortPath();
  buf += '"';
  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::outputCommandError(string_view errorMsg, int command)
{
  const string example = commands[command].extendedDesc.substr(commands[command].extendedDesc.find("Example:"));

  commandResult.str("");
  commandResult << red(errorMsg);
  if(!example.empty())
    commandResult << '\n' << example;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Completion-related stuff:
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::getCompletions(string_view in, StringList& completions)
{
  // cerr << "Attempting to complete \"" << in << "\"\n";
  for(const auto& c: commands)
  {
    if(BSPF::matchesCamelCase(c.cmdString, in))
      completions.push_back(c.cmdString);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Evaluate expression. Expressions always evaluate to a 16-bit value if
// they're valid, or -1 if they're not.
// decipher_arg may be called by the GUI as needed. It is also called
// internally by DebuggerParser::run()
int DebuggerParser::decipher_arg(string_view str)
{
  bool derefByte = false, derefWord = false;
  bool lobyte = false, hibyte = false;
  bool bin = false, dec = false;

  // Work with a string_view throughout to avoid allocation
  string_view arg = str;

  const Base::Fmt defaultBase = Base::format();
  if     (defaultBase == Base::Fmt::_2)  { bin = true;  dec = false; }
  else if(defaultBase == Base::Fmt::_10) { bin = false; dec = true;  }
  else                                   { bin = false; dec = false; }

  // Dereference prefixes
  if     (arg.starts_with('*')) { derefByte = true; arg.remove_prefix(1); }
  else if(arg.starts_with('@')) { derefWord = true; arg.remove_prefix(1); }

  // Byte-select prefixes
  if     (arg.starts_with('<')) { lobyte = true; arg.remove_prefix(1); }
  else if(arg.starts_with('>')) { hibyte = true; arg.remove_prefix(1); }

  // Base-override prefixes
  if     (arg.starts_with('\\')) { bin = true;  dec = false; arg.remove_prefix(1); }
  else if(arg.starts_with('#'))  { bin = false; dec = true;  arg.remove_prefix(1); }
  else if(arg.starts_with('$'))  { bin = false; dec = false; arg.remove_prefix(1); }

  // Special case: registers
  // Note: "$a" must not match the 'a' register, hence the original str check
  const auto& state = static_cast<const CpuState&>(debugger.cpuDebug().getState());
  int result = 0;
  bool resolved = false;

  // Use a small table to avoid a chain of string comparisons
  // Only checked when no base-override prefix was present (arg == original suffix)
  if(!bin && !dec && arg != str.substr(str.size() - arg.size(), arg.size()))
  {
    // A base prefix was consumed — can't be a register name, fall through
  }
  else
  {
    // Register name lookup — only valid when no base-override stripped a leading char
    // We check str to detect the "$a" case (str starts_with '$' means arg=="a" but
    // the user typed a hex literal, not the A register)
    const bool hexDollar = str.starts_with('$');
    if     (!hexDollar &&  arg == "a")  { result = state.A;  resolved = true; }
    else if(               arg == "x")  { result = state.X;  resolved = true; }
    else if(               arg == "y")  { result = state.Y;  resolved = true; }
    else if(               arg == "p")  { result = state.PS; resolved = true; }
    else if(               arg == "s")  { result = state.SP; resolved = true; }
    else if(arg == "pc" || arg == ".")  { result = state.PC; resolved = true; }
  }

  // Label lookup
  if(!resolved)
  {
    const int labelAddr = debugger.cartDebug().getAddress(arg);
    if(labelAddr >= 0)
    {
      result = labelAddr;
      resolved = true;
    }
  }

  // Numeric parsing via std::from_chars (no allocation, no exceptions, no UB)
  if(!resolved)
  {
    if(arg.empty())
      return -1;

    int base = 0;
    if     (bin) base = 2;
    else if(dec) base = 10;
    else         base = 16;

    const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(),
                                           result, base);

    // from_chars succeeds only if it consumed the entire input
    if (ec != std::errc{} || ptr != arg.data() + arg.size())
      return -1;
  }

  // Byte-select
  if     (lobyte) result = result & 0xFF;
  else if(hibyte) result = (result >> 8) & 0xFF;

  // Dereference
  if(derefByte) result = debugger.peek(result);
  if(derefWord) result = debugger.dpeek(result);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::showWatches()
{
  string buf;
  buf.reserve(myWatches.size() * 32);  // rough estimate per watch line

  for(uInt32 i = 0; i < myWatches.size(); ++i)
  {
    const string& watch = myWatches[i];
    if(watch.empty())
      continue;

    // Clear the args, since we're going to pass them to eval()
    argStrings.clear();
    args.clear();
    argCount = 1;
    argStrings.push_back(watch);
    args.push_back(decipher_arg(watch));

    if(args[0] < 0)
      std::format_to(std::back_inserter(buf), "BAD WATCH {}: {}\n", i + 1, watch);
    else
      std::format_to(std::back_inserter(buf), " watch #{} ({}) -> {}\n", i + 1, watch, eval());
  }
  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private methods below
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DebuggerParser::getArgs(string_view command, string& verb)
{
  argStrings.clear();
  args.clear();
  verb.clear();

  // Extract verb as everything up to the first space
  const auto verbEnd = command.find(' ');
  if(verbEnd == string_view::npos)
  {
    verb = command;
    argCount = 0;
    return true;
  }

  verb = command.substr(0, verbEnd);

  // Walk the remainder parsing space-separated tokens,
  // with {brace} quoting for tokens containing spaces
  auto rest = command.substr(verbEnd + 1);
  string curArg;
  curArg.reserve(32);
  ParseState state = ParseState::IN_SPACE;

  for(const char c: rest)
  {
    switch(state)
    {
      case ParseState::IN_SPACE:
        if(c == '{')
          state = ParseState::IN_BRACE;
        else if(c != ' ')
        {
          state = ParseState::IN_ARG;
          curArg += c;
        }
        break;

      case ParseState::IN_BRACE:
        if(c == '}')
        {
          state = ParseState::IN_SPACE;
          argStrings.push_back(std::move(curArg));
          curArg.clear();
        }
        else
          curArg += c;
        break;

      case ParseState::IN_ARG:
        if(c == ' ')
        {
          state = ParseState::IN_SPACE;
          argStrings.push_back(std::move(curArg));
          curArg.clear();
        }
        else
          curArg += c;
        break;

      default:
        break;
    }
  }

  if(!curArg.empty())
    argStrings.push_back(std::move(curArg));

  argCount = static_cast<uInt32>(argStrings.size());

  args.reserve(argCount);
  for(const auto& argStr: argStrings)
  {
    if(!YaccParser::parse(argStr))
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
  const auto& command = commands[cmd];
  const auto& parms = command.parms;

  if(argCount == 0)
  {
    if(command.parmsRequired)
    {
      outputCommandError("missing required argument(s)", cmd);
      return false;
    }
    return true;
  }

  // Count fixed parameters up to ARG_END_ARGS or ARG_MULTI_BYTE
  uInt32 fixedCount = 0;
  const auto* p = parms.data();
  while(*p != Parameters::ARG_END_ARGS && *p != Parameters::ARG_MULTI_BYTE)
  {
    ++fixedCount;
    ++p;
  }

  // Evil hack: some commands intentionally take multiple arguments
  // In this case, the required number of arguments is unbounded
  // Only ARG_MULTI_BYTE triggers unbounded mode
  const bool isMulti = (*p == Parameters::ARG_MULTI_BYTE);
  const uInt32 argRequiredCount = isMulti ? argCount : fixedCount;

  p = parms.data();
  uInt32 curCount = 0;

  do {
    if(curCount >= argCount)
      break;

    const uInt32  curArgInt = args[curCount];
    const string& curArgStr = argStrings[curCount];

    switch(*p)
    {
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
          commandResult.str(red(
            R"(invalid base (must be #2, #10, #16, "bin", "dec", or "hex"))"));
          return false;
        }
        break;

      case Parameters::ARG_LABEL:
      case Parameters::ARG_FILE:
        [[fallthrough]]; // FIXME: validate these (for now any string's allowed)

      case Parameters::ARG_MULTI_BYTE:
      case Parameters::ARG_MULTI_WORD:
        [[fallthrough]]; // FIXME: validate these (for now, any number's allowed)

      case Parameters::ARG_END_ARGS:
        [[fallthrough]];

      default:
        break;  // Not supposed to get here
    }
    ++curCount;
    ++p;
  } while(*p != Parameters::ARG_END_ARGS && curCount < argRequiredCount);

  if(curCount < argRequiredCount)
  {
    outputCommandError("missing required argument(s)", cmd);
    return false;
  }
  if(argCount > curCount)
  {
    outputCommandError("too many arguments", cmd);
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::eval()
{
  string buf;
  buf.reserve(static_cast<size_t>(argCount) * 64);  // rough estimate per arg line

  for(uInt32 i = 0; i < argCount; ++i)
  {
    const int arg = args[i];

    if(arg < 0x10000)
    {
      const string rlabel = debugger.cartDebug().getLabel(arg, true);
      const string wlabel = debugger.cartDebug().getLabel(arg, false);
      const bool validR = !rlabel.empty() && rlabel[0] != '$';
      const bool validW = !wlabel.empty() && wlabel[0] != '$';

      if(validR && validW)
      {
        if(rlabel == wlabel)
          std::format_to(std::back_inserter(buf), "{}(R/W): ", rlabel);
        else
          std::format_to(std::back_inserter(buf), "{}(R) / {}(W): ", rlabel, wlabel);
      }
      else if(validR)
        std::format_to(std::back_inserter(buf), "{}(R): ", rlabel);
      else if(validW)
        std::format_to(std::back_inserter(buf), "{}(W): ", wlabel);
    }

    std::format_to(std::back_inserter(buf), "${}", Base::toString(arg, Base::Fmt::_16));

    if(arg < 0x10000)
      std::format_to(std::back_inserter(buf), " %{}", Base::toString(arg, Base::Fmt::_2));

    std::format_to(std::back_inserter(buf), " #{}", static_cast<int>(arg));

    if(i != argCount - 1)
      buf += '\n';
  }
  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& DebuggerParser::cartName() const
{
  return debugger.myOSystem.console().properties().get(PropType::Cart_Name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::printTimer(uInt32 idx, bool showHeader)
{
  if(idx >= debugger.m6502().numTimers())
  {
    commandResult << red("invalid timer");
    return;
  }

  const auto& timer = debugger.m6502().getTimer(idx);
  const bool banked = debugger.cartDebug().romBankCount() > 1;
  const int colWidth = banked ? 12 : 15;

  // Helper to build a fixed-width label string for an address
  const auto makeLabel = [&](uInt16 addr) -> string
  {
    std::ostringstream buf;
    if(!debugger.cartDebug().getLabel(buf, addr, true))
      buf << "    $" << setw(4) << Base::HEX4 << addr;

    string label{buf.view()};
    const int trimWidth = colWidth - (timer.mirrors ? 1 : 0);
    if(std::cmp_greater(label.size(), trimWidth))
      label.resize(trimWidth);
    label += timer.mirrors ? "+" : "";
    label.resize(colWidth, ' ');  // pad to fixed width
    return label;
  };

  const auto labelFrom = makeLabel(timer.from.addr);
  const auto labelTo   = makeLabel(timer.to.addr);

  if(showHeader)
    commandResult << (banked
      ? " #|    From    /Bk|     To     /Bk| Execs| Avg. | Min. | Max. |"
      : " #|     From      |      To       | Execs| Avg. | Min. | Max. |");

  commandResult << '\n' << Base::toString(idx) << '|' << labelFrom;

  if(banked)
  {
    if(timer.anyBank)
      commandResult << "/ *";
    else
      std::format_to(std::ostreambuf_iterator(commandResult),
                     "/{:2}", static_cast<uInt16>(timer.from.bank));
  }

  commandResult << '|';

  if(timer.isPartial)
  {
    commandResult << (banked ? "       -       " : "      -        ")
                  << "|     -|     -|     -|     -|";
  }
  else
  {
    commandResult << labelTo;
    if(banked)
    {
      if(timer.anyBank)
        commandResult << "/ *";
      else
        std::format_to(std::ostreambuf_iterator(commandResult),
                       "/{:2}", static_cast<uInt16>(timer.to.bank));
    }

    std::format_to(std::ostreambuf_iterator(commandResult),
                   "|{:6}|", timer.execs);

    if(!timer.execs)
      commandResult << "     -|     -|     -|";
    else
      std::format_to(std::ostreambuf_iterator(commandResult),
                     "{:6}|{:6}|{:6}|",
                     timer.averageCycles(), timer.minCycles, timer.maxCycles);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::getTimerCmds()
{
  const auto numTimers = debugger.m6502().numTimers();
  if(numTimers == 0)
    return {};

  const bool banked = debugger.cartDebug().romBankCount() > 1;
  string out;
  out.reserve(static_cast<size_t>(numTimers) * 32);

  // Helper to build an address label with optional mirror/bank suffix
  std::ostringstream buf;
  const auto makeAddrStr = [&](uInt16 addr, uInt8 bank, bool mirrors, bool anyBank) -> string
  {
    buf.str("");
    if(!debugger.cartDebug().getLabel(buf, addr, true))
      buf << Base::HEX4 << addr;
    if(mirrors)
      buf << '+';
    if(banked)
    {
      if(anyBank)
        buf << '*';
      else
        std::format_to(std::ostreambuf_iterator(buf),
                       "{}", static_cast<uInt16>(bank));
    }
    return string{buf.view()};
  };

  for(uInt32 idx = 0; idx < numTimers; ++idx)
  {
    const auto& timer = debugger.m6502().getTimer(idx);

    out += "timer ";
    out += makeAddrStr(timer.from.addr, timer.from.bank,
                       timer.mirrors, timer.anyBank);

    if(!timer.isPartial)
    {
      out += ' ';
      out += makeAddrStr(timer.to.addr, timer.to.bank,
                         timer.mirrors, timer.anyBank);
    }
    out += '\n';
  }
  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::listTimers()
{
  const auto numTimers = debugger.m6502().numTimers();
  if(numTimers == 0)
  {
    commandResult << "no timers set";
    return;
  }

  commandResult << "timers:\n";
  for(uInt32 i = 0; i < numTimers; ++i)
    printTimer(i, i == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::listTraps(bool listCond)
{
  const auto& names = debugger.m6502().getCondTrapNames();
  const auto numNames = static_cast<uInt32>(names.size());

  commandResult << (listCond ? "trapifs:" : "traps:") << '\n';

  bool firstLine = true;
  for(uInt32 i = 0; i < numNames; ++i)
  {
    const bool hasCond = !names[i].empty();
    if(hasCond != listCond)
      continue;

    const auto& trap = *myTraps[i];

    if(!firstLine)
      commandResult << '\n';
    firstLine = false;

    commandResult << Base::toString(i) << ": ";

    if(trap.read && trap.write)
      commandResult << "read|write";
    else if(trap.read)
      commandResult << "read      ";
    else if(trap.write)
      commandResult << "     write";
    else
      commandResult << "none";

    if(hasCond)
      commandResult << ' ' << names[i];

    commandResult << ' ' << debugger.cartDebug().getLabel(trap.begin, true, 4);

    if(trap.begin != trap.end)
      commandResult << ' ' << debugger.cartDebug().getLabel(trap.end, true, 4);

    commandResult << trapStatus(trap) << " + mirrors";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::trapStatus(const Trap& trap)
{
  const auto lblb = debugger.cartDebug().getLabel(trap.begin, !trap.write);
  const auto lble = (trap.begin != trap.end)
    ? debugger.cartDebug().getLabel(trap.end, !trap.write)
    : string{};

  if(lblb.empty() && lble.empty())
    return {};

  string result;
  result.reserve(lblb.size() + lble.size() + 4);

  result += " (";
  if(!lblb.empty())
    result += lblb;
  if(!lble.empty())
  {
    if(!lblb.empty())
      result += ' ';
    result += lble;
  }
  result += ')';
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DebuggerParser::saveScriptFile(string file)
{
  // Append 'script' extension when necessary
  if(file.find_last_of('.') == string::npos)
    file += ".script";

  // Use user dir if no path is provided
  if(file.find_first_of(FSNode::PATH_SEPARATOR) == string::npos)
    file = debugger.myOSystem.userDir().getPath() + file;

  const FSNode node(file);

  string out;
  out.reserve(4096);

  const auto& funcs = debugger.getFunctionDefMap();
  for(const auto& [name, cmd]: funcs)
  {
    if(!Debugger::isBuiltinFunction(name))
    {
      out += "function ";
      out += name;
      out += " {";
      out += cmd;
      out += "}\n";
    }
  }

  for(const auto& w: myWatches)
  {
    out += "watch ";
    out += w;
    out += '\n';
  }

  for(const auto& bp: debugger.breakPoints().getBreakpoints())
  {
    out += "break ";
    out += Base::toString(bp.addr);
    out += ' ';
    out += Base::toString(bp.bank);
    out += '\n';
  }

  const auto& breakConds = debugger.m6502().getCondBreakNames();
  for(const auto& cond: breakConds)
  {
    out += "breakIf {";
    out += cond;
    out += "}\n";
  }

  const auto& saveConds = debugger.m6502().getCondSaveStateNames();
  for(const auto& cond: saveConds)
  {
    out += "saveStateIf {";
    out += cond;
    out += "}\n";
  }

  const auto& names = debugger.m6502().getCondTrapNames();
  for(uInt32 i = 0; i < myTraps.size(); ++i)
  {
    const auto& trap = *myTraps[i];
    const bool hasCond = !names[i].empty();

    if(trap.read && trap.write) out += "trap";
    else if(trap.read)          out += "trapRead";
    else if(trap.write)         out += "trapWrite";

    if(hasCond)
    {
      out += "if {";
      out += names[i];
      out += '}';
    }

    out += ' ';
    out += Base::toString(trap.begin);
    if(trap.begin != trap.end)
    {
      out += ' ';
      out += Base::toString(trap.end);
    }
    out += '\n';
  }

  out += getTimerCmds();

  if(!node.exists() && out.empty())
    return "nothing to save";

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
void DebuggerParser::saveDump(const FSNode& node, const std::ostringstream& out,
                              std::ostringstream& result)
{
  try
  {
    node.write(out.view());  // FIXME: can we eliminate the stream?
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
  if(args[1] < args[0])
  {
    commandResult << red("start address must be <= end address");
    return;
  }

  const bool result = debugger.cartDebug().addDirective(type, args[0], args[1]);

  std::format_to(std::ostreambuf_iterator(commandResult),
                 "{}{} directive on range ${:x} ${:x}",
                 result ? "added " : "removed ",
                 CartDebug::AccessTypeAsString(type),
                 args[0], args[1]);

  debugger.rom().invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// executor methods for commands[] array. All are void, no args.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "a"
void DebuggerParser::executeA()
{
  debugger.cpuDebug().setA(static_cast<uInt8>(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "aud"
void DebuggerParser::executeAud()
{
  executeDirective(Device::AUD);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "autoSave"
void DebuggerParser::executeAutoSave()
{
  const bool enable = !settings.getBool("dbg.autosave");

  settings.setValue("dbg.autosave", enable);
  commandResult << "autoSave " << (enable ? "enabled" : "disabled");
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
  switch(Base::format())
  {
    case Base::Fmt::_2:   commandResult << "#2/bin";        break;
    case Base::Fmt::_10:  commandResult << "#10/dec";       break;
    case Base::Fmt::_16:  commandResult << "#16/hex";       break;
    default:              commandResult << red("UNKNOWN");  break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "bCol"
void DebuggerParser::executeBCol()
{
  executeDirective(Device::BCOL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "break"
void DebuggerParser::executeBreak()
{
  const uInt32 romBankCount = debugger.cartDebug().romBankCount();
  const uInt16 addr = (argCount == 0) ? debugger.cpuDebug().pc() : args[0];
  uInt8 bank = 0;

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

  // Helper to format a single breakpoint result line
  const auto formatBreak = [&](int b)
  {
    const bool set = debugger.toggleBreakPoint(addr, b);
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "{} breakpoint at ${:04x} + mirrors",
                   set ? "set" : "cleared", addr);
    if(romBankCount > 1)
      std::format_to(std::ostreambuf_iterator(commandResult),
                     " in bank #{}", b);
  };

  if(bank != 0xff)
  {
    formatBreak(bank);
  }
  else
  {
    for(int i = 0; std::cmp_less(i, romBankCount); ++i)
    {
      if(i) commandResult << '\n';
      formatBreak(i);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breakIf"
void DebuggerParser::executeBreakIf()
{
  if(YaccParser::parse(argStrings[0]) != 0)
  {
    commandResult << red("invalid expression");
    return;
  }

  const string_view condition = argStrings[0];
  const auto& condNames = debugger.m6502().getCondBreakNames();

  const auto it = std::ranges::find(condNames, condition);
  if(it != condNames.end())
  {
    args[0] = static_cast<int>(it - condNames.begin());
    executeDelBreakIf();
    return;
  }

  const uInt32 ret = debugger.m6502().addCondBreak(
                       YaccParser::getResult(), argStrings[0]);
  commandResult << "added breakIf " << Base::toString(ret);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "breakLabel"
void DebuggerParser::executeBreakLabel()
{
  const uInt16 addr = (argCount == 0) ? debugger.cpuDebug().pc() : args[0];
  const bool set = debugger.toggleBreakPoint(addr, BreakpointMap::ANY_BANK);

  commandResult << (set ? "set" : "cleared");
  commandResult << " breakpoint at $" << Base::HEX4 << addr << " (no mirrors)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "c"
void DebuggerParser::executeC()
{
  if(argCount == 0)       debugger.cpuDebug().toggleC();
  else if(argCount == 1)  debugger.cpuDebug().setC(args[0]);
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

  for(const auto& cheat: argStrings)
  {
    if(debugger.myOSystem.cheat().add("DBG", cheat))
      commandResult << "cheat code " << cheat << " enabled\n";
    else
      commandResult << red("invalid cheat code ") << cheat << '\n';
  }
#else
  commandResult << red("Cheat support not enabled\n");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearBreaks"
void DebuggerParser::executeClearBreaks()
{
  debugger.clearAllBreakPoints();
  debugger.m6502().clearCondBreaks();
  commandResult << "all breakpoints cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearConfig"
void DebuggerParser::executeClearConfig()
{
  if(argCount == 1)  commandResult << debugger.cartDebug().clearConfig(args[0]);
  else               commandResult << debugger.cartDebug().clearConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearHistory"
void DebuggerParser::executeClearHistory()
{
  debugger.prompt().clearHistory();
  commandResult.str("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearBreaks"
void DebuggerParser::executeClearSaveStateIfs()
{
  debugger.m6502().clearCondSaveStates();
  commandResult << "all saveState points cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearTimers"
void DebuggerParser::executeClearTimers()
{
  debugger.m6502().clearTimers();
  commandResult << "all timers cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearTraps"
void DebuggerParser::executeClearTraps()
{
  debugger.clearAllTraps();
  debugger.m6502().clearCondTraps();
  myTraps.clear();
  commandResult << "all traps cleared";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "clearWatches"
void DebuggerParser::executeClearWatches()
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
// "colorTest"
void DebuggerParser::executeColorTest()
{
  commandResult << "test color: "
                << static_cast<char>((args[0]>>1) | 0x80)
                << inverse("        ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "d"
void DebuggerParser::executeD()
{
  if(argCount == 0)       debugger.cpuDebug().toggleD();
  else if(argCount == 1)  debugger.cpuDebug().setD(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "data"
void DebuggerParser::executeData()
{
  executeDirective(Device::DATA);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "debugColors"
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
// "delBreakIf"
void DebuggerParser::executeDelBreakIf()
{
  if(debugger.m6502().delCondBreak(args[0]))
    commandResult << "removed breakIf " << Base::toString(args[0]);
  else
    commandResult << red("no such breakIf");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delFunction"
void DebuggerParser::executeDelFunction()
{
  if(debugger.delFunction(argStrings[0]))
    commandResult << "removed function " << argStrings[0];
  else
    commandResult << "function " << argStrings[0] << " built-in or not found";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delSaveStateIf"
void DebuggerParser::executeDelSaveStateIf()
{
  if(debugger.m6502().delCondSaveState(args[0]))
    commandResult << "removed saveStateIf " << Base::toString(args[0]);
  else
    commandResult << red("no such saveStateIf");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delTimer"
void DebuggerParser::executeDelTimer()
{
  const int index = args[0];
  if(debugger.m6502().delTimer(index))
    commandResult << "removed timer " << Base::toString(index);
  else
    commandResult << red("no such timer");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delTrap"
void DebuggerParser::executeDelTrap()
{
  const int index = args[0];

  if(!debugger.m6502().delCondTrap(index))
  {
    commandResult << red("no such trap");
    return;
  }

  const auto& trap = *myTraps[index];
  for(uInt32 addr = trap.begin; addr <= trap.end; ++addr)
    executeTrapRW(addr, trap.read, trap.write, false);

  // @sa666666: please check this:
  Vec::removeAt(myTraps, index);
  commandResult << "removed trap " << Base::toString(index);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "delWatch"
void DebuggerParser::executeDelWatch()
{
  const int which = args[0] - 1;
  if(which < 0 || std::cmp_greater_equal(which, myWatches.size()))
  {
    commandResult << red("no such watch");
    return;
  }

  Vec::removeAt(myWatches, which);
  commandResult << "removed watch";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "disAsm"
void DebuggerParser::executeDisAsm()
{
  if(argCount > 2)
  {
    outputCommandError("wrong number of arguments", myCommand);
    return;
  }

  const auto start = (argCount == 0) ? debugger.cpuDebug().pc() : args[0];
  const auto lines = (argCount == 2) ? args[1] : 20;

  commandResult << debugger.cartDebug().disassembleLines(start, lines);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "dump"
void DebuggerParser::executeDump()
{
  const auto dump = [&](std::ostream& os, int start, int end)
  {
    for(int i = start; i <= end; i += 16)
    {
      os << Base::toString(i) << ": ";
      for(int j = i; j < i + 16 && j <= end; ++j)
      {
        os << Base::toString(debugger.peek(j)) << ' ';
        if(j == i + 7 && j != end) os << "- ";
      }
      os << '\n';
    }
  };

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
  {
    dump(commandResult, args[0], args[0] + 127);
    return;
  }
  if(argCount == 2 || args[2] == 0)
  {
    dump(commandResult, args[0], args[1]);
    return;
  }

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

  string path = debugger.myOSystem.userDir().getPath() + cartName() + "_dbg_";
  if(execDepth > 0)
    path += execPrefix;
  else
    std::format_to(std::back_inserter(path), "{:08x}",
                   static_cast<uInt32>(TimerManager::getTicks() / 1000));
  path += ".dump";

  commandResult << "dumped ";

  std::ostringstream out;
  if((args[2] & 0x01) != 0)
  {
    dump(out, args[0], args[1]);
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "bytes from ${:x} to ${:x}", args[0], args[1]);
    if((args[2] & 0x06) != 0)
      commandResult << ", ";
  }
  if((args[2] & 0x02) != 0)
  {
    const CpuDebug& cpu = debugger.cpuDebug();
    out << "   <PC>PC SP  A  X  Y  -  -    N  V  B  D  I  Z  C  -\n"
           "XC: "
        << Base::toString(cpu.pc() & 0xff) << ' ' // PC lsb
        << Base::toString(cpu.pc() >> 8)   << ' ' // PC msb
        << Base::toString(cpu.sp()) << ' '        // SP
        << Base::toString(cpu.a())  << ' '        // A
        << Base::toString(cpu.x())  << ' '        // X
        << Base::toString(cpu.y())  << ' '        // Y
        << Base::toString(0) << ' '               // unused
        << Base::toString(0) << " - "             // unused
        << Base::toString(cpu.n()) << ' '         // N (flag)
        << Base::toString(cpu.v()) << ' '         // V (flag)
        << Base::toString(cpu.b()) << ' '         // B (flag)
        << Base::toString(cpu.d()) << ' '         // D (flag)
        << Base::toString(cpu.i()) << ' '         // I (flag)
        << Base::toString(cpu.z()) << ' '         // Z (flag)
        << Base::toString(cpu.c()) << ' '         // C (flag)
        << Base::toString(0) << '\n';             // unused
    commandResult << "CPU state";
    if((args[2] & 0x04) != 0)
      commandResult << ", ";
  }
  if((args[2] & 0x04) != 0)
  {
    out << "   SWA - SWB  - IT  -  -  -   I0 I1 I2 I3 I4 I5 -  -\n"
           "XS: "
        << Base::toString(debugger.peek(0x280)) << ' '  // SWCHA
        << Base::toString(0) << ' '                     // unused
        << Base::toString(debugger.peek(0x282)) << ' '  // SWCHB
        << Base::toString(0) << ' '                     // unused
        << Base::toString(debugger.peek(0x284)) << ' '  // INTIM
        << Base::toString(0) << ' '                     // unused
        << Base::toString(0) << ' '                     // unused
        << Base::toString(0) << " - "                   // unused
        << Base::toString(debugger.peek(TIARegister::INPT0)) << ' '
        << Base::toString(debugger.peek(TIARegister::INPT1)) << ' '
        << Base::toString(debugger.peek(TIARegister::INPT2)) << ' '
        << Base::toString(debugger.peek(TIARegister::INPT3)) << ' '
        << Base::toString(debugger.peek(TIARegister::INPT4)) << ' '
        << Base::toString(debugger.peek(TIARegister::INPT5)) << ' '
        << Base::toString(0) << ' '                     // unused
        << Base::toString(0) << '\n';                   // unused
    commandResult << "switches and fire buttons";
  }

  if(argCount == 4)
  {
    string outStr{out.view()};
    string resultStr{commandResult.view()};

    DebuggerDialog* dlg = debugger.myDialog;
    BrowserDialog::show(dlg, "Save Dump as", path,
                        BrowserDialog::Mode::FileSave,
                        [dlg, outStr = std::move(outStr),
                              resultStr = std::move(resultStr)]
                        (bool OK, const FSNode& node) mutable
    {
      if(OK)
      {
        const std::ostringstream localOut(std::move(outStr));
        std::ostringstream localResult(std::move(resultStr), std::ios_base::app);
        saveDump(node, localOut, localResult);
        dlg->prompt().print(localResult.str() + '\n');
      }
      dlg->prompt().printPrompt();
    });
    commandResult.str("_NO_PROMPT");
  }
  else
    saveDump(FSNode(path), out, commandResult);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "exec"
void DebuggerParser::executeExec()
{
  // Append 'script' extension when necessary
  string file = argStrings[0];
  if(file.find_last_of('.') == string::npos)
    file += ".script";

  FSNode node(file);
  if(!node.exists())
    node = FSNode(debugger.myOSystem.userDir().getPath() + file);

  if(argCount == 2)
    execPrefix = argStrings[1];
  else
    execPrefix = std::format("{:08x}",
                             static_cast<uInt32>(TimerManager::getTicks() / 1000));

  StringList history;
  commandResult << exec(node, &history);

  for(const auto& item: history)
    debugger.prompt().addToHistory(item.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "exitRom"
void DebuggerParser::executeExitRom()
{
  debugger.exit(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "frame"
void DebuggerParser::executeFrame()
{
  int count = 1;
  if(argCount != 0)
    count = args[0];
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

  if(YaccParser::parse(argStrings[1]) != 0)
  {
    commandResult << red("invalid expression");
    return;
  }

  debugger.addFunction(argStrings[0], argStrings[1], YaccParser::getResult());
  commandResult << "added function " << argStrings[0] << " -> " << argStrings[1];
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
    size_t clen = 0;
    for(const auto& c: commands)
      clen = std::max(clen, c.cmdString.length());

    commandResult << setfill(' ');
    for(const auto& c: commands)
      commandResult << setw(static_cast<int>(clen)) << right << c.cmdString
                    << " - " << c.description << '\n';

    commandResult << Debugger::builtinHelp();
  }
  else  // get help for specific command
  {
    for(auto& c: commands)
    {
      if(BSPF::toLowerCase(argStrings[0]) == BSPF::toLowerCase(c.cmdString))
      {
        commandResult << "  " << red(c.description) << '\n' << c.extendedDesc;
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0Up"
void DebuggerParser::executeJoy0Up()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::One);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0Down"
void DebuggerParser::executeJoy0Down()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Two);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0Left"
void DebuggerParser::executeJoy0Left()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Three);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0Right"
void DebuggerParser::executeJoy0Right()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Four);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy0Fire"
void DebuggerParser::executeJoy0Fire()
{
  ControllerLowLevel lport(debugger.myOSystem.console().leftController());
  if(argCount == 0)
    lport.togglePin(Controller::DigitalPin::Six);
  else if(argCount == 1)
    lport.setPin(Controller::DigitalPin::Six, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1Up"
void DebuggerParser::executeJoy1Up()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::One);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::One, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1Down"
void DebuggerParser::executeJoy1Down()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Two);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Two, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1Left"
void DebuggerParser::executeJoy1Left()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Three);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Three, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1Right"
void DebuggerParser::executeJoy1Right()
{
  ControllerLowLevel rport(debugger.myOSystem.console().rightController());
  if(argCount == 0)
    rport.togglePin(Controller::DigitalPin::Four);
  else if(argCount == 1)
    rport.setPin(Controller::DigitalPin::Four, args[0] != 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "joy1Fire"
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
  int address = args[0];
  int line = -1;

  // The specific address we want may not exist (it may be part of a data section)
  // If so, scroll backward a little until we find it
  while(address >= 0 && (line = debugger.cartDebug().addressToLine(address)) == -1)
    --address;

  if(line >= 0 && address >= 0)
  {
    debugger.rom().scrollTo(line);
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "disassembly scrolled to address ${:04x}", address);
  }
  else
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "address ${:04x} doesn't exist", args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listBreaks"
void DebuggerParser::executeListBreaks()
{
  const uInt32 romBankCount = debugger.cartDebug().romBankCount();
  const auto& breakpoints = debugger.breakPoints().getBreakpoints();
  const auto& conds = debugger.m6502().getCondBreakNames();

  if(breakpoints.empty() && conds.empty())
  {
    commandResult << "no breakpoints set";
    return;
  }

  int count = 0;
  if(!breakpoints.empty())
  {
    string buf;
    buf.reserve(breakpoints.size() * 8);

    for(const auto& bp: breakpoints)
    {
      if(romBankCount == 1)
      {
        buf += debugger.cartDebug().getLabel(bp.addr, true, 4);
        buf += ' ';
        if(!(++count % 8)) buf += '\n';
      }
      else
      {
        if(count % 6)
          buf += ", ";
        buf += debugger.cartDebug().getLabel(bp.addr, true, 4);
        if(bp.bank != 255)
          std::format_to(std::back_inserter(buf), " #{}", static_cast<int>(bp.bank));
        else
          buf += " *";
        if(!(++count % 6)) buf += '\n';
      }
    }

    commandResult << "breaks:\n" << buf;
  }

  if(!conds.empty())
  {
    if(count)
      commandResult << '\n';
    commandResult << "BreakIfs:\n";
    for(uInt32 i = 0; i < conds.size(); ++i)
    {
      commandResult << Base::toString(i) << ": " << conds[i];
      if(i != conds.size() - 1) commandResult << '\n';
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listConfig"
void DebuggerParser::executeListConfig()
{
  if(argCount == 1)  commandResult << debugger.cartDebug().listConfig(args[0]);
  else               commandResult << debugger.cartDebug().listConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listFunctions"
void DebuggerParser::executeListFunctions()
{
  const Debugger::FunctionDefMap& functions = debugger.getFunctionDefMap();

  if(!functions.empty())
    for(const auto& [name, cmd]: functions)
      commandResult << name << " -> " << cmd << '\n';
  else
    commandResult << "no user-defined functions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listSaveStateIfs"
void DebuggerParser::executeListSaveStateIfs()
{
  const auto& conds = debugger.m6502().getCondSaveStateNames();
  if(conds.empty())
  {
    commandResult << "no savestateifs defined";
    return;
  }

  commandResult << "saveStateIf:\n";
  for(uInt32 i = 0; i < conds.size(); ++i)
  {
    commandResult << Base::toString(i) << ": " << conds[i];
    if(i != conds.size() - 1) commandResult << '\n';
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listTimers"
void DebuggerParser::executeListTimers()
{
  if(debugger.m6502().numTimers())  listTimers();
  else                              commandResult << "no timers set";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "listTraps"
void DebuggerParser::executeListTraps()
{
  const auto& names = debugger.m6502().getCondTrapNames();
  if(myTraps.size() != names.size())
  {
    commandResult << "Internal error! Different trap sizes.";
    return;
  }
  if(names.empty())
  {
    commandResult << "no traps set";
    return;
  }

  const bool trapFound   = std::ranges::any_of(names, &string::empty);
  const bool trapifFound = std::ranges::any_of(names, std::not_fn(&string::empty));

  if(trapFound)   listTraps(false);
  if(trapifFound) listTraps(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "loadAllStates"
void DebuggerParser::executeLoadAllStates()
{
  debugger.loadAllStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "loadConfig"
void DebuggerParser::executeLoadConfig()
{
  commandResult << debugger.cartDebug().loadConfigFile();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "loadState"
void DebuggerParser::executeLoadState()
{
  if(args[0] >= 0 && args[0] <= 9)
    debugger.loadState(args[0]);
  else
    commandResult << red("invalid slot (must be 0-9)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::executeLogBreaks()
{
  const bool enable = !debugger.mySystem.m6502().getLogBreaks();

  debugger.mySystem.m6502().setLogBreaks(enable);
  settings.setValue("dbg.logbreaks", enable);
  commandResult << "logBreaks " << (enable ? "enabled" : "disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::executeLogExec()
{
  const bool enable = !settings.getBool("dbg.logexec");

  settings.setValue("dbg.logexec", enable);
  commandResult << "logExec " << (enable ? "enabled" : "disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerParser::executeLogTrace()
{
  const bool enable = !debugger.mySystem.m6502().getLogTrace();

  debugger.mySystem.m6502().setLogTrace(enable);
  settings.setValue("dbg.logtrace", enable);
  commandResult << "logTrace " << (enable ? "enabled" : "disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "n"
void DebuggerParser::executeN()
{
  if(argCount == 0)       debugger.cpuDebug().toggleN();
  else if(argCount == 1)  debugger.cpuDebug().setN(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "palette"
void DebuggerParser::executePalette()
{
  commandResult << TIADebug::palette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pc"
void DebuggerParser::executePc()
{
  debugger.cpuDebug().setPC(args[0]);
  debugger.addState(std::format("Set PC @ {:04x}", args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pCol"
void DebuggerParser::executePCol()
{
  executeDirective(Device::PCOL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "pGfx"
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
// "printTimer"
void DebuggerParser::executePrintTimer()
{
  printTimer(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "ram"
void DebuggerParser::executeRam()
{
  if(argCount == 0)  commandResult << debugger.cartDebug().toString();
  else               commandResult << debugger.setRAM(args);
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
// "resetTimers"
void DebuggerParser::executeResetTimers()
{
  debugger.m6502().resetTimers();
  commandResult << "all timers reset";
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
    if(!debugger.patchROM(addr++, args[i]))
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

  std::format_to(std::ostreambuf_iterator(commandResult),
                 "changed {} location(s)", args.size() - 1);
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
  debugger.exit(false);
  commandResult << "_EXIT_DEBUGGER";  // See PromptWidget for more info
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "runTo"
void DebuggerParser::executeRunTo()
{
  const auto& cartdbg = debugger.cartDebug();
  const auto& list = cartdbg.disassembly().list;
  const auto max_iterations = list.size();
  const string_view target = argStrings[0];

  debugger.saveOldState();

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());
  progress.setMessage(std::format(
    "runTo searching through {} disassembled instructions{}",
    max_iterations, progress.ELLIPSIS));
  progress.setRange(0, static_cast<int>(max_iterations), 5);
  progress.open();

  size_t count = 0;
  bool done = false;
  do {
    debugger.step(false);

    // Update romlist to point to current PC
    const int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    if(pcline >= 0)
      done = (BSPF::findIgnoreCase(list[pcline].disasm, target) != string::npos);

    // Update the progress bar
    progress.incProgress();
  } while(!done && ++count < max_iterations && !progress.isCancelled());

  progress.close();

  if(done)
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "found {} in {} disassembled instructions", target, count);
  else
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "{} not found in {} disassembled instructions", target, count);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "runToPc"
void DebuggerParser::executeRunToPc()
{
  const auto& cartdbg = debugger.cartDebug();
  const auto& list = cartdbg.disassembly().list;

  debugger.saveOldState();

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());
  progress.setMessage(std::format("        runTo PC running{}        ",
                                  progress.ELLIPSIS));
  progress.setRange(0, 100000, 5);
  progress.open();

  uInt32 count = 0;
  bool done = false;
  do {
    debugger.step(false);

    // Update romlist to point to current PC
    const int pcline = cartdbg.addressToLine(debugger.cpuDebug().pc());
    done = (pcline >= 0) && std::cmp_equal(list[pcline].address, args[0]);
    progress.incProgress();
    ++count;
  } while(!done && !progress.isCancelled());

  progress.close();

  if(done)
  {
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "Set PC to ${:04x} in {} instructions", args[0], count);
    debugger.addState(std::format("RunTo PC @ {:04x}", args[0]));
  }
  else
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "PC ${:04x} not reached or found in {} instructions",
                   args[0], count);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "s"
void DebuggerParser::executeS()
{
  debugger.cpuDebug().setSP(static_cast<uInt8>(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "save"
void DebuggerParser::executeSave()
{
  auto* dlg = debugger.myDialog;
  const string fileName = dlg->instance().userDir().getPath() + cartName() + ".script";

  if(argCount && argStrings[0] == "?")
  {
    BrowserDialog::show(dlg, "Save Workbench as", fileName,
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FSNode& node)
    {
      if(OK)
        dlg->prompt().print(saveScriptFile(node.getPath()) + '\n');
      dlg->prompt().printPrompt();
    });

    // avoid printing a new prompt
    commandResult.str("_NO_PROMPT");
  }
  else
    commandResult << saveScriptFile(argCount ? argStrings[0] : fileName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveAccess"
void DebuggerParser::executeSaveAccess()
{
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Access Counters as",
                        dlg->instance().userDir().getPath() + cartName() + ".csv",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FSNode& node)
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
// "saveConfig"
void DebuggerParser::executeSaveConfig()
{
  commandResult << debugger.cartDebug().saveConfigFile();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveDis"
void DebuggerParser::executeSaveDisassembly()
{
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save Disassembly as",
                        dlg->instance().userDir().getPath() + cartName() + ".asm",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FSNode& node)
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
// "saveRom"
void DebuggerParser::executeSaveRom()
{
  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;

    BrowserDialog::show(dlg, "Save ROM as",
                        dlg->instance().userDir().getPath() + cartName() + ".a26",
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FSNode& node)
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
// "saveSes"
void DebuggerParser::executeSaveSes()
{
  const auto timeinfo = BSPF::localTime();
  char timebuf[32];
  std::ignore = std::strftime(timebuf, sizeof(timebuf),
                              "session_%F_%H-%M-%S.txt", &timeinfo);
  const string filename{timebuf};

  if(argCount && argStrings[0] == "?")
  {
    DebuggerDialog* dlg = debugger.myDialog;
    BrowserDialog::show(dlg, "Save Session as",
                        dlg->instance().userDir().getPath() + filename,
                        BrowserDialog::Mode::FileSave,
                        [this, dlg](bool OK, const FSNode& node)
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
    const string path = argCount
      ? argStrings[0]
      : debugger.myOSystem.userDir().getPath() + filename;
    commandResult << debugger.prompt().saveBuffer(FSNode(path));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveSnap"
void DebuggerParser::executeSaveSnap()
{
  debugger.tiaOutput().saveSnapshot(execDepth, execPrefix, argCount == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveAllStates"
void DebuggerParser::executeSaveAllStates()
{
  debugger.saveAllStates();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveState"
void DebuggerParser::executeSaveState()
{
  if(args[0] >= 0 && args[0] <= 9)
    debugger.saveState(args[0]);
  else
    commandResult << red("invalid slot (must be 0-9)");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "saveStateIf"
void DebuggerParser::executeSaveStateIf()
{
  if(YaccParser::parse(argStrings[0]) != 0)
  {
    commandResult << red("invalid expression");
    return;
  }

  const string_view condition = argStrings[0];
  const auto& condNames = debugger.m6502().getCondSaveStateNames();

  const auto it = std::ranges::find(condNames, condition);
  if(it != condNames.end())
  {
    args[0] = static_cast<int>(it - condNames.begin());
    executeDelSaveStateIf();
    return;
  }

  const uInt32 ret = debugger.m6502().addCondSaveState(
    YaccParser::getResult(), argStrings[0]);
  commandResult << "added saveStateIf " << Base::toString(ret);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "scanLine"
void DebuggerParser::executeScanLine()
{
  int count = 1;
  if(argCount != 0)
    count = args[0];
  debugger.nextScanline(count);
  commandResult << "advanced " << dec << count << " scanLine(s)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "step"
void DebuggerParser::executeStep()
{
  commandResult << "executed " << dec << debugger.step() << " cycles";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "stepWhile"
void DebuggerParser::executeStepWhile()
{
  if(YaccParser::parse(argStrings[0]) != 0)
  {
    commandResult << red("invalid expression");
    return;
  }

  const Expression* expr = YaccParser::getResult();
  int ncycles = 0;

  // Create a progress dialog box to show the progress searching through the
  // disassembly, since this may be a time-consuming operation
  ProgressDialog progress(debugger.baseDialog(), debugger.lfont());
  progress.setMessage(std::format(
    "stepWhile running through disassembled instructions{}",
    progress.ELLIPSIS));
  progress.setRange(0, 100000, 5);
  progress.open();

  do {
    ncycles += debugger.step(false);
    progress.incProgress();
  } while(expr->evaluate() && !progress.isCancelled());

  progress.close();
  std::format_to(std::ostreambuf_iterator(commandResult),
                 "executed {} cycles", ncycles);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "swchb"
void DebuggerParser::executeSwchb()
{
  debugger.riotDebug().switches(args[0]);
  commandResult << "SWCHB set to " << std::hex << std::setw(2)
                << std::setfill('0') << args[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "tia"
void DebuggerParser::executeTia()
{
  commandResult << debugger.tiaDebug().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "timer"
void DebuggerParser::executeTimer()
{
  // Input variants:
  // timer                         current address @ current bank
  // timer +                       current address + mirrors @ current bank
  // timer *                       current address @ any bank
  // timer + *                     current address + mirrors @ any bank
  // timer addr                    addr @ current bank
  // timer addr +                  addr + mirrors @ current bank
  // timer addr *                  addr @ any bank
  // timer addr + *                addr + mirrors @ any bank
  // timer bank                    current address @ bank
  // timer bank +                  current address + mirrors @ bank
  // timer addr bank               addr @ bank
  // timer addr bank +             addr + mirrors @ bank
  // timer addr addr               addr, addr @ current bank
  // timer addr addr +             addr, addr + mirrors @ current bank
  // timer addr addr *             addr, addr @ any bank
  // timer addr addr + *           addr, addr + mirrors @ any bank
  // timer addr addr bank          addr, addr @ bank
  // timer addr addr bank bank     addr, addr @ bank, bank
  // timer addr addr bank bank +   addr, addr + mirrors @ bank, bank
  const uInt32 romBankCount = debugger.cartDebug().romBankCount();
  const bool banked = romBankCount > 1;

  bool mirrors = (argCount >= 1 && argStrings[argCount - 1] == "+") ||
                 (argCount >= 2 && argStrings[argCount - 2] == "+");
  bool anyBank = banked &&
     ((argCount >= 1 && argStrings[argCount - 1] == "*") ||
      (argCount >= 2 && argStrings[argCount - 2] == "*"));

  argCount -= ((mirrors ? 1 : 0) + (anyBank ? 1 : 0));

  if(argCount > 4)
  {
    outputCommandError("too many arguments", myCommand);
    return;
  }

  uInt16 addr[2]{};
  uInt8  bank[2]{};
  uInt32 numAddrs = 0, numBanks = 0;

  // set defaults:
  addr[0] = debugger.cpuDebug().pc();
  bank[0] = debugger.cartDebug().getBank(addr[0]);

  for(uInt32 i = 0; i < argCount; ++i)
  {
    if(std::cmp_greater_equal(args[i], std::max(0x80U, romBankCount - 1)))
    {
      if(numAddrs == 2)
      {
        outputCommandError("too many address arguments", myCommand);
        return;
      }
      addr[numAddrs++] = args[i];
    }
    else
    {
      if(anyBank || (numBanks == 1 && numAddrs < 2) || numBanks == 2)
      {
        outputCommandError("too many bank arguments", myCommand);
        return;
      }
      if(std::cmp_greater_equal(args[i], romBankCount))
      {
        commandResult << red("invalid bank");
        return;
      }
      bank[numBanks++] = args[i];
    }
  }

  const auto idx = (numAddrs < 2)
    ? debugger.m6502().addTimer(addr[0], bank[0], mirrors, anyBank)
    : debugger.m6502().addTimer(addr[0], addr[1],
                                bank[0], numBanks < 2 ? bank[0] : bank[1],
                                mirrors, anyBank);

  const bool isPartial = debugger.m6502().getTimer(idx).isPartial;
  if(numAddrs < 2 && !isPartial)
  {
    mirrors |= debugger.m6502().getTimer(idx).mirrors;
    anyBank |= debugger.m6502().getTimer(idx).anyBank;
  }

  commandResult << "set timer " << Base::toString(idx)
    << (numAddrs < 2 ? (isPartial ? " start" : " end") : "")
    << " at $" << Base::HEX4 << addr[0];

  if(numAddrs == 2)
    commandResult << ", $" << Base::HEX4 << addr[1];

  if(mirrors)
    commandResult << " + mirrors";

  if(banked)
  {
    if(anyBank)
      commandResult << " in all banks";
    else
    {
      commandResult << " in bank #" << dec << static_cast<int>(bank[0]);
      if(numBanks == 2)
        commandResult << ", #" << dec << static_cast<int>(bank[1]);
    }
  }
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
// "trapIf"
void DebuggerParser::executeTrapIf()
{
  executeTraps(true, true, "trapIf", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapRead"
void DebuggerParser::executeTrapRead()
{
  executeTraps(true, false, "trapRead");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapReadIf"
void DebuggerParser::executeTrapReadIf()
{
  executeTraps(true, false, "trapReadIf", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapWrite"
void DebuggerParser::executeTrapWrite()
{
  executeTraps(false, true, "trapWrite");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "trapWriteIf"
void DebuggerParser::executeTrapWriteIf()
{
  executeTraps(false, true, "trapWriteIf", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Wrapper function for trap(if)s
void DebuggerParser::executeTraps(bool read, bool write, string_view command,
                                  bool hasCond)
{
  const uInt32 ofs   = hasCond ? 1 : 0;
  const uInt32 begin = args[ofs];
  const uInt32 end   = (argCount == 2 + ofs) ? args[1 + ofs] : begin;

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
  const uInt32 beginRead  = Debugger::getBaseAddress(begin, true);
  const uInt32 endRead    = Debugger::getBaseAddress(end, true);
  const uInt32 beginWrite = Debugger::getBaseAddress(begin, false);
  const uInt32 endWrite   = Debugger::getBaseAddress(end, false);

  string condition;
  condition.reserve(128);

  // parenthesize provided and address range condition(s) (begin)
  if(hasCond)
  {
    condition += '(';
    condition += argStrings[0];
    condition += ")&&(";
  }

  // add address range condition(s) to provided condition
  if(read)
  {
    if(beginRead != endRead)
      std::format_to(std::back_inserter(condition),
                     "__lastBaseRead>={0}&&__lastBaseRead<={1}",
                     Base::toString(beginRead), Base::toString(endRead));
    else
      std::format_to(std::back_inserter(condition),
                     "__lastBaseRead=={}", Base::toString(beginRead));
  }
  if(read && write)
    condition += "||";
  if(write)
  {
    if(beginWrite != endWrite)
      std::format_to(std::back_inserter(condition),
                     "__lastBaseWrite>={0}&&__lastBaseWrite<={1}",
                     Base::toString(beginWrite), Base::toString(endWrite));
    else
      std::format_to(std::back_inserter(condition),
                     "__lastBaseWrite=={}", Base::toString(beginWrite));
  }
  // parenthesize provided condition (end)
  if(hasCond)
    condition += ')';

  if(YaccParser::parse(condition) != 0)
  {
    commandResult << red("invalid expression");
    return;
  }

  // Check for duplicate — duplicates remove each other
  const auto it = std::ranges::find_if(myTraps,
    [&](const unique_ptr<Trap>& trap)
    {
      return trap->begin == begin && trap->end == end &&
             trap->read == read   && trap->write == write &&
             trap->condition == condition;
    });

  if(it != myTraps.end())
  {
    const auto i = static_cast<uInt32>(it - myTraps.begin());
    if(!debugger.m6502().delCondTrap(i))
    {
      commandResult << "Internal error! Duplicate trap removal failed!";
      return;
    }
    // @sa666666: please check this:
    Vec::removeAt(myTraps, i);
    commandResult << "removed trap " << Base::toString(i);
    for(uInt32 addr = begin; addr <= end; ++addr)
      executeTrapRW(addr, read, write, false);
  }
  else
  {
    const auto ret = debugger.m6502().addCondTrap(
      YaccParser::getResult(), hasCond ? argStrings[0] : "");
    commandResult << "added trap " << Base::toString(ret);
    myTraps.emplace_back(std::make_unique<Trap>(read, write, begin, end, condition));
    for(uInt32 addr = begin; addr <= end; ++addr)
      executeTrapRW(addr, read, write, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// wrapper function for trap(if)/trapRead(if)/trapWrite(if) commands
void DebuggerParser::executeTrapRW(uInt32 addr, bool read, bool write, bool add)
{
  // Helper to add or remove read/write traps for a given address
  const auto setTraps = [&](uInt32 i)
  {
    if(read)  add ? debugger.addReadTrap(i)  : debugger.removeReadTrap(i);
    if(write) add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
  };

  switch(CartDebug::addressType(addr))
  {
    case CartDebug::AddrType::TIA:
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
      {
        if((i & 0x1080) != 0x0000)
          continue;
        // @sa666666: This seems wrong. E.g. trapRead 40 4f will never trigger
        if(read && (i & 0x000F) == (addr & 0x000F))
          add ? debugger.addReadTrap(i) : debugger.removeReadTrap(i);
        if(write && (i & 0x003F) == (addr & 0x003F))
          add ? debugger.addWriteTrap(i) : debugger.removeWriteTrap(i);
      }
      break;

    case CartDebug::AddrType::IO:
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
        if((i & 0x1280) == 0x0280 && (i & 0x029F) == (addr & 0x029F))
          setTraps(i);
      break;

    case CartDebug::AddrType::ZPRAM:
      for(uInt32 i = 0; i <= 0xFFFF; ++i)
        if((i & 0x1280) == 0x0080 && (i & 0x00FF) == (addr & 0x00FF))
          setTraps(i);
      break;

    case CartDebug::AddrType::ROM:
      if(addr >= 0x1000 && addr <= 0xFFFF)
        for(uInt32 i = 0x1000; i <= 0xFFFF; ++i)
          if((i % 0x2000 >= 0x1000) && (i & 0x0FFF) == (addr & 0x0FFF))
            setTraps(i);
      break;

    default:
      break;  // Not supposed to get here
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "type"
void DebuggerParser::executeType()
{
  uInt32 beg = args[0];
  uInt32 end = argCount >= 2 ? args[1] : beg;
  if(beg > end) std::swap(beg, end);

  for(uInt32 i = beg; i <= end; ++i)
  {
    std::format_to(std::ostreambuf_iterator(commandResult), "{:04x}: {}\n", i,
        debugger.cartDebug().accessTypeAsString(i));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "uHex"
void DebuggerParser::executeUHex()
{
  const bool enable = !Base::hexUppercase();
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
  if(argCount == 0)       debugger.cpuDebug().toggleV();
  else if(argCount == 1)  debugger.cpuDebug().setV(args[0]);
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
  const uInt16 states = (argCount == 0) ? 1 : args[0];
  const string_view type = unwind ? "unwind" : "rewind";
  string message;

  const uInt16 winds = unwind ? debugger.unwindStates(states, message)
                              : debugger.rewindStates(states, message);

  if(winds > 0)
  {
    debugger.rom().invalidate();
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "{} by {} state{} (~{})",
                   type, winds, winds > 1 ? "s" : "", message);
  }
  else
    std::format_to(std::ostreambuf_iterator(commandResult),
                   "no states left to {}", type);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "x"
void DebuggerParser::executeX()
{
  debugger.cpuDebug().setX(static_cast<uInt8>(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "y"
void DebuggerParser::executeY()
{
  debugger.cpuDebug().setY(static_cast<uInt8>(args[0]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// "z"
void DebuggerParser::executeZ()
{
  if(argCount == 0)       debugger.cpuDebug().toggleZ();
  else if(argCount == 1)  debugger.cpuDebug().setZ(args[0]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// List of all commands available to the parser
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
DebuggerParser::CommandArray DebuggerParser::commands = { {
  {
    "a",
    "Set Accumulator to <value>",
    "Valid value is 0 - ff\nExample: a ff, a #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeA
  },

  {
    "aud",
    "Mark 'AUD' range in disassembly",
    "Start and end of range required\nExample: aud f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeAud
  },

  {
    "autoSave",
    "Toggle automatic saving of commands (see 'save')",
    "Example: autoSave (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeAutoSave
  },

  {
    "base",
    "Set default number base to <base>",
    "Base is #2, #10, #16, bin, dec or hex\nExample: base hex",
    true,
    true,
    { Parameters::ARG_BASE_SPCL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeBase
  },

  {
    "bCol",
    "Mark 'bCol' range in disassembly",
    "Start and end of range required\nExample: bCol f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeBCol
  },


  {
    "break",
    "Break at <address> and <bank>",
    "Set/clear breakpoint on address (and all mirrors) and bank\nDefault are current PC and bank, valid address is 0 - ffff\n"
    "Example: break, break f000, break 7654 3\n         break ff00 ff (= all banks)",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeBreak
  },

  {
    "breakIf",
    "Set/clear breakpoint on <condition>",
    "Condition can include multiple items, see documentation\nExample: breakIf _scan>100",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeBreakIf
  },

  {
    "breakLabel",
    "Set/clear breakpoint on <address> (no mirrors, all banks)",
    "Example: breakLabel, breakLabel MainLoop",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeBreakLabel
  },

  {
    "c",
    "Carry Flag: set (0 or 1), or toggle (no arg)",
    "Example: c, c 0, c 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeC
  },

  {
    "cheat",
    "Use a cheat code (see manual for cheat types)",
    "Example: cheat 0040, cheat abff00",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeCheat
  },

  {
    "clearBreaks",
    "Clear all breakpoints",
    "Example: clearBreaks (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearBreaks
  },

  {
    "clearConfig",
    "Clear Distella config directives [bank xx]",
    "Example: clearConfig 0, clearConfig 1",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeClearConfig
  },

  {
    "clearHistory",
    "Clear the prompt history",
    "Example: clearhisotry (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearHistory
  },

  {
    "clearSaveStateIfs",
    "Clear all saveState points",
    "Example: ClearSaveStateIfss (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearSaveStateIfs
  },

  {
    "clearTimers",
    "Clear all timers",
    "All timers cleared\nExample: clearTimers (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearTimers
  },

  {
    "clearTraps",
    "Clear all traps",
    "All traps cleared, including any mirrored ones\nExample: clearTraps (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearTraps
  },

  {
    "clearWatches",
    "Clear all watches",
    "Example: clearWatches (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeClearWatches
  },

  {
    "cls",
    "Clear prompt area of text",
    "Completely clears screen, but keeps history of commands",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeCls
  },

  {
    "code",
    "Mark 'CODE' range in disassembly",
    "Start and end of range required\nExample: code f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeCode
  },

  {
    "col",
    "Mark 'COL' range in disassembly",
    "Start and end of range required\nExample: col f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeCol
  },

  {
    "colorTest",
    "Show value xx as TIA color",
    "Shows a color swatch for the given value\nExample: colorTest 1f",
    true,
    false,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeColorTest
  },

  {
    "d",
    "Decimal Flag: set (0 or 1), or toggle (no arg)",
    "Example: d, d 0, d 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeD
  },

  {
    "data",
    "Mark 'DATA' range in disassembly",
    "Start and end of range required\nExample: data f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeData
  },

  {
    "debugColors",
    "Show Fixed Debug Colors information",
    "Example: debugColors (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDebugColors
  },

  {
    "define",
    "Define label xx for address yy",
    "Example: define LABEL1 f100",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDefine
  },

  {
    "delBreakIf",
    "Delete conditional breakIf <xx>",
    "Example: delBreakIf 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelBreakIf
  },

  {
    "delFunction",
    "Delete function with label xx",
    "Example: delFunction FUNC1",
    true,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelFunction
  },

  {
    "delSaveStateIf",
    "Delete conditional saveState point <xx>",
    "Example: delSaveStateIf 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelSaveStateIf
  },

  {
    "delTimer",
    "Delete timer <xx>",
    "Example: delTimer 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelTimer
  },

  {
    "delTrap",
    "Delete trap <xx>",
    "Example: delTrap 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelTrap
  },

  {
    "delWatch",
    "Delete watch <xx>",
    "Example: delWatch 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeDelWatch
  },

  {
    "disAsm",
    "Disassemble address xx [yy lines] (default=PC)",
    "Disassembles from starting address <xx> (default=PC) for <yy> lines\n"
    "Example: disAsm, disAsm f000 100",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeDisAsm
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
    &DebuggerParser::executeDump
  },

  {
    "exec",
    "Execute script file <xx> [prefix]",
    "Example: exec script.dat, exec auto.txt",
    true,
    true,
    { Parameters::ARG_FILE, Parameters::ARG_LABEL, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeExec
  },

  {
    "exitRom",
    "Exit emulator, return to ROM launcher",
    "Self-explanatory",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeExitRom
  },

  {
    "frame",
    "Advance emulation by <xx> frames (default=1)",
    "Example: frame, frame 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeFrame
  },

  {
    "function",
    "Define function name xx for expression yy",
    "Example: function FUNC1 { ... }",
    true,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeFunction
  },

  {
    "gfx",
    "Mark 'GFX' range in disassembly",
    "Start and end of range required\nExample: gfx f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeGfx
  },

  {
    "help",
    "help <command>",
    "Show all commands, or give function for help on that command\n"
    "Example: help, help code",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeHelp
  },

  {
    "joy0Up",
    "Set joystick 0 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0Up 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy0Up
  },

  {
    "joy0Down",
    "Set joystick 0 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0Down 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy0Down
  },

  {
    "joy0Left",
    "Set joystick 0 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0Left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy0Left
  },

  {
    "joy0Right",
    "Set joystick 0 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0Left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy0Right
  },

  {
    "joy0Fire",
    "Set joystick 0 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy0Fire 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy0Fire
  },

  {
    "joy1Up",
    "Set joystick 1 up direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1Up 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy1Up
  },

  {
    "joy1Down",
    "Set joystick 1 down direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1Down 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy1Down
  },

  {
    "joy1Left",
    "Set joystick 1 left direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1Left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy1Left
  },

  {
    "joy1Right",
    "Set joystick 1 right direction to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1Left 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy1Right
  },

  {
    "joy1Fire",
    "Set joystick 1 fire button to value <x> (0 or 1), or toggle (no arg)",
    "Example: joy1Fire 0",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJoy1Fire
  },

  {
    "jump",
    "Scroll disassembly to address xx",
    "Moves disassembly listing to address <xx>\nExample: jump f400",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeJump
  },

  {
    "listBreaks",
    "List breakpoints",
    "Example: listBreaks (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeListBreaks
  },

  {
    "listConfig",
    "List Distella config directives [bank xx]",
    "Example: listConfig 0, listConfig 1",
    false,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeListConfig
  },

  {
    "listFunctions",
    "List user-defined functions",
    "Example: listFunctions (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeListFunctions
  },

  {
    "listSaveStateIfs",
    "List saveState points",
    "Example: listSaveStateIfs (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeListSaveStateIfs
  },

  {
    "listTimers",
    "List timers",
    "Lists all timers\nExample: listTimers (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeListTimers
  },

  {
    "listTraps",
    "List traps",
    "Lists all traps (read and/or write)\nExample: listTraps (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeListTraps
  },

  {
    "loadConfig",
    "Load Distella config file",
    "Example: loadConfig",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLoadConfig
  },

  {
    "loadAllStates",
    "Load all emulator states",
    "Example: loadAllStates (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLoadAllStates
  },

  {
    "loadState",
    "Load emulator state xx (0-9)",
    "Example: loadState 0, loadState 9",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLoadState
  },

  {
    "logBreaks",
    "Toggle logging of breaks/traps and continue emulation",
    "Example: logBreaks (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLogBreaks
  },

  {
    "logExec",
    "Toggle script execution logging to file",
    "Example: logExec (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLogExec
  },

  {
    "logTrace",
    "Toggle emulation logging",
    "Example: logBreaks",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeLogTrace
  },

  {
    "n",
    "Negative Flag: set (0 or 1), or toggle (no arg)",
    "Example: n, n 0, n 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeN
  },

  {
    "palette",
    "Show current TIA palette",
    "Example: palette (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executePalette
  },

  {
    "pc",
    "Set Program Counter to address xx",
    "Example: pc f000",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executePc
  },

  {
    "pCol",
    "Mark 'pCol' range in disassembly",
    "Start and end of range required\nExample: col f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executePCol
  },

  {
    "pGfx",
    "Mark 'pGfx' range in disassembly",
    "Start and end of range required\nExample: pGfx f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executePGfx
  },

  {
    "print",
    "Evaluate/print expression xx in hex/dec/binary",
    "Almost anything can be printed (constants, expressions, registers)\n"
    "Example: print pc, print f000",
    true,
    false,
    { Parameters::ARG_DWORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executePrint
  },

  {
    "printTimer",
    "Print statistics for timer <xx>",
    "Example: printTimer 0",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executePrintTimer
  },

  {
    "ram",
    "Show ZP RAM, or set address xx to yy1 [yy2 ...]",
    "Example: ram, ram 80 00 ...",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeRam
  },

  {
    "reset",
    "Reset system to power-on state",
    "System is completely reset, just as if it was just powered on",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeReset
  },

  {
    "resetTimers",
    "Reset all timers' statistics" ,
    "All timers resetted\nExample: resetTimers (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeResetTimers
  },

  {
    "rewind",
    "Rewind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: rewind, rewind 5",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeRewind
  },

  {
    "riot",
    "Show RIOT timer/input status",
    "Display text-based output of the contents of the RIOT tab",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeRiot
  },

  {
    "rom",
    "Set ROM address xx to yy1 [yy2 ...]",
    "What happens here depends on the current bankswitching scheme\n"
    "Example: rom f000 00 01 ff ...",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeRom
  },

  {
    "row",
    "Mark 'ROW' range in disassembly",
    "Start and end of range required\nExample: row f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeRow
  },

  {
    "run",
    "Exit debugger, return to emulator",
    "Self-explanatory",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeRun
  },

  {
    "runTo",
    "Run until string xx in disassembly",
    "Advance until the given string is detected in the disassembly\n"
    "Example: runTo lda",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeRunTo
  },

  {
    "runToPc",
    "Run until PC is set to value xx",
    "Example: runToPc f200",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeRunToPc
  },

  {
    "s",
    "Set Stack Pointer to value xx",
    "Accepts 8-bit value, Example: s f0",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeS
  },

  {
    "save",
    "Save breaks, watches, traps and functions to file [xx or ?]",
    "Example: save, save commands.script, save ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_FILE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSave
  },

  {
    "saveAccess",
    "Save the access counters to CSV file [?]",
    "Example: saveAccess, saveAccess ?\n"
    "NOTE: saves to user dir by default",
      false,
      false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
      &DebuggerParser::executeSaveAccess
  },

  {
    "saveConfig",
    "Save Distella config file (with default name)",
    "Example: saveConfig",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveConfig
  },

  {
    "saveDis",
    "Save Distella disassembly to file [?]",
    "Example: saveDis, saveDis ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveDisassembly
  },

  {
    "saveRom",
    "Save (possibly patched) ROM to file [?]",
    "Example: saveRom, saveRom ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveRom
  },

  {
    "saveSes",
    "Save console session to file [?]",
    "Example: saveSes, saveSes ?\n"
    "NOTE: saves to user dir by default",
    false,
    false,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveSes
  },

  {
    "saveSnap",
    "Save current TIA image to PNG file",
    "Save snapshot to current snapshot save directory\n"
    "Example: saveSnap (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveSnap
  },

  {
    "saveAllStates",
    "Save all emulator states",
    "Example: saveAllStates (no parameters)",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveAllStates
  },

  {
    "saveState",
    "Save emulator state xx (valid args 0-9)",
    "Example: saveState 0, saveState 9",
    true,
    false,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveState
  },

  {
    "saveStateIf",
    "Create saveState on <condition>",
    "Condition can include multiple items, see documentation\nExample: saveStateIf pc==f000",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSaveStateIf
  },

  {
    "scanLine",
    "Advance emulation by <xx> scanlines (default=1)",
    "Example: scanLine, scanLine 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeScanLine
  },

  {
    "step",
    "Single step CPU [with count xx]",
    "Example: step, step 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeStep
  },

  {
    "stepWhile",
    "Single step CPU while <condition> is true",
    "Example: stepWhile pc!=$f2a9",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeStepWhile
  },

  {
    "swchb",
    "Set SWCHB to xx",
    "Example: swchb fe",
    true,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeSwchb
  },

  {
    "tia",
    "Show TIA state",
    "Display text-based output of the contents of the TIA tab",
    false,
    false,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeTia
  },

  {
    "timer",
    "Set a cycle counting timer from addresses xx to yy [banks aa bb]",
    "Example: timer, timer 1000 + *, timer f000 f800 1 +",
    false,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_LABEL, Parameters::ARG_LABEL,
      Parameters::ARG_LABEL, Parameters::ARG_LABEL, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeTimer
  },

  {
    "trace",
    "Single step CPU over subroutines [with count xx]",
    "Example: trace, trace 100",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeTrace
  },

  {
    "trap",
    "Trap read/write access to address(es) xx [yy]",
    "Set/clear a R/W trap on the given address(es) and all mirrors\n"
    "Example: trap f000, trap f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeTrap
  },

  {
    "trapIf",
    "On <condition> trap R/W access to address(es) xx [yy]",
    "Set/clear a conditional R/W trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapIf _scan>#100 GRP0, trapIf _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      &DebuggerParser::executeTrapIf
  },

  {
    "trapRead",
    "Trap read access to address(es) xx [yy]",
    "Set/clear a read trap on the given address(es) and all mirrors\n"
    "Example: trapRead f000, trapRead f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeTrapRead
  },

  {
    "trapReadIf",
    "On <condition> trap read access to address(es) xx [yy]",
    "Set/clear a conditional read trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapReadIf _scan>#100 GRP0, trapReadIf _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      &DebuggerParser::executeTrapReadIf
  },

  {
    "trapWrite",
    "Trap write access to address(es) xx [yy]",
    "Set/clear a write trap on the given address(es) and all mirrors\n"
    "Example: trapWrite f000, trapWrite f000 f100",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeTrapWrite
  },

  {
    "trapWriteIf",
    "On <condition> trap write access to address(es) xx [yy]",
    "Set/clear a conditional write trap on the given address(es) and all mirrors\nCondition can include multiple items.\n"
    "Example: trapWriteIf _scan>#100 GRP0, trapWriteIf _bank==1 f000 f100",
      true,
      false,
      { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
      &DebuggerParser::executeTrapWriteIf
  },

  {
    "type",
    "Show access type for address xx [yy]",
    "Example: type f000, type f000 f010",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_MULTI_BYTE },
    &DebuggerParser::executeType
  },

  {
    "uHex",
    "Toggle upper/lowercase HEX display",
    "Note: not all hex output can be changed\n"
    "Example: uHex (no parameters)",
    false,
    true,
    { Parameters::ARG_END_ARGS },
    &DebuggerParser::executeUHex
  },

  {
    "undef",
    "Undefine label xx (if defined)",
    "Example: undef LABEL1",
    true,
    true,
    { Parameters::ARG_LABEL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeUndef
  },

  {
    "unwind",
    "Unwind state by one or [xx] steps/traces/scanlines/frames...",
    "Example: unwind, unwind 5",
    false,
    true,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeUnwind
  },

  {
    "v",
    "Overflow Flag: set (0 or 1), or toggle (no arg)",
    "Example: v, v 0, v 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeV
  },

  {
    "watch",
    "Print contents of address xx before every prompt",
    "Example: watch ram_80",
    true,
    false,
    { Parameters::ARG_WORD, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeWatch
  },

  {
    "x",
    "Set X Register to value xx",
    "Valid value is 0 - ff\nExample: x ff, x #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeX
  },

  {
    "y",
    "Set Y Register to value xx",
    "Valid value is 0 - ff\nExample: y ff, y #10",
    true,
    true,
    { Parameters::ARG_BYTE, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeY
  },

  {
    "z",
    "Zero Flag: set (0 or 1), or toggle (no arg)",
    "Example: z, z 0, z 1",
    false,
    true,
    { Parameters::ARG_BOOL, Parameters::ARG_END_ARGS },
    &DebuggerParser::executeZ
  }
} };
