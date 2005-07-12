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
// $Id: ConstExpression.hxx,v 1.1 2005-07-12 17:02:35 stephena Exp $
//============================================================================

#ifndef CONST_EXPRESSION_HXX
#define CONST_EXPRESSION_HXX

#include "Expression.hxx"

/**
  This class provides an implementation of an constant expression;
  that is, one that consists solely of a constant integer value.

  @author  Stephen Anthony
  @version $Id: ConstExpression.hxx,v 1.1 2005-07-12 17:02:35 stephena Exp $
*/
class ConstExpression : public Expression
{
  public:
    ConstExpression(const int value);
    int evaluate() { return myValue; }

  private:
    int myValue;
};

#endif
