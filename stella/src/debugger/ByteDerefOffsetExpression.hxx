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
// $Id: ByteDerefOffsetExpression.hxx,v 1.1 2005-07-19 01:31:36 urchlay Exp $
//============================================================================

#ifndef BYTEDEREFOFFSET_EXPRESSION_HXX
#define BYTEDEREFOFFSET_EXPRESSION_HXX

#include "Debugger.hxx"
#include "Expression.hxx"

/**
  @author  B. Watson
  @version $Id: ByteDerefOffsetExpression.hxx,v 1.1 2005-07-19 01:31:36 urchlay Exp $
*/
class ByteDerefOffsetExpression : public Expression
{
  public:
    ByteDerefOffsetExpression(Expression *left, Expression *right);
    int evaluate() { return Debugger::debugger().peek(myLHS->evaluate() + myRHS->evaluate()); }
};

#endif

