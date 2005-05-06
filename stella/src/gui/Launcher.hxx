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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Launcher.hxx,v 1.1 2005-05-06 18:39:00 stephena Exp $
//============================================================================

#ifndef LAUNCHER_HXX
#define LAUNCHER_HXX

class OSystem;

#include "DialogContainer.hxx"

enum {
  kLauncherWidth = 400,
  kLauncherHeight = 300
};

/**
  The base dialog for the ROM launcher in Stella.

  @author  Stephen Anthony
  @version $Id: Launcher.hxx,v 1.1 2005-05-06 18:39:00 stephena Exp $
*/
class Launcher : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    Launcher(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~Launcher();

  public:
    /**
      Updates the basedialog to be of the type defined for this derived class.
    */
    void initialize();
};

#endif
