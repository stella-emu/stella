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

#include "PhosphorHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhosphorHandler::initialize(bool enable, int blend)
{
  if(myUsePhosphor == enable && std::cmp_equal(blend, myPhosphorBlend))
    return false;

  myUsePhosphor = enable;
  if(blend >= 0 && blend <= 100)
    myPhosphorBlend = static_cast<uInt32>(blend);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhosphorHandler::blendLine(uInt32* FORCE_RESTRICT curr,
                                uInt32* FORCE_RESTRICT prev, uInt32 width) const
{
  // Process the lines byte-wise: plain arithmetic (the division is by a
  // constant) lets the compiler auto-vectorize this loop, unlike the
  // data-dependent LUT lookups it replaces
  auto* c = reinterpret_cast<uInt8*>(curr);
  auto* p = reinterpret_cast<uInt8*>(prev);
  const uInt32 blend = myPhosphorBlend;
  const uInt32 len = width * 4;

  for(uInt32 i = 0; i < len; ++i)
  {
    // Use maximum of current and decayed previous values
    const auto decayed = static_cast<uInt8>(p[i] * blend / 100);
    const uInt8 v = std::max(c[i], decayed);
    c[i] = v;
    p[i] = v;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhosphorHandler::PhosphorMode PhosphorHandler::toPhosphorMode(string_view name)
{
  if(name == VALUE_ALWAYS)
    return PhosphorMode::Always;

  if(name == VALUE_AUTO_ON)
    return PhosphorMode::Auto_on;

  if(name == VALUE_AUTO)
    return PhosphorMode::Auto;

  return PhosphorMode::ByRom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view PhosphorHandler::toPhosphorName(PhosphorMode type)
{
  static constexpr std::array<string_view, PhosphorMode::NumTypes> SETTING_NAMES = {
    VALUE_BYROM, VALUE_ALWAYS, VALUE_AUTO_ON, VALUE_AUTO
  };

  return SETTING_NAMES[type];
}
