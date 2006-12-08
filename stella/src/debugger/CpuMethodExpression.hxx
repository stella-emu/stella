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
// $Id: CpuMethodExpression.hxx,v 1.3 2006-12-08 16:48:59 stephena Exp $
//============================================================================

#ifndef CPUMETHOD_EXPRESSION_HXX
#define CPUMETHOD_EXPRESSION_HXX

//#include "Debugger.hxx"
#include "CpuDebug.hxx"
#include "bspf.hxx"
#include "Expression.hxx"

/**
  @author  B. Watson
  @version $Id: CpuMethodExpression.hxx,v 1.3 2006-12-08 16:48:59 stephena Exp $
*/
class CpuMethodExpression : public Expression
{
  public:
    CpuMethodExpression(CPUDEBUG_INT_METHOD method);
    uInt16 evaluate() { return CALL_CPUDEBUG_METHOD(myMethod); }

  private:
    CPUDEBUG_INT_METHOD myMethod;
};

#endif

