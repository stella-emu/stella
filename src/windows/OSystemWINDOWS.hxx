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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef OSYSTEM_WINDOWS_HXX
#define OSYSTEM_WINDOWS_HXX

#include "OSystem.hxx"

/**
  This class defines Windows system specific settings.

  @author  Stephen Anthony
  @version $Id$
*/
class OSystemWINDOWS : public OSystem
{
  public:
    /**
      Create a new WINDOWS operating system object
    */
    OSystemWINDOWS();

    /**
      Destructor
    */
    virtual ~OSystemWINDOWS();

  public:
    /**
      Returns the default paths for the snapshot directory.
    */
    string defaultSnapSaveDir() override;
    string defaultSnapLoadDir() override;

  private:
    // Following constructors and assignment operators not supported
    OSystemWINDOWS(const OSystemWINDOWS&) = delete;
    OSystemWINDOWS(OSystemWINDOWS&&) = delete;
    OSystemWINDOWS& operator=(const OSystemWINDOWS&) = delete;
    OSystemWINDOWS& operator=(OSystemWINDOWS&&) = delete;
};

#endif
