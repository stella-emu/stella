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

#include <iomanip>

#include "OSystem.hxx"
#include "Cheat.hxx"
#include "Settings.hxx"
#include "CheetahCheat.hxx"
#include "BankRomCheat.hxx"
#include "RamCheat.hxx"
#include "Vec.hxx"

#include "CheatManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::CheatManager(OSystem& osystem)
  : myOSystem{osystem}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheatManager::add(string_view name, string_view code,
                       bool enable, int idx)
{
  const shared_ptr<Cheat> cheat = createCheat(name, code);
  if(!cheat)
    return false;

  // Delete duplicate entries
  std::erase_if(myCheatList, [&](const auto& c) {
    return c->name() == name || c->code() == code;
  });

  // Add the cheat to the main cheat list
  if(idx == -1)
    myCheatList.push_back(cheat);
  else
    Vec::insertAt(myCheatList, idx, cheat);

  // And enable/disable it (the cheat knows how to enable or disable itself)
  if(enable)
    cheat->enable();
  else
    cheat->disable();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::remove(int idx)
{
  if(static_cast<size_t>(idx) < myCheatList.size())
  {
    // This will also remove it from the per-frame list (if applicable)
    myCheatList[idx]->disable();

    // Then remove it from the cheatlist entirely
    Vec::removeAt(myCheatList, idx);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addPerFrame(string_view name, string_view code, bool enable)
{
  // The actual cheat will always be in the main list; we look there first
  const auto it = std::ranges::find_if(myCheatList,
      [&](const auto& c) { return c->name() == name || c->code() == code; });
  if(it == myCheatList.end())
    return;

  const shared_ptr<Cheat>& cheat = *it;

  // Make sure there are no duplicates
  const auto perFrameIt = std::ranges::find_if(myPerFrameList,
      [&](const auto& c) { return c->code() == cheat->code(); });
  const bool found = perFrameIt != myPerFrameList.end();

  if(enable)
  {
    if(!found)
      myPerFrameList.push_back(cheat);
  }
  else
  {
    if(found)
      myPerFrameList.erase(perFrameIt);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addOneShot(string_view name, string_view code)
{
  // Evaluate this cheat once, and then immediately discard it
  const shared_ptr<Cheat> cheat = createCheat(name, code);
  if(cheat)
    cheat->evaluate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
shared_ptr<Cheat> CheatManager::createCheat(string_view name, string_view code) const
{
  if(!isValidCode(code))
    return nullptr;

  // Create new cheat based on string length
  switch(code.size())
  {
    case 4:  return std::make_shared<RamCheat>(myOSystem, name, code);
    case 6:  return std::make_shared<CheetahCheat>(myOSystem, name, code);
    case 7:  [[fallthrough]];
    case 8:  return std::make_shared<BankRomCheat>(myOSystem, name, code);
    default: return nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::parse(string_view cheats)
{
  // Split on ',' using ranges; each token is a subrange of chars
  for(const auto cheatToken : cheats | std::views::split(','))
  {
    const string_view cheat{cheatToken.begin(), cheatToken.end()};
    if(cheat.empty())
      continue;

    // Split on ':' to get [name, code, enabled] parts
    StringList parts;
    for(const auto partToken : cheat | std::views::split(':'))
    {
      string_view part{partToken.begin(), partToken.end()};
      if(!part.empty())
        parts.emplace_back(part);
    }

    switch(parts.size())
    {
      case 1:
        // Bare code, use it as both name and code
        add(parts[0], parts[0], true);
        break;

      case 2:
        add(parts[0], parts[1], true);
        break;

      case 3:
        add(parts[0], parts[1], parts[2] == "1");
        break;

      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::enable(string_view code, bool enable)
{
  const auto it = std::ranges::find_if(myCheatList,
      [&](const auto& c) { return c->code() == code; });

  if(it != myCheatList.end())
  {
    if(enable) (*it)->enable();
    else       (*it)->disable();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::loadCheatDatabase()
{
  std::stringstream in;
  try         { myOSystem.cheatFile().read(in); }
  catch(...)  { return; }

  string md5, cheat;
  while(in >> std::quoted(md5) >> std::quoted(cheat))
    myCheatMap.emplace(md5, cheat);

  myListIsDirty = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::saveCheatDatabase()
{
  if(!myListIsDirty)
    return;

  std::ostringstream out;
  for(const auto& [md5, cheat]: myCheatMap)
    out << std::quoted(md5) << ' ' << std::quoted(cheat) << '\n';

  try         { myOSystem.cheatFile().write(out); }
  catch(...)  { return; }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::loadCheats(string_view md5sum)
{
  myPerFrameList.clear();
  myCheatList.clear();
  myCurrentCheat = "";

  // Set up any cheatcodes that were on the command line
  // (and remove the key from the settings, so they won't get set again)
  const string& cheats = myOSystem.settings().getString("cheat");
  if(!cheats.empty())
    myOSystem.settings().setValue("cheat", "");

  // Remember the cheats for this ROM
  const auto& iter = myCheatMap.find(md5sum);
  if(iter != myCheatMap.end())
    myCurrentCheat = iter->second;

  // Parse the cheat list, constructing cheats and adding them to the manager
  const string& mapCheats = (iter != myCheatMap.end())
    ? iter->second
    : EmptyString();
  parse(cheats.empty() ? mapCheats : mapCheats + cheats);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::saveCheats(string_view md5sum)
{
  string serialized;
  for(const auto& c : myCheatList)
  {
    if(!serialized.empty()) serialized += ',';
    serialized += std::format("{}:{}:{}", c->name(), c->code(), c->enabled());
  }

  const bool changed = serialized != myCurrentCheat;

  // Only update the list if absolutely necessary
  if(changed)
  {
    const auto iter = myCheatMap.find(md5sum);

    // Erase old entry and add a new one only if it's changed
    if(iter != myCheatMap.end())
      myCheatMap.erase(iter);
    // Add new entry only if there are any cheats defined
    if(!serialized.empty())
      myCheatMap.emplace(md5sum, serialized);
  }

  // Update the dirty flag
  myListIsDirty = myListIsDirty || changed;
  myPerFrameList.clear();
  myCheatList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheatManager::isValidCode(string_view code)
{
  const size_t length = code.length();
  if(length != 4 && length != 6 && length != 7 && length != 8)
    return false;
  return std::ranges::all_of(code, [](unsigned char c) { return std::isxdigit(c); });
}
