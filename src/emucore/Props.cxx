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

#include "Props.hxx"
#include "Variant.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties()
{
  setDefaults();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::load(KeyValueRepository& repo)
{
  setDefaults();

  for(const auto& [key, value] : repo.load())
    set(getPropType(key), value.toString());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Properties::save(KeyValueRepository& repo) const
{
  KVRMap props;

  for(size_t i = 0; i < NUM_PROPS; ++i)
  {
    if(myProperties[i] == ourDefaultProperties[i])
    {
      if(repo.atomic())
        repo.atomic()->remove(ourPropertyNames[i]);
    }
    else
      props[string{ourPropertyNames[i]}] = myProperties[i];
  }

  return repo.save(props);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(PropType key, string_view value)
{
  const auto pos = static_cast<size_t>(key);
  if(pos >= NUM_PROPS)
    return;

  myProperties[pos] = value;
  if(BSPF::equalsIgnoreCase(myProperties[pos], "AUTO-DETECT"))
    myProperties[pos] = "AUTO";

  switch(key)
  {
    case PropType::Cart_Sound:
    case PropType::Cart_Type:
    case PropType::Console_LeftDiff:
    case PropType::Console_RightDiff:
    case PropType::Console_TVType:
    case PropType::Console_SwapPorts:
    case PropType::Controller_Left:
    case PropType::Controller_Left1:
    case PropType::Controller_Left2:
    case PropType::Controller_Right:
    case PropType::Controller_Right1:
    case PropType::Controller_Right2:
    case PropType::Controller_SwapPaddles:
    case PropType::Controller_MouseAxis:
    case PropType::Display_Format:
    case PropType::Display_Phosphor:
      BSPF::toUpperCase(myProperties[pos]);
      break;

    case PropType::Display_PPBlend:
    {
      const int blend = BSPF::stoi(myProperties[pos]);
      if(blend < 0 || blend > 100)
        myProperties[pos] = ourDefaultProperties[pos];
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::reset(PropType key)
{
  const auto pos = static_cast<size_t>(key);
  myProperties[pos] = ourDefaultProperties[pos];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::setDefaults()
{
  for(size_t i = 0; i < NUM_PROPS; ++i)
    myProperties[i] = ourDefaultProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropType Properties::getPropType(string_view name)
{
  // Binary search over the sorted table
  const auto it = std::ranges::lower_bound( // NOLINT(readability-qualified-auto)
    ourNameToPropType, name,
    [](string_view a, string_view b) {
      return BSPF::compareIgnoreCase(a, b) < 0;
    },
    &PropTypeEntry::first);  // projection: extract the key from each element

  if(it != ourNameToPropType.end() &&
     BSPF::compareIgnoreCase(it->first, name) == 0)
    return it->second;

  return PropType::NumTypes;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::print() const
{
  for(size_t i = 0; i < NUM_PROPS; ++i)
  {
    if(i > 0) cout << '|';
    cout << myProperties[i];
  }
  cout << '\n';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::printHeader()
{
  for(size_t i = 0; i < NUM_PROPS; ++i)
  {
    if(i > 0) cout << '|';
    cout << ourPropertyNames[i];
  }
  cout << '\n';
}
