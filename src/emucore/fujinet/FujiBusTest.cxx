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

#include <gtest/gtest.h>

#include "bspf.hxx"
#include "fujibus.hxx"

namespace {

  // fujibus.cxx is vendored from the FujiNet cartridge firmware, where it is
  // a deliberate port of lib/bus/rs232/FujiBusPacket.cpp in the fujinet
  // firmware tree.  It ships its own vectors; running them here is what keeps
  // a re-vendoring honest.
  TEST(FujiBus, selftest)
  {
    EXPECT_TRUE(fujibus_selftest());
  }

  // The checksum is an 8-bit sum with end-around carry fold, taken over the
  // whole packet with the checksum byte itself zeroed -- easy to get subtly
  // wrong, and wrong only for some payloads.  This vector is hand-verified
  // against the C++ encoder and against the shipping Intellivision port.
  TEST(FujiBus, buildsKnownRequest)
  {
    // GET_ADAPTERCONFIG_EXTENDED to the FujiNet device, no parameters
    static constexpr std::array<uInt8, 8> expected = {
      0xC0, 0x70, 0xC4, 0x06, 0x00, 0x3B, 0x00, 0xC0
    };
    std::array<uInt8, 64> out{};

    const size_t n = fujibus_build_request(
      FUJI_DEVICEID_FUJINET, CMD_FUJI_GET_ADAPTERCONFIG_EXTENDED,
      nullptr, 0, nullptr, 0, out.data(), out.size());

    ASSERT_EQ(n, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), out.begin()));
  }

  // Note the mutable buffers below: the parser collapses SLIP escapes in
  // place, so it writes through its `const uint8_t*`.  Handing it read-only
  // storage faults.
  TEST(FujiBus, parsesBareAck)
  {
    std::array<uInt8, 8> frame = {
      0xC0, 0x70, 0x06, 0x06, 0x00, 0x7C, 0x00, 0xC0
    };
    fb_reply_t reply{};

    ASSERT_TRUE(fujibus_parse_reply(frame.data(), frame.size(), &reply));
    EXPECT_EQ(reply.device, FUJI_DEVICEID_FUJINET);
    EXPECT_EQ(reply.command, CMD_FUJI_ACK);
    EXPECT_EQ(reply.data_len, 0);
  }

  TEST(FujiBus, rejectsCorruptedChecksum)
  {
    std::array<uInt8, 8> frame = {
      0xC0, 0x70, 0x06, 0x06, 0x00, 0x7C, 0x00, 0xC0
    };
    fb_reply_t reply{};

    frame[5] ^= 0xFF;  // the checksum byte
    EXPECT_FALSE(fujibus_parse_reply(frame.data(), frame.size(), &reply));
  }
} // namespace
