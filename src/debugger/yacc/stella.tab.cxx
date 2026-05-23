// NOLINTBEGIN
// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.





#include "stella.tab.hxx"


// Unqualified %code blocks.
#line 52 "stella.y"

  #include "DebuggerExpressions.hxx"
  namespace YaccParser {
    parser::symbol_type yylex(Lexer& lexer);
  }

#line 53 "stella.tab.cxx"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif



// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 24 "stella.y"
namespace YaccParser {
#line 127 "stella.tab.cxx"

  /// Build a parser object.
  parser::parser (Lexer& lexer_yyarg, unique_ptr<Expression>& result_yyarg, string& errorMsg_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      lexer (lexer_yyarg),
      result (result_yyarg),
      errorMsg (errorMsg_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_kind_type
  parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_CART_METHOD: // CART_METHOD
        value.YY_MOVE_OR_COPY< CartMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_CPU_METHOD: // CPU_METHOD
        value.YY_MOVE_OR_COPY< CpuMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_RIOT_METHOD: // RIOT_METHOD
        value.YY_MOVE_OR_COPY< RiotMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_TIA_METHOD: // TIA_METHOD
        value.YY_MOVE_OR_COPY< TiaMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_EQUATE: // EQUATE
      case symbol_kind::S_FUNCTION: // FUNCTION
        value.YY_MOVE_OR_COPY< string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_expression: // expression
        value.YY_MOVE_OR_COPY< unique_ptr<Expression> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s)
  {
    switch (that.kind ())
    {
      case symbol_kind::S_CART_METHOD: // CART_METHOD
        value.move< CartMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_CPU_METHOD: // CPU_METHOD
        value.move< CpuMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_RIOT_METHOD: // RIOT_METHOD
        value.move< RiotMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_TIA_METHOD: // TIA_METHOD
        value.move< TiaMethod > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
        value.move< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_EQUATE: // EQUATE
      case symbol_kind::S_FUNCTION: // FUNCTION
        value.move< string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_expression: // expression
        value.move< unique_ptr<Expression> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_CART_METHOD: // CART_METHOD
        value.copy< CartMethod > (that.value);
        break;

      case symbol_kind::S_CPU_METHOD: // CPU_METHOD
        value.copy< CpuMethod > (that.value);
        break;

      case symbol_kind::S_RIOT_METHOD: // RIOT_METHOD
        value.copy< RiotMethod > (that.value);
        break;

      case symbol_kind::S_TIA_METHOD: // TIA_METHOD
        value.copy< TiaMethod > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
        value.copy< int > (that.value);
        break;

      case symbol_kind::S_EQUATE: // EQUATE
      case symbol_kind::S_FUNCTION: // FUNCTION
        value.copy< string > (that.value);
        break;

      case symbol_kind::S_expression: // expression
        value.copy< unique_ptr<Expression> > (that.value);
        break;

      default:
        break;
    }

    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_CART_METHOD: // CART_METHOD
        value.move< CartMethod > (that.value);
        break;

      case symbol_kind::S_CPU_METHOD: // CPU_METHOD
        value.move< CpuMethod > (that.value);
        break;

      case symbol_kind::S_RIOT_METHOD: // RIOT_METHOD
        value.move< RiotMethod > (that.value);
        break;

      case symbol_kind::S_TIA_METHOD: // TIA_METHOD
        value.move< TiaMethod > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
        value.move< int > (that.value);
        break;

      case symbol_kind::S_EQUATE: // EQUATE
      case symbol_kind::S_FUNCTION: // FUNCTION
        value.move< string > (that.value);
        break;

      case symbol_kind::S_expression: // expression
        value.move< unique_ptr<Expression> > (that.value);
        break;

      default:
        break;
    }

    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " (";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (lexer));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_CART_METHOD: // CART_METHOD
        yylhs.value.emplace< CartMethod > ();
        break;

      case symbol_kind::S_CPU_METHOD: // CPU_METHOD
        yylhs.value.emplace< CpuMethod > ();
        break;

      case symbol_kind::S_RIOT_METHOD: // RIOT_METHOD
        yylhs.value.emplace< RiotMethod > ();
        break;

      case symbol_kind::S_TIA_METHOD: // TIA_METHOD
        yylhs.value.emplace< TiaMethod > ();
        break;

      case symbol_kind::S_NUMBER: // NUMBER
        yylhs.value.emplace< int > ();
        break;

      case symbol_kind::S_EQUATE: // EQUATE
      case symbol_kind::S_FUNCTION: // FUNCTION
        yylhs.value.emplace< string > ();
        break;

      case symbol_kind::S_expression: // expression
        yylhs.value.emplace< unique_ptr<Expression> > ();
        break;

      default:
        break;
    }



      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // statement: expression
#line 103 "stella.y"
                { result = std::move(yystack_[0].value.as < unique_ptr<Expression> > ()); }
#line 656 "stella.tab.cxx"
    break;

  case 3: // expression: expression '+' expression
#line 107 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<PlusExpression>         (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 662 "stella.tab.cxx"
    break;

  case 4: // expression: expression '-' expression
#line 108 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<MinusExpression>        (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 668 "stella.tab.cxx"
    break;

  case 5: // expression: expression '*' expression
#line 109 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<MultExpression>         (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 674 "stella.tab.cxx"
    break;

  case 6: // expression: expression '/' expression
#line 110 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<DivExpression>          (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 680 "stella.tab.cxx"
    break;

  case 7: // expression: expression '%' expression
#line 111 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ModExpression>          (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 686 "stella.tab.cxx"
    break;

  case 8: // expression: expression '&' expression
#line 112 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<BinAndExpression>       (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 692 "stella.tab.cxx"
    break;

  case 9: // expression: expression '|' expression
#line 113 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<BinOrExpression>        (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 698 "stella.tab.cxx"
    break;

  case 10: // expression: expression '^' expression
#line 114 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<BinXorExpression>       (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 704 "stella.tab.cxx"
    break;

  case 11: // expression: expression '<' expression
#line 115 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LessExpression>         (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 710 "stella.tab.cxx"
    break;

  case 12: // expression: expression '>' expression
#line 116 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<GreaterExpression>      (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 716 "stella.tab.cxx"
    break;

  case 13: // expression: expression GTE expression
#line 117 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<GreaterEqualsExpression>(std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 722 "stella.tab.cxx"
    break;

  case 14: // expression: expression LTE expression
#line 118 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LessEqualsExpression>   (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 728 "stella.tab.cxx"
    break;

  case 15: // expression: expression NE expression
#line 119 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<NotEqualsExpression>    (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 734 "stella.tab.cxx"
    break;

  case 16: // expression: expression EQ expression
#line 120 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<EqualsExpression>       (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 740 "stella.tab.cxx"
    break;

  case 17: // expression: expression SHR expression
#line 121 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ShiftRightExpression>   (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 746 "stella.tab.cxx"
    break;

  case 18: // expression: expression SHL expression
#line 122 "stella.y"
                               { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ShiftLeftExpression>    (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 752 "stella.tab.cxx"
    break;

  case 19: // expression: expression LOG_OR expression
#line 123 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LogOrExpression>  (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 758 "stella.tab.cxx"
    break;

  case 20: // expression: expression LOG_AND expression
#line 124 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LogAndExpression> (std::move(yystack_[2].value.as < unique_ptr<Expression> > ()), std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 764 "stella.tab.cxx"
    break;

  case 21: // expression: '-' expression
#line 125 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<UnaryMinusExpression>(std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 770 "stella.tab.cxx"
    break;

  case 22: // expression: '~' expression
#line 126 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<BinNotExpression>    (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 776 "stella.tab.cxx"
    break;

  case 23: // expression: '!' expression
#line 127 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LogNotExpression>    (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 782 "stella.tab.cxx"
    break;

  case 24: // expression: '*' expression
#line 128 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ByteDerefExpression> (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 788 "stella.tab.cxx"
    break;

  case 25: // expression: '@' expression
#line 129 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<WordDerefExpression> (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 794 "stella.tab.cxx"
    break;

  case 26: // expression: '<' expression
#line 130 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<LoByteExpression>    (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 800 "stella.tab.cxx"
    break;

  case 27: // expression: '>' expression
#line 131 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<HiByteExpression>    (std::move(yystack_[0].value.as < unique_ptr<Expression> > ())); }
#line 806 "stella.tab.cxx"
    break;

  case 28: // expression: '(' expression ')'
#line 132 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::move(yystack_[1].value.as < unique_ptr<Expression> > ()); }
#line 812 "stella.tab.cxx"
    break;

  case 29: // expression: expression '[' expression ']'
#line 133 "stella.y"
                                   { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ByteDerefOffsetExpression>(std::move(yystack_[3].value.as < unique_ptr<Expression> > ()), std::move(yystack_[1].value.as < unique_ptr<Expression> > ())); }
#line 818 "stella.tab.cxx"
    break;

  case 30: // expression: NUMBER
#line 134 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<ConstExpression>    (yystack_[0].value.as < int > ()); }
#line 824 "stella.tab.cxx"
    break;

  case 31: // expression: EQUATE
#line 135 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<EquateExpression>   (yystack_[0].value.as < string > ()); }
#line 830 "stella.tab.cxx"
    break;

  case 32: // expression: CPU_METHOD
#line 136 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<CpuMethodExpression> (yystack_[0].value.as < CpuMethod > ()); }
#line 836 "stella.tab.cxx"
    break;

  case 33: // expression: CART_METHOD
#line 137 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<CartMethodExpression>(yystack_[0].value.as < CartMethod > ()); }
#line 842 "stella.tab.cxx"
    break;

  case 34: // expression: RIOT_METHOD
#line 138 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<RiotMethodExpression>(yystack_[0].value.as < RiotMethod > ()); }
#line 848 "stella.tab.cxx"
    break;

  case 35: // expression: TIA_METHOD
#line 139 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<TiaMethodExpression> (yystack_[0].value.as < TiaMethod > ()); }
#line 854 "stella.tab.cxx"
    break;

  case 36: // expression: FUNCTION
#line 140 "stella.y"
                { yylhs.value.as < unique_ptr<Expression> > () = std::make_unique<FunctionExpression>  (yystack_[0].value.as < string > ()); }
#line 860 "stella.tab.cxx"
    break;

  case 37: // expression: ERR
#line 141 "stella.y"
                { error("Invalid label or constant"); YYERROR; }
#line 866 "stella.tab.cxx"
    break;


#line 870 "stella.tab.cxx"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (YY_MOVE (msg));
      }


    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;


      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.what ());
  }

  const char *
  parser::symbol_name (symbol_kind_type yysymbol)
  {
    static const char *const yy_sname[] =
    {
    "end of file", "error", "invalid token", "NUMBER", "ERR", "EQUATE",
  "CART_METHOD", "CPU_METHOD", "RIOT_METHOD", "TIA_METHOD", "FUNCTION",
  "LOG_OR", "LOG_AND", "SHR", "SHL", "GTE", "LTE", "NE", "EQ", "DEREF",
  "UMINUS", "'+'", "'-'", "'*'", "'/'", "'%'", "'|'", "'^'", "'&'", "'<'",
  "'>'", "'['", "'~'", "'!'", "'@'", "'('", "')'", "']'", "$accept",
  "statement", "expression", YY_NULLPTR
    };
    return yy_sname[yysymbol];
  }



  // parser::context.
  parser::context::context (const parser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  parser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    const int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        const int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        const int yychecklim = yylast_ - yyn + 1;
        const int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }






  int
  parser::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -23;

  const signed char parser::yytable_ninf_ = -1;

  const short
  parser::yypact_[] =
  {
      35,   -23,   -23,   -23,   -23,   -23,   -23,   -23,   -23,    35,
      35,    35,    35,    35,    35,    35,    35,     8,   113,   -22,
     -22,    31,    31,   -22,   -22,   -22,    87,   -23,    35,    35,
      35,    35,    35,    35,    35,    35,    35,    35,    35,    35,
      35,    35,    35,    35,    35,    35,    35,   -23,   162,   181,
     210,   210,    31,    31,    31,    31,   134,   134,   155,   155,
     155,   187,   187,   206,    31,    31,    60,   -23
  };

  const signed char
  parser::yydefact_[] =
  {
       0,    30,    37,    31,    33,    32,    34,    35,    36,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     2,    21,
      24,    26,    27,    22,    23,    25,     0,     1,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    28,    19,    20,
      17,    18,    13,    14,    15,    16,     3,     4,     5,     6,
       7,     9,    10,     8,    11,    12,     0,    29
  };

  const signed char
  parser::yypgoto_[] =
  {
     -23,   -23,    -9
  };

  const signed char
  parser::yydefgoto_[] =
  {
       0,    17,    18
  };

  const signed char
  parser::yytable_[] =
  {
      19,    20,    21,    22,    23,    24,    25,    26,    27,    46,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,     1,     2,
       3,     4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,
       0,     0,     0,     0,     0,     0,     0,     9,    10,     0,
      -1,    -1,    46,     0,    11,    12,     0,    13,    14,    15,
      16,    28,    29,    30,    31,    32,    33,    34,    35,     0,
       0,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,     0,     0,     0,     0,     0,    67,    28,    29,
      30,    31,    32,    33,    34,    35,     0,     0,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,     0,
       0,     0,     0,    47,    28,    29,    30,    31,    32,    33,
      34,    35,     0,     0,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    28,    29,    30,    31,    32,
      33,    34,    35,     0,     0,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    28,    29,    30,    31,
      32,    33,    34,    35,    29,    30,    31,    32,    33,    34,
      35,    41,    42,    43,    44,    45,    46,     0,    41,    42,
      43,    44,    45,    46,    30,    31,    32,    33,    34,    35,
      30,    31,    32,    33,    34,    35,     0,    41,    42,    43,
      44,    45,    46,     0,     0,    43,    44,    45,    46,    30,
      31,    32,    33,    34,    35,    32,    33,    34,    35,     0,
       0,     0,     0,     0,     0,    44,    45,    46,     0,    44,
      45,    46
  };

  const signed char
  parser::yycheck_[] =
  {
       9,    10,    11,    12,    13,    14,    15,    16,     0,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,     3,     4,
       5,     6,     7,     8,     9,    10,    15,    16,    17,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    -1,
      29,    30,    31,    -1,    29,    30,    -1,    32,    33,    34,
      35,    11,    12,    13,    14,    15,    16,    17,    18,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    -1,    37,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,    -1,    -1,    36,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    11,    12,    13,    14,    15,
      16,    17,    18,    -1,    -1,    -1,    -1,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    11,    12,    13,    14,
      15,    16,    17,    18,    12,    13,    14,    15,    16,    17,
      18,    26,    27,    28,    29,    30,    31,    -1,    26,    27,
      28,    29,    30,    31,    13,    14,    15,    16,    17,    18,
      13,    14,    15,    16,    17,    18,    -1,    26,    27,    28,
      29,    30,    31,    -1,    -1,    28,    29,    30,    31,    13,
      14,    15,    16,    17,    18,    15,    16,    17,    18,    -1,
      -1,    -1,    -1,    -1,    -1,    29,    30,    31,    -1,    29,
      30,    31
  };

  const signed char
  parser::yystos_[] =
  {
       0,     3,     4,     5,     6,     7,     8,     9,    10,    22,
      23,    29,    30,    32,    33,    34,    35,    39,    40,    40,
      40,    40,    40,    40,    40,    40,    40,     0,    11,    12,
      13,    14,    15,    16,    17,    18,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    36,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    37
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    38,    39,    40,    40,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    40,    40,    40,    40
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     2,     2,     2,     3,     4,
       1,     1,     1,     1,     1,     1,     1,     1
  };




#if YYDEBUG
  const unsigned char
  parser::yyrline_[] =
  {
       0,   103,   103,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141
  };

  void
  parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 24 "stella.y"
} // YaccParser
#line 1360 "stella.tab.cxx"

#line 144 "stella.y"


void YaccParser::parser::error(const string& msg)
{
  errorMsg = msg;
}
// NOLINTEND
