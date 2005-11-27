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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CheatManager.cxx,v 1.3 2005-11-27 15:48:05 stephena Exp $
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Cheat.hxx"
#include "CheetahCheat.hxx"
#include "BankRomCheat.hxx"
#include "RamCheat.hxx"

#include "CheatManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::CheatManager(OSystem* osystem)
  : myOSystem(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheatManager::~CheatManager()
{
  saveCheatDatabase();
  myCheatMap.clear();
  clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Cheat* CheatManager::add(const string& name, const string& code,
                               bool enable)
{
  for(unsigned int i = 0; i < code.size(); i++)
    if(!isxdigit(code[i]))
      return NULL;

  Cheat* cheat = (Cheat*) NULL;

  // Delete duplicate entries
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->code() == code)
    {
      myCheatList.remove_at(i);
      break;
    }
  }

  // Create new cheat based on string length
  switch(code.size())
  {
    case 4:
      cheat = new RamCheat(myOSystem, name, code);
      break;

    case 6:
      cheat = new CheetahCheat(myOSystem, name, code);
      break;

    case 8:
      cheat = new BankRomCheat(myOSystem, name, code);
      break;
  }

  // Add the cheat to the main cheat list
  if(cheat)
  {
    myCheatList.push_back(cheat);

    // And enable/disable it (the cheat knows how to enable or disable itself)
    if(enable)
      cheat->enable();
    else
      cheat->disable();
  }

  return cheat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::addPerFrame(Cheat* cheat, bool enable)
{
  if(!cheat)
    return;

  // Make sure there are no duplicates
  bool found = false;
  unsigned int i;
  for(i = 0; i < myPerFrameList.size(); i++)
  {
    if(myPerFrameList[i]->code() == cheat->code())
    {
      found = true;
      break;
    }
  }

  if(enable)
  {
    if(!found)
      myPerFrameList.push_back(cheat);
  }
  else
  {
    if(found)
      myPerFrameList.remove_at(i);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::parse(const string& cheats)
{
  cerr << "parsing cheats: " << cheats << endl;

  StringList s;
  string::size_type lastPos = cheats.find_first_not_of(",", 0);
  string::size_type pos     = cheats.find_first_of(",", lastPos);
  string cheat, name, code;

  // Split string by comma, getting each cheat
  while(string::npos != pos || string::npos != lastPos)
  {
    // Get the next cheat
    cheat = cheats.substr(lastPos, pos - lastPos);

    // Split cheat by colon, separating each part
    string::size_type lastColonPos = cheat.find_first_not_of(":", 0);
    string::size_type colonPos     = cheat.find_first_of(":", lastColonPos);
    while(string::npos != colonPos || string::npos != lastColonPos)
    {
      s.push_back(cheat.substr(lastColonPos, colonPos - lastColonPos));
      lastColonPos = cheat.find_first_not_of(":", colonPos);
      colonPos     = cheat.find_first_of(":", lastColonPos);
    }

    // Account for variable number of items specified for cheat
    switch(s.size())
    {
      case 1:
        name = s[0];
        code = name;
        add(name, code, true);
        break;

      case 2:
        name = s[0];
        code = s[1];
        add(name, code, true);
        break;

      case 3:
        name = s[0];
        code = s[1];
        add(name, code, s[2] == "1");
        break;
    }
    s.clear();

    lastPos = cheats.find_first_not_of(",", pos);
    pos     = cheats.find_first_of(",", lastPos);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::enable(const string& code, bool enable)
{
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    if(myCheatList[i]->code() == code)
    {
      if(enable)
        myCheatList[i]->enable();
      else
        myCheatList[i]->disable();
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::loadCheatDatabase()
{
  string cheatfile = myOSystem->baseDir() + BSPF_PATH_SEPARATOR + "stella.cht";
  ifstream in(cheatfile.c_str(), ios::in);
  if(!in)
    return;

  string line, md5, cheat;
  string::size_type one, two, three, four;

  // Loop reading cheats
  while(getline(in, line))
  {
    if(line.length() == 0)
      continue;

    one = line.find("\"", 0);
    two = line.find("\"", one + 1);
    three = line.find("\"", two + 1);
    four = line.find("\"", three + 1);

    // Invalid line if it doesn't contain 4 quotes
    if((one == string::npos) || (two == string::npos) ||
       (three == string::npos) || (four == string::npos))
      break;

    // Otherwise get the ms5sum and associated cheats
    md5   = line.substr(one + 1, two - one - 1);
    cheat = line.substr(three + 1, four - three - 1);

    myCheatMap.insert(make_pair(md5, cheat));
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::saveCheatDatabase()
{
  string cheatfile = myOSystem->baseDir() + BSPF_PATH_SEPARATOR + "stella.cht";
  ofstream out(cheatfile.c_str(), ios::out);
  if(!out)
    return;

  CheatCodeMap::iterator iter;
  for(iter = myCheatMap.begin(); iter != myCheatMap.end(); ++iter)
    out << "\"" << iter->first << "\" "
        << "\"" << iter->second << "\""
        << endl;

  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::loadCheats(const string& md5sum)
{
  clear();

  // Set up any cheatcodes that was on the command line
  // (and remove the key from the settings, so they won't get set again)
  string cheats = myOSystem->settings().getString("cheat");
  if(cheats != "")
    myOSystem->settings().setString("cheat", "", false);

  CheatCodeMap::iterator iter = myCheatMap.find(md5sum);
  if(iter == myCheatMap.end() && cheats == "")
    return;

  parse(iter->second + cheats);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::saveCheats(const string& md5sum)
{
  ostringstream cheats;
  for(unsigned int i = 0; i < myCheatList.size(); i++)
  {
    cheats << myCheatList[i]->name() << ":"
           << myCheatList[i]->code() << ":"
           << myCheatList[i]->enabled();
    if(i+1 < myCheatList.size())
      cheats << ",";
  }

  CheatCodeMap::iterator iter = myCheatMap.find(md5sum);

  // Erase old entry
  if(iter != myCheatMap.end())
    myCheatMap.erase(iter);

  // Add new entry only if there are any cheats defined
  if(cheats.str() != "")
    myCheatMap.insert(make_pair(md5sum, cheats.str()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheatManager::clear()
{
  // Don't delete the items from per-frame list, since it will be done in
  // the following loop
  myPerFrameList.clear();

  for(unsigned int i = 0; i < myCheatList.size(); i++)
    delete myCheatList[i];
  myCheatList.clear();
}
