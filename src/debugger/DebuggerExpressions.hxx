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

#ifndef DEBUGGER_EXPRESSIONS_HXX
#define DEBUGGER_EXPRESSIONS_HXX

#include <functional>

#include "bspf.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"
#include "Debugger.hxx"
#include "Expression.hxx"

/**
  All expressions currently supported by the debugger.
  @author  B. Watson and Stephen Anthony
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BinAndExpression : public Expression
{
  public:
    BinAndExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() & myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BinNotExpression : public Expression
{
  public:
    explicit BinNotExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return ~(myLHS->evaluate()); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BinOrExpression : public Expression
{
  public:
    BinOrExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() | myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BinXorExpression : public Expression
{
  public:
    BinXorExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() ^ myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ByteDerefExpression : public Expression
{
  public:
    explicit ByteDerefExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return Debugger::debugger().peek(myLHS->evaluate()); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ByteDerefOffsetExpression : public Expression
{
  public:
    ByteDerefOffsetExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return Debugger::debugger().peek(myLHS->evaluate() + myRHS->evaluate()); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ConstExpression : public Expression
{
  public:
    explicit ConstExpression(int value) : myValue{value} { }
    Int32 evaluate() const override
      { return myValue; }

  private:
    int myValue;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CpuMethodExpression : public Expression
{
  public:
    explicit CpuMethodExpression(CpuMethod method) : myMethod{std::mem_fn(method)} { }
    Int32 evaluate() const override
      { return myMethod(Debugger::debugger().cpuDebug()); }

  private:
    std::function<int(const CpuDebug&)> myMethod;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class DivExpression : public Expression
{
  public:
    DivExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { const int denom = myRHS->evaluate();
        return denom == 0 ? 0 : myLHS->evaluate() / denom; }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class EqualsExpression : public Expression
{
  public:
    EqualsExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() == myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class EquateExpression : public Expression
{
  public:
    explicit EquateExpression(string_view label) : myLabel{label} { }
    Int32 evaluate() const override
      { return Debugger::debugger().cartDebug().getAddress(myLabel); }

  private:
    string myLabel;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class FunctionExpression : public Expression
{
  public:
    explicit FunctionExpression(string_view label) : myLabel{label} { }
    Int32 evaluate() const override
      { return Debugger::debugger().getFunction(myLabel).evaluate(); }

  private:
    string myLabel;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class GreaterEqualsExpression : public Expression
{
  public:
    GreaterEqualsExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() >= myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class GreaterExpression : public Expression
{
  public:
    GreaterExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() > myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class HiByteExpression : public Expression
{
  public:
    explicit HiByteExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return 0xff & (myLHS->evaluate() >> 8); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LessEqualsExpression : public Expression
{
  public:
    LessEqualsExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() <= myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LessExpression : public Expression
{
  public:
    LessExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() < myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LoByteExpression : public Expression
{
  public:
    explicit LoByteExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return 0xff & myLHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LogAndExpression : public Expression
{
  public:
    LogAndExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() && myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LogNotExpression : public Expression
{
  public:
    explicit LogNotExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return !(myLHS->evaluate()); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LogOrExpression : public Expression
{
  public:
    LogOrExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() || myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MinusExpression : public Expression
{
  public:
    MinusExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() - myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ModExpression : public Expression
{
  public:
    ModExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { const int rhs = myRHS->evaluate();
        return rhs == 0 ? 0 : myLHS->evaluate() % rhs; }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MultExpression : public Expression
{
  public:
    MultExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() * myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NotEqualsExpression : public Expression
{
  public:
    NotEqualsExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() != myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class PlusExpression : public Expression
{
  public:
    PlusExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() + myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CartMethodExpression : public Expression
{
  public:
    explicit CartMethodExpression(CartMethod method) :
      myMethod{std::mem_fn(method)} { }
    Int32 evaluate() const override
      { return myMethod(Debugger::debugger().cartDebug()); }

  private:
    std::function<int(CartDebug&)> myMethod;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ShiftLeftExpression : public Expression
{
  public:
    ShiftLeftExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() << myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ShiftRightExpression : public Expression
{
  public:
    ShiftRightExpression(unique_ptr<Expression> left, unique_ptr<Expression> right)
      : Expression(std::move(left), std::move(right)) { }
    Int32 evaluate() const override
      { return myLHS->evaluate() >> myRHS->evaluate(); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class RiotMethodExpression : public Expression
{
  public:
    explicit RiotMethodExpression(RiotMethod method) :
      myMethod{std::mem_fn(method)} { }
    Int32 evaluate() const override
      { return myMethod(Debugger::debugger().riotDebug()); }

  private:
    std::function<int(const RiotDebug&)> myMethod;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class TiaMethodExpression : public Expression
{
  public:
    explicit TiaMethodExpression(TiaMethod method) :
      myMethod{std::mem_fn(method)} { }
    Int32 evaluate() const override
      { return myMethod(Debugger::debugger().tiaDebug()); }

  private:
    std::function<int(const TIADebug&)> myMethod;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class UnaryMinusExpression : public Expression
{
  public:
    explicit UnaryMinusExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return -(myLHS->evaluate()); }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class WordDerefExpression : public Expression
{
  public:
    explicit WordDerefExpression(unique_ptr<Expression> left)
      : Expression(std::move(left)) { }
    Int32 evaluate() const override
      { return Debugger::debugger().dpeekAsInt(myLHS->evaluate()); }
};

#endif  // DEBUGGER_EXPRESSIONS_HXX
