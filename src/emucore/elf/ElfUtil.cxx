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

#include "ElfUtil.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 elfUtil::decode_B_BL(uInt32 opcode)
{
  // nomenclature follows Thumb32 BL / B.W encoding in Arm Architecture Reference

  const uInt16 hw1 = opcode;
  const uInt16 hw2 = opcode >> 16U;

  const uInt8 s = (hw1 >> 10U) & 0x01;
  const uInt8 i1 = ~((hw2 >> 13U) ^ s) & 0x01;
  const uInt8 i2 = ~((hw2 >> 11U) ^ s) & 0x01;
  const uInt32 imm11 = hw2 & 0x7ffU;
  const uInt32 imm10 = hw1 & 0x3ffU;

  Int32 offset = imm11 | (imm10 << 11U) | (i2 << 21U) | (i1 << 22U) | (s << 23U);

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

  const uInt16 hw1 = 0xf000U | (s << 10U) | imm10;
  uInt16 hw2 = 0x9000U | (j1 << 13U) | (j2 << 11U) | imm11;
  if (link) hw2 |= 0x4000U;

  return hw1 | (hw2 << 16U);
}
