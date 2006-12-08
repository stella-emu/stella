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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: LoByteExpression.hxx,v 1.3 2006-12-08 16:49:02 stephena Exp $
//============================================================================

#ifndef LOBYTE_EXPRESSION_HXX
#define LOBYTE_EXPRESSION_HXX

#include "bspf.hxx"
#include "Expression.hxx"

/**
  @author  B. Watson
  @version $Id: LoByteExpression.hxx,v 1.3 2006-12-08 16:49:02 stephena Exp $
*/
class LoByteExpression : public Expression
{
  public:
    LoByteExpression(Expression *left);
    uInt16 evaluate() { return 0xff & myLHS->evaluate(); }
};

#endif

