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
// $Id: CheatManager.hxx,v 1.2 2005-11-27 00:17:16 stephena Exp $
//============================================================================

#ifndef CHEAT_MANAGER_HXX
#define CHEAT_MANAGER_HXX

#include <map>

#include "OSystem.hxx"
#include "bspf.hxx"
#include "Array.hxx"

#include "Cheat.hxx"

typedef GUI::Array<Cheat*> CheatList;
typedef map<string,string> CheatCodeMap;

/**
  This class provides an interface for performing all cheat operations
  in Stella.  It is accessible from the OSystem interface, and contains
  the list of all cheats currently in use.

  @author  Stephen Anthony
  @version $Id: CheatManager.hxx,v 1.2 2005-11-27 00:17:16 stephena Exp $
*/
class CheatManager
{
  public:
    CheatManager(OSystem* osystem);
    virtual ~CheatManager();

    /**
      Adds the specified cheat to an internal list.

      @param name    Name of the cheat (not absolutely required)
      @param code    The actual cheatcode (in hex)
      @param enable  Whether to enable this cheat right away

      @return  The cheat (if was created), else NULL.
    */
    const Cheat* add(const string& name, const string& code, bool enable = true);

    /**
      Adds the specified cheat to the internal per-frame list.
      This method doesn't create a new cheat; it just adds/removes
      an already created cheat to the per-frame list.

      @param cheat   The actual cheat object
      @param enable  Add or remove the cheat to the per-frame list
    */
    void addPerFrame(Cheat* cheat, bool enable);

    /**
      Parses a list of cheats and adds/enables each one.

      @param cheats  Comma-separated list of cheats (without any names)
    */
    void parse(const string& cheats);

    /**
      Enable/disabled the cheat specified by the given code.

      @param code    The actual cheatcode to search for
      @param enable  Enable/disable the cheat
    */
    void enable(const string& code, bool enable);

    /**
      Returns the per-frame cheatlist (needed to evaluate cheats each frame)
    */
    const CheatList& perFrame() { return myPerFrameList; }

    /**
      Load all cheats (for all ROMs) from disk to internal database.
    */
    void loadAllCheats();

    /**
      Save all cheats (for all ROMs) in internal database to disk.
    */
    void saveAllCheats();

    /**
      Load cheats for ROM with given MD5sum to cheatlist(s).
    */
    void loadCheats(const string& md5sum);

  private:
    /**
      Clear all per-ROM cheats lists.
    */
    void clear();

  private:
    OSystem* myOSystem;

    CheatList myCheatList;
    CheatList myPerFrameList;

    CheatCodeMap myCheatMap;
    string myCheatFile;
};

#endif
