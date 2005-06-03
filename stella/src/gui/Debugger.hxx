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
// $Id: Debugger.hxx,v 1.2 2005-06-03 17:52:06 stephena Exp $
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;
class Console;

#include "DialogContainer.hxx"

enum {
  kDebuggerWidth = 511,
  kDebuggerHeight = 383
};

/**
  The base dialog for the ROM launcher in Stella.

  @author  Stephen Anthony
  @version $Id: Debugger.hxx,v 1.2 2005-06-03 17:52:06 stephena Exp $
*/
class Debugger : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    Debugger(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~Debugger();

  public:
    /**
      Updates the basedialog to be of the type defined for this derived class.
    */
    void initialize();

    /**
      Initialize the video subsystem wrt this class.
    */
    void initializeVideo();

    void setConsole(Console* console) { myConsole = console; }

  private:
    Console* myConsole;
};

#endif
