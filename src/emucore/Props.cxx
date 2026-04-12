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
void Properties::print() const
{
  cout << std::format(
    "{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}\n",
    get(PropType::Cart_MD5),
    get(PropType::Cart_Name),
    get(PropType::Cart_Manufacturer),
    get(PropType::Cart_ModelNo),
    get(PropType::Cart_Note),
    get(PropType::Cart_Rarity),
    get(PropType::Cart_Sound),
    get(PropType::Cart_StartBank),
    get(PropType::Cart_Type),
    get(PropType::Cart_Highscore),
    get(PropType::Cart_Url),
    get(PropType::Console_LeftDiff),
    get(PropType::Console_RightDiff),
    get(PropType::Console_TVType),
    get(PropType::Console_SwapPorts),
    get(PropType::Controller_Left),
    get(PropType::Controller_Left1),
    get(PropType::Controller_Left2),
    get(PropType::Controller_Right),
    get(PropType::Controller_Right1),
    get(PropType::Controller_Right2),
    get(PropType::Controller_SwapPaddles),
    get(PropType::Controller_PaddlesXCenter),
    get(PropType::Controller_PaddlesYCenter),
    get(PropType::Controller_MouseAxis),
    get(PropType::Display_Format),
    get(PropType::Display_VCenter),
    get(PropType::Display_Phosphor),
    get(PropType::Display_PPBlend),
    get(PropType::Bezel_Name)
  );
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
void Properties::printHeader()
{
  cout <<
    "Cart_MD5|"
    "Cart_Name|"
    "Cart_Manufacturer|"
    "Cart_ModelNo|"
    "Cart_Note|"
    "Cart_Rarity|"
    "Cart_Sound|"
    "Cart_StartBank|"
    "Cart_Type|"
    "Cart_Highscore|"
    "Cart_Url|"
    "Console_LeftDiff|"
    "Console_RightDiff|"
    "Console_TVType|"
    "Console_SwapPorts|"
    "Controller_Left|"
    "Controller_Left1|"
    "Controller_Left2|"
    "Controller_Right|"
    "Controller_Right1|"
    "Controller_Right2|"
    "Controller_SwapPaddles|"
    "Controller_PaddlesXCenter|"
    "Controller_PaddlesYCenter|"
    "Controller_MouseAxis|"
    "Display_Format|"
    "Display_VCenter|"
    "Display_Phosphor|"
    "Display_PPBlend|"
    "Bezel_Name\n";
}
