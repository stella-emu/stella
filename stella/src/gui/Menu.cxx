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
// $Id: Menu.cxx,v 1.3 2005-03-12 01:47:15 stephena Exp $
//============================================================================

#include <SDL.h>

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "OptionsDialog.hxx"
#include "Stack.hxx"
#include "bspf.hxx"
#include "Menu.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Menu::Menu(OSystem* osystem)
    : myOSystem(osystem),
      myOptionsDialog(NULL)
{
  cerr << "Menu::Menu()\n";

  myOSystem->attach(this);  

  // Create the top-level menu
  myOptionsDialog = new OptionsDialog(myOSystem);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Menu::~Menu()
{
  cerr << "Menu::~Menu()\n";

  delete myOptionsDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::draw()
{
  // Draw all the dialogs on the stack
  for(Int32 i = 0; i < myDialogStack.size(); i++)
  {
    myDialogStack[i]->open();
    myDialogStack[i]->drawDialog();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::addDialog(Dialog* d)
{
  myDialogStack.push(d);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::removeDialog()
{
  if(!myDialogStack.empty())
    myDialogStack.pop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::reset()
{
  // Pop all items from the stack, and then add the base menu
  for(Int32 i = 0; i < myDialogStack.size(); i++)
  {
    Dialog* d = myDialogStack.pop();
    d->close();
  }

  myDialogStack.push(myOptionsDialog);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Menu::handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state)
{
  if(myDialogStack.empty())
    return;

  // Send the event to the dialog box on the top of the stack
  Dialog* activeDialog = myDialogStack.top();

  // Convert SDL values to ascii so the ScummVM subsystem can work with it
  // FIXME - convert SDLKey and SDLMod to int values
  uInt16 ascii = 0;
  Int32 keycode = 0, modifiers = 0;

  if(state == 1)
    activeDialog->handleKeyDown(ascii, keycode, modifiers);
  else
    activeDialog->handleKeyUp(ascii, keycode, modifiers);
}
