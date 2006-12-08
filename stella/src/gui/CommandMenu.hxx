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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CommandMenu.hxx,v 1.2 2006-12-08 16:49:32 stephena Exp $
//============================================================================

#ifndef COMMAND_MENU_HXX
#define COMMAND_MENU_HXX

class Properties;
class OSystem;

#include "DialogContainer.hxx"

/**
  The base dialog for common commands in Stella.

  @author  Stephen Anthony
  @version $Id: CommandMenu.hxx,v 1.2 2006-12-08 16:49:32 stephena Exp $
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

  public:
    /**
      Updates the basedialog to be of the type defined for this derived class.
    */
    void initialize();
};

#endif
