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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "JoyMap.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(const Event::Type event, const JoyMapping& mapping)
{
  myMap[mapping] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(const Event::Type event, const EventMode mode, const int button,
                 const JoyAxis axis, const JoyDir adir,
                 const int hat, const JoyHatDir hdir)
{
  add(event, JoyMapping(mode, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(const Event::Type event, const EventMode mode, const int button,
                 const int hat, const JoyHatDir hdir)
{
  add(event, JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(const JoyMapping& mapping)
{
  myMap.erase(mapping);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(const EventMode mode, const int button,
                   const JoyAxis axis, const JoyDir adir)
{
  erase(JoyMapping(mode, button, axis, adir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(const EventMode mode, const int button,
                   const int hat, const JoyHatDir hdir)
{
  erase(JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(const JoyMapping& mapping) const
{
  auto find = myMap.find(mapping);
  if (find != myMap.end())
    return find->second;

  // try without button as modifier
  JoyMapping m = mapping;

  m.button = JOY_CTRL_NONE;

  find = myMap.find(m);
  if (find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(const EventMode mode, const int button,
                        const JoyAxis axis, const JoyDir adir) const
{
  return get(JoyMapping(mode, button, axis, adir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(const EventMode mode, const int button,
                        const int hat, const JoyHatDir hdir) const
{
  return get(JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JoyMap::check(const JoyMapping & mapping) const
{
  auto find = myMap.find(mapping);

  return (find != myMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JoyMap::check(const EventMode mode, const int button,
                   const JoyAxis axis, const JoyDir adir,
                   const int hat, const JoyHatDir hdir) const
{
  return check(JoyMapping(mode, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string JoyMap::getDesc(const Event::Type event, const JoyMapping& mapping) const
{
  ostringstream buf;

  // button description
  if (mapping.button != JOY_CTRL_NONE)
    buf << "/B" << mapping.button;

  // axis description
  if (mapping.axis != JoyAxis::NONE)
  {
    buf << "/A";
    switch (mapping.axis)
    {
      case JoyAxis::X: buf << "X"; break;
      case JoyAxis::Y: buf << "Y"; break;
      case JoyAxis::Z: buf << "Z"; break;
      default:         buf << int(mapping.axis); break;
    }

    if (Event::isAnalog(event))
      buf << "+|-";
    else if (mapping.adir == JoyDir::NEG)
      buf << "-";
    else
      buf << "+";
  }

  // hat description
  if (mapping.hat != JOY_CTRL_NONE)
  {
    buf << "/H" << mapping.hat;
    switch (mapping.hdir)
    {
      case JoyHatDir::UP:    buf << "Y+"; break;
      case JoyHatDir::DOWN:  buf << "Y-"; break;
      case JoyHatDir::LEFT:  buf << "X-"; break;
      case JoyHatDir::RIGHT: buf << "X+"; break;
      default:                            break;
    }
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string JoyMap::getEventMappingDesc(int stick, const Event::Type event, const EventMode mode) const
{
  ostringstream buf;

  for (auto item : myMap)
  {
    if (item.second == event && item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << ", ";
      buf << "J" << stick << getDesc(event, item.first);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoyMap::JoyMappingArray JoyMap::getEventMapping(const Event::Type event, const EventMode mode) const
{
  JoyMappingArray map;

  for (auto item : myMap)
    if (item.second == event && item.first.mode == mode)
      map.push_back(item.first);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string JoyMap::saveMapping(const EventMode mode) const
{
  using MapType = std::pair<JoyMapping, Event::Type>;
  std::vector<MapType> sortedMap(myMap.begin(), myMap.end());

  std::sort(sortedMap.begin(), sortedMap.end(),
    [](const MapType& a, const MapType& b)
    {
      // Event::Type first
      if(a.second != b.second)
        return a.second < b.second;

      if(a.first.button != b.first.button)
        return a.first.button < b.first.button;

      if(a.first.axis != b.first.axis)
        return a.first.axis < b.first.axis;

      if(a.first.adir != b.first.adir)
        return a.first.adir < b.first.adir;

      if(a.first.hat != b.first.hat)
        return a.first.hat < b.first.hat;

      return a.first.hdir < b.first.hdir;
    }
  );

  ostringstream buf;

  for (auto item : sortedMap)
  {
    if (item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << "|";
      buf << item.second << ":" << item.first.button << ","
        << int(item.first.axis) << "," << int(item.first.adir) << ","
        << item.first.hat << "," << int(item.first.hdir);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int JoyMap::loadMapping(string& list, const EventMode mode)
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::replace(list.begin(), list.end(), '|', ' ');
  std::replace(list.begin(), list.end(), ':', ' ');
  std::replace(list.begin(), list.end(), ',', ' ');
  istringstream buf(list);
  int event, button, axis, adir, hat, hdir, i = 0;

  while (buf >> event && buf >> button
         && buf >> axis && buf >> adir
         && buf >> hat && buf >> hdir && ++i)
    add(Event::Type(event), EventMode(mode), button, JoyAxis(axis), JoyDir(adir), hat, JoyHatDir(hdir));

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::eraseMode(const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::eraseEvent(const Event::Type event, const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->second == event && item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}
