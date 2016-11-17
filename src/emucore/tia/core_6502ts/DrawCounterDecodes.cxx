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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

// namespace TIA6502tsCore

#include "DrawCounterDecodes.hxx"

namespace TIA6502tsCore {

const uInt8* const* DrawCounterDecodes::playerDecodes() const {
  return myPlayerDecodes;
}

const uInt8* const* DrawCounterDecodes::missileDecodes() const {
  return myMissileDecodes;
}

DrawCounterDecodes DrawCounterDecodes::myInstance;

DrawCounterDecodes& DrawCounterDecodes::DrawCounterDecodes::get()
{
  return myInstance;
}

DrawCounterDecodes::DrawCounterDecodes()
{
  myDecodes0 = new uInt8[160];
  myDecodes1 = new uInt8[160];
  myDecodes2 = new uInt8[160];
  myDecodes3 = new uInt8[160];
  myDecodes4 = new uInt8[160];
  myDecodes6 = new uInt8[160];
  myDecodesWide = new uInt8[160];

  for (uInt8 *decodes : {myDecodes0, myDecodes1, myDecodes2, myDecodes3, myDecodes4, myDecodes6}) {
    memset(decodes, 0, 160);
    decodes[156] = 1;
  }

  memset(myDecodesWide, 0, 160);
  myDecodesWide[157] = 1;

  myDecodes1[12] = 1;
  myDecodes2[28] = 1;
  myDecodes3[12] = myDecodes3[28] = 1;
  myDecodes4[60] = 1;
  myDecodes6[28] = myDecodes6[60] = 1;

  myPlayerDecodes[0] = myDecodes0;
  myPlayerDecodes[1] = myDecodes1;
  myPlayerDecodes[2] = myDecodes2;
  myPlayerDecodes[3] = myDecodes3;
  myPlayerDecodes[4] = myDecodes4;
  myPlayerDecodes[5] = myDecodesWide;
  myPlayerDecodes[6] = myDecodes6;
  myPlayerDecodes[7] = myDecodesWide;

  myMissileDecodes[0] = myDecodes0;
  myMissileDecodes[1] = myDecodes1;
  myMissileDecodes[2] = myDecodes2;
  myMissileDecodes[3] = myDecodes3;
  myMissileDecodes[4] = myDecodes4;
  myMissileDecodes[5] = myDecodes0;
  myMissileDecodes[6] = myDecodes6;
  myMissileDecodes[7] = myDecodes0;
}

DrawCounterDecodes::~DrawCounterDecodes()
{
  for (uInt8 *decodes : {myDecodes0, myDecodes1, myDecodes2, myDecodes3, myDecodes4, myDecodes6, myDecodesWide}) {
    delete[] decodes;
  }
}

}