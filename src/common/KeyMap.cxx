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

#include "KeyMap.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::KeyMap(void)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(const Event::Type event, const Mapping& input)
{
	myMap[convertMod(input)] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(const Event::Type event, const int mode, const int key, const int mod)
{
  add(event, Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const Mapping& input)
{
	myMap.erase(convertMod(input));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const int mode, const int key, const int mod)
{
  erase(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const Mapping& input) const
{
  auto find = myMap.find(convertMod(input));

  if (find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const int mode, const int key, const int mod) const
{
  return get(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getEventMappingDesc(const Event::Type event, const int mode) const
{
  ostringstream buf;
#ifndef BSPF_MACOS
  string modifier = "Ctrl";
#else
  string modifier = "Cmd";
#endif

  for (auto item : myMap)
  {
    if (item.second == event && item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << ", ";
      if (item.first.mod & StellaMod::KBDM_CTRL) buf << modifier << "+";
      if (item.first.mod & StellaMod::KBDM_ALT) buf << "Alt+";
      if (item.first.mod & StellaMod::KBDM_SHIFT) buf << "Shift+";
      buf << StellaKeyName::forKey(item.first.key);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<KeyMap::Mapping> KeyMap::getEventMapping(const Event::Type event, const int mode) const
{
  std::vector<KeyMap::Mapping> map;

  for (auto item : myMap)
    if (item.second == event && item.first.mode == mode)
      map.push_back(item.first);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::saveMapping(const int mode) const
{
  ostringstream buf;

  for (auto item : myMap)
  {
    if (item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << "|";
      buf << item.second << ":" << item.first.key << "," << item.first.mod;
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int KeyMap::loadMapping(string& list, const int mode)
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::replace(list.begin(), list.end(), '|', ' ');
  std::replace(list.begin(), list.end(), ':', ' ');
  std::replace(list.begin(), list.end(), ',', ' ');
  istringstream buf(list);
  int event, key, mod, i = 0;

  while (buf >> event && buf >> key && buf >> mod && ++i)
    add(Event::Type(event), mode, key, mod);

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const Mapping& input) const
{
  return "TODO";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const int mode, const int key, const int mod) const
{
  return getDesc(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseMode(const int mode)
{
  for (auto i = myMap.begin(); i != myMap.end();)
    if (i->first.mode == mode) {
      auto _i = i++;
      erase(_i->first);
    }
    else i++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseEvent(const Event::Type event, const int mode)
{
  for (auto i = myMap.begin(); i != myMap.end();)
    if (i->second == event && i->first.mode == mode) {
      auto _i = i++;
      erase(_i->first);
    }
    else i++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::Mapping KeyMap::convertMod(const Mapping& input) const
{
  Mapping i = input;

  // limit to modifiers we want to support
  i.mod = StellaMod(i.mod & (StellaMod::KBDM_SHIFT | StellaMod::KBDM_ALT | StellaMod::KBDM_CTRL));

  // merge left and right modifiers
  if (i.mod & KBDM_CTRL) i.mod = StellaMod(i.mod | KBDM_CTRL);
  if (i.mod & KBDM_SHIFT) i.mod = StellaMod(i.mod | KBDM_SHIFT);
  if (i.mod & KBDM_ALT) i.mod = StellaMod(i.mod | KBDM_ALT);

  return i;
}
