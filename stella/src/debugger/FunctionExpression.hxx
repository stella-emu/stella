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
// $Id: FunctionExpression.hxx,v 1.1 2005-07-21 03:26:58 urchlay Exp $
//============================================================================

#ifndef FUNCTION_EXPRESSION_HXX
#define FUNCTION_EXPRESSION_HXX

#include "Expression.hxx"
#include "Debugger.hxx"

#include "bspf.hxx"

/**
  @author  Stephen Anthony
  @version $Id: FunctionExpression.hxx,v 1.1 2005-07-21 03:26:58 urchlay Exp $
*/
class FunctionExpression : public Expression
{
  public:
    FunctionExpression(const string& label);
    int evaluate() {
		 Expression *exp = Debugger::debugger().getFunction(myLabel);
		 if(exp)
			 return exp->evaluate();
		 else
			 return 0;
	 }

  private:
    string myLabel;
};

#endif
