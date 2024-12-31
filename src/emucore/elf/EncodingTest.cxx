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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <gtest/gtest.h>

#include "bspf.hxx"
#include "ElfUtil.hxx"

namespace {

  struct Encoding {
    Int32 offset;
    uInt32 opcodeBL;
    uInt32 opcodeB;
  };

  class EncodingTest: public testing::TestWithParam<Encoding> {};

  TEST_P(EncodingTest, OffsetIsEncodedCorrectlyToBL) {
    EXPECT_EQ(elfUtil::encode_B_BL(GetParam().offset - 4, true), GetParam().opcodeBL);
  }

  TEST_P(EncodingTest, OffsetIsEncodedCorrectlyToBW) {
    EXPECT_EQ(elfUtil::encode_B_BL(GetParam().offset - 4, false), GetParam().opcodeB);
  }

  TEST_P(EncodingTest, OffsetIsDecodedCorrectlyFromBL) {
    EXPECT_EQ(elfUtil::decode_B_BL(GetParam().opcodeBL), GetParam().offset - 4);
  }

  TEST_P(EncodingTest, OffsetIsDecodedCorrectlyFromB) {
    EXPECT_EQ(elfUtil::decode_B_BL(GetParam().opcodeB), GetParam().offset - 4);
  }

  INSTANTIATE_TEST_SUITE_P(EncodingSuite, EncodingTest, testing::Values(
    Encoding{.offset = 10, .opcodeBL = 0xf803f000, .opcodeB = 0xb803f000},
    Encoding{.offset = 16777090, .opcodeBL = 0xd7bff3ff, .opcodeB = 0x97bff3ff},
    Encoding{.offset = 8388606, .opcodeBL = 0xf7fdf3ff, .opcodeB = 0xb7fdf3ff},
    Encoding{.offset = -10, .opcodeBL = 0xfff9f7ff, .opcodeB = 0xbff9f7ff},
    Encoding{.offset = -16777090, .opcodeBL = 0xd03df400, .opcodeB = 0x903df400},
    Encoding{.offset = -8388606, .opcodeBL = 0xdffff7ff, .opcodeB = 0x9ffff7ff}
  ));

}
