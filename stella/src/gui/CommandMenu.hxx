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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CommandMenu.hxx,v 1.4 2007-01-01 18:04:52 stephena Exp $
//============================================================================

#ifndef COMMAND_MENU_HXX
#define COMMAND_MENU_HXX

class Properties;
class OSystem;

#include "DialogContainer.hxx"

/**
  The base dialog for common commands in Stella.

  @author  Stephen Anthony
  @version $Id: CommandMenu.hxx,v 1.4 2007-01-01 18:04:52 stephena Exp $
*/
class CommandMenu : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    CommandMenu(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~CommandMenu();
};

#endif
