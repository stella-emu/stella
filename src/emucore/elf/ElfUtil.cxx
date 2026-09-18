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

  const uInt32 hw1 = opcode & 0xffffU;
  const uInt32 hw2 = (opcode >> 16U) & 0xffffU;

  const uInt32 s = (hw1 >> 10U) & 0x01U;
  const uInt32 i1 = ~((hw2 >> 13U) ^ s) & 0x01U;
  const uInt32 i2 = ~((hw2 >> 11U) ^ s) & 0x01U;
  const uInt32 imm11 = hw2 & 0x7ffU;
  const uInt32 imm10 = hw1 & 0x3ffU;

  Int32 offset = imm11 | (imm10 << 11U) | (i2 << 21U) | (i1 << 22U) | (s << 23U);

  // sign-extends the 25-bit offset; must stay a signed arithmetic shift
  // NOLINTBEGIN(bugprone-signed-bitwise)
  offset <<= 8;
  offset >>= 7;
  // NOLINTEND(bugprone-signed-bitwise)

  return offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 elfUtil::encode_B_BL(Int32 offset, bool link)
{
  // nomenclature follows Thumb32 BL / B.W encoding in Arm Architecture Reference

  // halves the offset; must stay a signed arithmetic shift
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  offset >>= 1;

  const auto uoffset = U32(offset);

  const uInt32 s = (uoffset >> 23U) & 0x01U;
  const uInt32 j2 = ((~uoffset >> 21U) ^ s) & 0x01U;
  const uInt32 j1 = ((~uoffset >> 22U) ^ s) & 0x01U;
  const uInt32 imm11 = uoffset & 0x7ffU;
  const uInt32 imm10 = (uoffset >> 11U) & 0x3ffU;

  const uInt32 hw1 = 0xf000U | (s << 10U) | imm10;
  uInt32 hw2 = 0x9000U | (j1 << 13U) | (j2 << 11U) | imm11;
  if (link) hw2 |= 0x4000U;

  return hw1 | (hw2 << 16U);
}
