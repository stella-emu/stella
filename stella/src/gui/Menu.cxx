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
// $Id: Menu.cxx,v 1.8 2005-05-04 21:32:25 stephena Exp $
//============================================================================

#include "Dialog.hxx"
#include "OptionsDialog.hxx"
#include "bspf.hxx"
#include "Menu.hxx"

class Properties;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Menu::Menu(OSystem* osystem)
    : DialogContainer(osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Menu::~Menu()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::initialize()
{
  delete myBaseDialog;
  myBaseDialog = new OptionsDialog(myOSystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::setGameProfile(Properties& props)
{
  ((OptionsDialog*)myBaseDialog)->setGameProfile(props);
}
