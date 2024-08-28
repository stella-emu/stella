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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "ElfUtil.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 elfUtil::decode_B_BL(uInt32 opcode)
{
  // nomenclature follows Thumb32 BL / B.W encoding in Arm Architecture Reference

  const uInt16 hw1 = opcode;
  const uInt16 hw2 = opcode >> 16;

  const uInt8 s = (hw1 >> 10) & 0x01;
  const uInt8 i1 = ~((hw2 >> 13) ^ s) & 0x01;
  const uInt8 i2 = ~((hw2 >> 11) ^ s) & 0x01;
  const uInt32 imm11 = hw2 & 0x7ff;
  const uInt32 imm10 = hw1 & 0x3ff;

  Int32 offset = imm11 | (imm10 << 11) | (i2 << 21) | (i1 << 22) | (s << 23);

  offset <<= 8;
  offset >>= 7;

  return offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 elfUtil::encode_B_BL(Int32 offset, bool link)
{
  // nomenclature follows Thumb32 BL / B.W encoding in Arm Architecture Reference

  offset >>= 1;

  const uInt8 s = (offset >> 23) & 0x01;
  const uInt8 j2 = ((~offset >> 21) ^ s) & 0x01;
  const uInt8 j1 = ((~offset >> 22) ^ s) & 0x01;
  const uInt32 imm11 = offset & 0x7ff;
  const uInt32 imm10 = (offset >> 11) & 0x3ff;

  const uInt16 hw1 = 0xf000 | (s << 10) | imm10;
  uInt16 hw2 = 0x9000 | (j1 << 13) | (j2 << 11) | imm11;
  if (link) hw2 |= 0x4000;

  return hw1 | (hw2 << 16);
}
