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

#include "ControllerMap.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ControllerMap::ControllerMap(void)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::add(const Event::Type event, const ControllerMapping& mapping)
{
  myMap[mapping] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::add(const Event::Type event, const EventMode mode, const int stick, const int button,
  const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir)
{
  add(event, ControllerMapping(mode, stick, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::erase(const ControllerMapping& mapping)
{
  myMap.erase(mapping);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::erase(const EventMode mode, const int stick, const int button,
  const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir)
{
  erase(ControllerMapping(mode, stick, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type ControllerMap::get(const ControllerMapping& mapping) const
{
  auto find = myMap.find(mapping);
  if (find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type ControllerMap::get(const EventMode mode, const int stick, const int button,
  const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const
{
  return get(ControllerMapping(mode, stick, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerMap::check(const ControllerMapping & mapping) const
{
  auto find = myMap.find(mapping);

  return (find != myMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ControllerMap::check(const EventMode mode, const int stick, const int button,
  const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const
{
  return check(ControllerMapping(mode, stick, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerMap::getDesc(const Event::Type event, const ControllerMapping& mapping) const
{
  ostringstream buf;

  buf << "J" << mapping.stick;

  // button description
  if (mapping.button != CTRL_NONE)
    buf << "/B" << mapping.button;

  // axis description
  if (int(mapping.axis) != CTRL_NONE)
  {
    buf << "/A" << mapping.hat;
    switch (mapping.axis)
    {
      case JoyAxis::X: buf << "X"; break;
      case JoyAxis::Y: buf << "Y"; break;
      default:                     break;
    }

    if (Event::isAnalog(event))
      buf << "+|-";
    else if (mapping.adir == JoyDir::NEG)
      buf << "-";
    else
      buf << "+";
  }

  // hat description
  if (mapping.hat != CTRL_NONE)
  {
    buf << "/H" << mapping.hat;
    switch (mapping.hdir)
    {
      case JoyHat::UP:    buf << "/up";    break;
      case JoyHat::DOWN:  buf << "/down";  break;
      case JoyHat::LEFT:  buf << "/left";  break;
      case JoyHat::RIGHT: buf << "/right"; break;
      default:                             break;
    }
  }


  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerMap::getDesc(const Event::Type event, const EventMode mode, const int stick, const int button,
  const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const
{
  return getDesc(event, ControllerMapping(mode, stick, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerMap::getEventMappingDesc(const Event::Type event, const EventMode mode) const
{
  ostringstream buf;

  for (auto item : myMap)
  {
    if (item.second == event && item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << ", ";
      buf << getDesc(event, item.first);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ControllerMap::ControllerMappingArray ControllerMap::getEventMapping(const Event::Type event, const EventMode mode) const
{
  ControllerMappingArray map;

  for (auto item : myMap)
    if (item.second == event && item.first.mode == mode)
      map.push_back(item.first);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ControllerMap::saveMapping(const EventMode mode) const
{
  ostringstream buf;

  for (auto item : myMap)
  {
    if (item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << "|";
      buf << item.second << ":" << item.first.stick << "," << item.first.button << ","
        << int(item.first.axis) << "," << int(item.first.adir) << "," << item.first.hat << "," << int(item.first.hdir);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ControllerMap::loadMapping(string& list, const EventMode mode)
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::replace(list.begin(), list.end(), '|', ' ');
  std::replace(list.begin(), list.end(), ':', ' ');
  std::replace(list.begin(), list.end(), ',', ' ');
  istringstream buf(list);
  int event, stick, button, axis, adir, hat, hdir, i = 0;

  while (buf >> event && buf >> stick && buf >> button
      && buf >> axis && buf >> adir && buf >> hat && buf >> hdir && ++i)
    add(Event::Type(event), EventMode(mode), stick, button, JoyAxis(axis), JoyDir(adir), hat, JoyHat(hdir));

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::eraseMode(const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ControllerMap::eraseEvent(const Event::Type event, const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->second == event && item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}
