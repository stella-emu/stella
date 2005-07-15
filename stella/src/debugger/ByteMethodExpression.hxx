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
// $Id: ByteMethodExpression.hxx,v 1.1 2005-07-15 02:19:07 urchlay Exp $
//============================================================================

#ifndef BYTEMETHOD_EXPRESSION_HXX
#define BYTEMETHOD_EXPRESSION_HXX

#include "Debugger.hxx"
#include "CpuDebug.hxx"
#include "Expression.hxx"

/**
  @author  B. Watson
  @version $Id: ByteMethodExpression.hxx,v 1.1 2005-07-15 02:19:07 urchlay Exp $
*/
class ByteMethodExpression : public Expression
{
  public:
    ByteMethodExpression(CPUDEBUG_BYTE_METHOD method);
    int evaluate() { return CALL_CPUDEBUG_METHOD(myMethod); }

  private:
    CPUDEBUG_BYTE_METHOD myMethod;
};

#endif

