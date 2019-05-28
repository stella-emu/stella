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
void KeyMap::add(const Event::Type event, const Mapping& mapping)
{
	myMap[convertMod(mapping)] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(const Event::Type event, const int mode, const int key, const int mod)
{
  add(event, Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const Mapping& mapping)
{
	myMap.erase(convertMod(mapping));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const int mode, const int key, const int mod)
{
  erase(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const Mapping& mapping) const
{
  Mapping m = convertMod(mapping);

  auto find = myMap.find(m);
  if (find != myMap.end())
    return find->second;

  // mapping not found, try without modifiers
  m.mod = StellaMod(0);

  find = myMap.find(m);
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
bool KeyMap::check(const Mapping& mapping) const
{
  auto find = myMap.find(convertMod(mapping));

  return (find != myMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(const int mode, const int key, const int mod) const
{
  return check(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const Mapping& mapping) const
{
  ostringstream buf;
#ifdef BSPF_MACOS
  string control = "Cmd";
  string alt = "Ctrl";
  int ALT = KBDM_GUI;
  int LALT = KBDM_LGUI;
  int RALT = KBDM_RGUI;
#else
  string control = "Ctrl";
  string alt = "Alt";
  int ALT = KBDM_ALT;
  int LALT = KBDM_LALT;
  int RALT = KBDM_RALT;
#endif

  if ((mapping.mod & KBDM_CTRL) == KBDM_CTRL) buf << control;
  else if (mapping.mod & KBDM_LCTRL) buf << "Left " << control;
  else if (mapping.mod & KBDM_RCTRL) buf << "Right " << control;

  if ((mapping.mod & (ALT)) && buf.tellp()) buf << "+";
  if ((mapping.mod & ALT) == ALT) buf << alt;
  else if (mapping.mod & LALT) buf << alt;
  else if (mapping.mod & RALT) buf << alt;

  if ((mapping.mod & (KBDM_SHIFT)) && buf.tellp()) buf << "+";
  if ((mapping.mod & KBDM_SHIFT) == KBDM_SHIFT) buf << "Shift";
  else if (mapping.mod & KBDM_LSHIFT) buf << "Left Shift";
  else if (mapping.mod & KBDM_RSHIFT) buf << "Right Shift";

  if (buf.tellp()) buf << "+";
  buf << StellaKeyName::forKey(mapping.key);

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const int mode, const int key, const int mod) const
{
  return getDesc(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getEventMappingDesc(const Event::Type event, const int mode) const
{
  ostringstream buf;
#ifndef BSPF_MACOS
  string modifier = "Ctrl";
#else
  string control = "Cmd";
#endif

  for (auto item : myMap)
  {
    if (item.second == event && item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << ", ";
      buf << getDesc(item.first);
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
void KeyMap::eraseMode(const int mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseEvent(const Event::Type event, const int mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->second == event && item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::Mapping KeyMap::convertMod(const Mapping& mapping) const
{
  Mapping m = mapping;

  if (m.key >= KBDK_LCTRL && m.key <= KBDK_RGUI)
    // handle solo modifier keys differently
    m.mod = KBDM_NONE;
  else
  {
    // limit to modifiers we want to support
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
    m.mod = StellaMod(m.mod & (KBDM_SHIFT | KBDM_CTRL | KBDM_GUI));
#else
    m.mod = StellaMod(m.mod & (KBDM_SHIFT | KBDM_CTRL | KBDM_ALT));
#endif
  }

  return m;
}
