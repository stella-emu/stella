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

#include <cctype>
#include <limits>

#include "Base.hxx"
#include "Debugger.hxx"
// Suppress warnings from Bison-generated code
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-macros"
#endif
#include "stella.tab.hxx"
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif
#include "YaccParser.hxx"

namespace YaccParser {

// ============================================================================
// Lexer
// ============================================================================

class Lexer
{
public:
  explicit Lexer(string_view input)
    : myC{input.data()}, myEnd{input.data() + input.size()} { }

  parser::symbol_type yylex();

private:
  enum class State : uInt8 { DEFAULT, IDENTIFIER, OPERATOR, SPACE };

  const char* myC{nullptr};
  const char* myEnd{nullptr};
  State myState{State::DEFAULT};
  string myIdbuf;

  static constexpr bool is_base_prefix(char x)
  {
    return x == '\\' || x == '$' || x == '#';
  }
  static constexpr bool is_identifier(char x)
  {
    return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'z') ||
           (x >= 'A' && x <= 'Z') || x == '.' || x == '_';
  }
  static constexpr bool is_operator(char x)
  {
    return x == '+' || x == '-' || x == '*' || x == '/' ||
           x == '<' || x == '>' || x == '|' || x == '&' ||
           x == '^' || x == '!' || x == '~' || x == '(' ||
           x == ')' || x == '=' || x == '%' || x == '[' || x == ']';
  }

  static parser::symbol_type make_char_tok(char c)
  {
    // The Bison symbol_type(int) constructor asserts that the value is a
    // grammar-declared token.  Characters not in this set (e.g. '?') must
    // return ERR so the parser emits a tidy error instead of aborting.
    static constexpr std::array<int, 17> kValid = {
      33, 37, 38, 40, 41, 42, 43, 45, 47, 60, 62, 64, 91, 93, 94, 124, 126
    };
    const int v = static_cast<unsigned char>(c);
    if(std::ranges::find(kValid, v) != kValid.end())
      // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange,modernize-return-braced-init-list)
      return parser::symbol_type(v);
    return parser::make_ERR();
  }

  static int const_to_int(string_view s);

  static CartMethod getCartSpecial(string_view s);
  static CpuMethod  getCpuSpecial(string_view s);
  static RiotMethod getRiotSpecial(string_view s);
  static TiaMethod  getTiaSpecial(string_view s);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// const_to_int converts a string to a number in the current or overridden base.
// Returns -1 on error (including overflow); negative numbers are the parser's
// responsibility.
int Lexer::const_to_int(string_view s)
{
  Common::Base::Fmt format = Common::Base::format();

  if (!s.empty()) {
    switch (s.front()) {
      case '\\': format = Common::Base::Fmt::_2;  s.remove_prefix(1); break;
      case '#':  format = Common::Base::Fmt::_10; s.remove_prefix(1); break;
      case '$':  format = Common::Base::Fmt::_16; s.remove_prefix(1); break;
      default: break;
    }
  }

  // A bare base prefix with no digits following it (e.g. "$" on its own)
  // is not a valid numeral.
  if (s.empty())
    return -1;

  // Accumulate in Int64 and bail as soon as the value can no longer fit in
  // the int NUMBER token; this both avoids signed-overflow UB on long/
  // adversarial numerals and bounds the work done on them.
  static constexpr Int64 kMax = std::numeric_limits<int>::max();
  Int64 ret = 0;

  switch (format) {
    case Common::Base::Fmt::_2:
      for (const char c : s) {
        if (c != '0' && c != '1') return -1;
        ret = ret * 2 + (c - '0');
        if (ret > kMax) return -1;
      }
      return static_cast<int>(ret);

    case Common::Base::Fmt::_10:
      for (const char c : s) {
        if (!isdigit(c)) return -1;
        ret = ret * 10 + (c - '0');
        if (ret > kMax) return -1;
      }
      return static_cast<int>(ret);

    case Common::Base::Fmt::_16:
      for (const char c : s) {
        if (!isxdigit(c)) return -1;
        const int dig = (c - '0');
        ret = ret * 16 + (dig > 9 ? tolower(c) - 'a' + 10 : dig);
        if (ret > kMax) return -1;
      }
      return static_cast<int>(ret);

    default:
      return 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartMethod Lexer::getCartSpecial(string_view s)
{
  if (BSPF::equalsIgnoreCase(s, "_bank"))           return &CartDebug::getPCBank;
  if (BSPF::equalsIgnoreCase(s, "__lastBaseRead"))  return &CartDebug::lastReadBaseAddress;
  if (BSPF::equalsIgnoreCase(s, "__lastBaseWrite")) return &CartDebug::lastWriteBaseAddress;
  if (BSPF::equalsIgnoreCase(s, "__lastRead"))      return &CartDebug::lastReadAddress;
  if (BSPF::equalsIgnoreCase(s, "__lastWrite"))     return &CartDebug::lastWriteAddress;
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuMethod Lexer::getCpuSpecial(string_view s)
{
  if (BSPF::equalsIgnoreCase(s, "a"))        return &CpuDebug::a;
  if (BSPF::equalsIgnoreCase(s, "x"))        return &CpuDebug::x;
  if (BSPF::equalsIgnoreCase(s, "y"))        return &CpuDebug::y;
  if (BSPF::equalsIgnoreCase(s, "pc"))       return &CpuDebug::pc;
  if (BSPF::equalsIgnoreCase(s, "sp"))       return &CpuDebug::sp;
  if (BSPF::equalsIgnoreCase(s, "c"))        return &CpuDebug::c;
  if (BSPF::equalsIgnoreCase(s, "z"))        return &CpuDebug::z;
  if (BSPF::equalsIgnoreCase(s, "n"))        return &CpuDebug::n;
  if (BSPF::equalsIgnoreCase(s, "v"))        return &CpuDebug::v;
  if (BSPF::equalsIgnoreCase(s, "d"))        return &CpuDebug::d;
  if (BSPF::equalsIgnoreCase(s, "i"))        return &CpuDebug::i;
  if (BSPF::equalsIgnoreCase(s, "b"))        return &CpuDebug::b;
  if (BSPF::equalsIgnoreCase(s, "_iCycles")) return &CpuDebug::icycles;
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotMethod Lexer::getRiotSpecial(string_view s)
{
  if (BSPF::equalsIgnoreCase(s, "_timWrapRead"))    return &RiotDebug::timWrappedOnRead;
  if (BSPF::equalsIgnoreCase(s, "_timWrapWrite"))   return &RiotDebug::timWrappedOnWrite;
  if (BSPF::equalsIgnoreCase(s, "_fTimReadCycles")) return &RiotDebug::timReadCycles;
  if (BSPF::equalsIgnoreCase(s, "_inTim"))          return &RiotDebug::intimAsInt;
  if (BSPF::equalsIgnoreCase(s, "_timInt"))         return &RiotDebug::timintAsInt;
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaMethod Lexer::getTiaSpecial(string_view s)
{
  if (BSPF::equalsIgnoreCase(s, "_scan"))          return &TIADebug::scanlines;
  if (BSPF::equalsIgnoreCase(s, "_scanEnd"))       return &TIADebug::scanlinesLastFrame;
  if (BSPF::equalsIgnoreCase(s, "_sCycles"))       return &TIADebug::cyclesThisLine;
  if (BSPF::equalsIgnoreCase(s, "_fCount"))        return &TIADebug::frameCount;
  if (BSPF::equalsIgnoreCase(s, "_fCycles"))       return &TIADebug::frameCycles;
  if (BSPF::equalsIgnoreCase(s, "_fWsyncCycles"))  return &TIADebug::frameWsyncCycles;
  if (BSPF::equalsIgnoreCase(s, "_cyclesLo"))      return &TIADebug::cyclesLo;
  if (BSPF::equalsIgnoreCase(s, "_cyclesHi"))      return &TIADebug::cyclesHi;
  if (BSPF::equalsIgnoreCase(s, "_cClocks"))       return &TIADebug::clocksThisLine;
  if (BSPF::equalsIgnoreCase(s, "_vSync"))         return &TIADebug::vsyncAsInt;
  if (BSPF::equalsIgnoreCase(s, "_vBlank"))        return &TIADebug::vblankAsInt;
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
parser::symbol_type Lexer::yylex()
{
  char o{0}, p{0};

  while (myC != myEnd) {
    switch (myState) {
      case State::SPACE:
        if (isspace(*myC))
          ++myC;
        else if (is_identifier(*myC) || is_base_prefix(*myC))
          myState = State::IDENTIFIER;
        else if (is_operator(*myC))
          myState = State::OPERATOR;
        else
          myState = State::DEFAULT;
        break;

      case State::IDENTIFIER: {
        myIdbuf.clear();
        myIdbuf += *myC++;
        while (myC != myEnd && is_identifier(*myC))
          myIdbuf += *myC++;
        myState = State::DEFAULT;

        // Labels have priority over specials; specials have priority over numbers.
        // (A bare 'a' always means the accumulator, not hex 0xa. Use '$a' or 'A'
        // for the literal.)
        if (Debugger::debugger().cartDebug().getAddress(myIdbuf) > -1)
          return parser::make_EQUATE(myIdbuf);

        CpuMethod  cpuMeth  = nullptr;
        CartMethod cartMeth = nullptr;
        RiotMethod riotMeth = nullptr;
        TiaMethod  tiaMeth  = nullptr;

        cpuMeth  = getCpuSpecial (myIdbuf); if (cpuMeth)  return parser::make_CPU_METHOD (cpuMeth);
        cartMeth = getCartSpecial(myIdbuf); if (cartMeth) return parser::make_CART_METHOD(cartMeth);
        riotMeth = getRiotSpecial(myIdbuf); if (riotMeth) return parser::make_RIOT_METHOD(riotMeth);
        tiaMeth  = getTiaSpecial (myIdbuf); if (tiaMeth)  return parser::make_TIA_METHOD (tiaMeth);

        if (!Debugger::debugger().getFunctionDef(myIdbuf).empty())
          return parser::make_FUNCTION(myIdbuf);

        const int val = const_to_int(myIdbuf);
        return val >= 0 ? parser::make_NUMBER(val) : parser::make_ERR();
      }

      case State::OPERATOR:
        o = *myC++;
        if (myC == myEnd)
          return make_char_tok(o);
        if (isspace(*myC)) {
          myState = State::SPACE;
          return make_char_tok(o);
        }
        if (is_identifier(*myC) || is_base_prefix(*myC)) {
          myState = State::IDENTIFIER;
          return make_char_tok(o);
        }
        myState = State::DEFAULT;
        p = *myC++;
        if      (o == '>' && p == '=') return parser::make_GTE();
        else if (o == '<' && p == '=') return parser::make_LTE();
        else if (o == '!' && p == '=') return parser::make_NE();
        else if (o == '=' && p == '=') return parser::make_EQ();
        else if (o == '|' && p == '|') return parser::make_LOG_OR();
        else if (o == '&' && p == '&') return parser::make_LOG_AND();
        else if (o == '<' && p == '<') return parser::make_SHL();
        else if (o == '>' && p == '>') return parser::make_SHR();
        else {
          --myC;
          return make_char_tok(o);
        }

      case State::DEFAULT:
        if (isspace(*myC))
          myState = State::SPACE;
        else if (is_identifier(*myC) || is_base_prefix(*myC))
          myState = State::IDENTIFIER;
        else if (is_operator(*myC))
          myState = State::OPERATOR;
        else
          return make_char_tok(*myC++);
        break;

      default:
        break;
    }
  }

  return parser::make_YYEOF();
}

// ============================================================================
// yylex bridge called by the generated parser
// ============================================================================

// NOLINTNEXTLINE(misc-use-internal-linkage)
parser::symbol_type yylex(Lexer& lexer);
// NOLINTNEXTLINE(misc-use-internal-linkage)
parser::symbol_type yylex(Lexer& lexer)
{
  return lexer.yylex();
}

// ============================================================================
// Public API
// ============================================================================

namespace {
  string& s_errorMsg()
  {
    static string msg;
    return msg;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Expression trees are evaluated and destroyed recursively (one C++ stack
// frame per nesting level), so an absurdly long, deeply-nested expression
// (chained operators, nested parens/derefs) could exhaust the stack.  Capping
// the input length bounds the tree depth well below any risk of that.
static constexpr size_t kMaxExpressionLength = 1024;

unique_ptr<Expression> parse(string_view in)
{
  if (in.size() > kMaxExpressionLength) {
    s_errorMsg() = "Expression too long";
    return nullptr;
  }

  unique_ptr<Expression> result;
  string parseError;

  Lexer lexer{in};
  parser p{lexer, result, parseError};

  if (p.parse() != 0) {
    s_errorMsg() = std::move(parseError);
    return nullptr;
  }
  s_errorMsg().clear();
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& errorMessage()
{
  return s_errorMsg();
}

}  // namespace YaccParser
