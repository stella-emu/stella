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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>
#ifndef M_PI
  #define M_PI 3.14159265358979323846f
#endif

#include "HighPass.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HighPass::HighPass(float cutOffFrequency, float frequency)
  : myLastValueIn(0),
    myLastValueOut(0),
    myAlpha(1.f / (1.f + 2.f*M_PI*cutOffFrequency/frequency))
{}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float HighPass::apply(float valueIn)
{
  float valueOut = myAlpha * (myLastValueOut + valueIn - myLastValueIn);

  myLastValueIn = valueIn;
  myLastValueOut = valueOut;

  return valueOut;
}
