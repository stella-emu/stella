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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Console.cxx,v 1.6 2002-11-10 19:05:57 stephena Exp $
//============================================================================

#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "Booster.hxx"
#include "Cart.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Driving.hxx"
#include "Event.hxx"
#include "Joystick.hxx"
#include "Keyboard.hxx"
#include "M6502Low.hxx"
#include "M6502Hi.hxx"
#include "M6532.hxx"
#include "MD5.hxx"
#include "MediaSrc.hxx"
#include "Paddles.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "TIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, const char* filename,
    const Event& event, PropertiesSet& propertiesSet, uInt32 sampleRate,
    const Properties* userDefinedProperties)
    : myEvent(event)
{
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;

  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Search for the properties based on MD5
  propertiesSet.getMD5(md5, myProperties);

  // Merge any user-defined properties
  if(userDefinedProperties != 0)
  {
    myProperties.merge(*userDefinedProperties);
  }

  // Setup the controllers based on properties
  string left = myProperties.get("Controller.Left");
  string right = myProperties.get("Controller.Right");

  // Construct left controller
  if(left == "Booster-Grip")
  {
    myControllers[0] = new BoosterGrip(Controller::Left, myEvent);
  }
  else if(left == "Driving")
  {
    myControllers[0] = new Driving(Controller::Left, myEvent);
  }
  else if((left == "Keyboard") || (left == "Keypad"))
  {
    myControllers[0] = new Keyboard(Controller::Left, myEvent);
  }
  else if(left == "Paddles")
  {
    myControllers[0] = new Paddles(Controller::Left, myEvent);
  }
  else
  {
    myControllers[0] = new Joystick(Controller::Left, myEvent);
  }
  
  // Construct right controller
  if(right == "Booster-Grip")
  {
    myControllers[1] = new BoosterGrip(Controller::Right, myEvent);
  }
  else if(right == "Driving")
  {
    myControllers[1] = new Driving(Controller::Right, myEvent);
  }
  else if((right == "Keyboard") || (right == "Keypad"))
  {
    myControllers[1] = new Keyboard(Controller::Right, myEvent);
  }
  else if(right == "Paddles")
  {
    myControllers[1] = new Paddles(Controller::Right, myEvent);
  }
  else
  {
    myControllers[1] = new Joystick(Controller::Right, myEvent);
  }

  // Create switches for the console
  mySwitches = new Switches(myEvent, myProperties);

  // Now, we can construct the system and components
  mySystem = new System(13, 6);

  M6502* m6502;
  if((myProperties.get("Emulation.CPU") == "High") ||
      ((myProperties.get("Emulation.CPU") == "Auto-detect") && !(size % 8448)))
  {
    m6502 = new M6502High(1);
  }
  else
  {
    m6502 = new M6502Low(1);
  }

  M6532* m6532 = new M6532(*this);
  TIA* tia = new TIA(*this, sampleRate);
  Cartridge* cartridge = Cartridge::create(image, size, myProperties);

  mySystem->attach(m6502);
  mySystem->attach(m6532);
  mySystem->attach(tia);
  mySystem->attach(cartridge);

  // Remember what my media source is
  myMediaSource = tia;

  // Reset, the system to its power-on state
  mySystem->reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
    : myEvent(console.myEvent)
{
  // TODO: Write this method
  assert(false);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::~Console()
{
  delete mySystem;
  delete mySwitches;
  delete myControllers[0];
  delete myControllers[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& Console::properties() const
{
  return myProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console& Console::operator = (const Console&)
{
  // TODO: Write this method
  assert(false);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& Console::defaultProperties()
{
  // Make sure the <key,value> pairs are in the default properties object
  ourDefaultProperties.set("Cartridge.Filename", "");
  ourDefaultProperties.set("Cartridge.MD5", "");
  ourDefaultProperties.set("Cartridge.Manufacturer", "");
  ourDefaultProperties.set("Cartridge.ModelNo", "");
  ourDefaultProperties.set("Cartridge.Name", "Untitled");
  ourDefaultProperties.set("Cartridge.Note", "");
  ourDefaultProperties.set("Cartridge.Rarity", "");
  ourDefaultProperties.set("Cartridge.Type", "Auto-detect");

  ourDefaultProperties.set("Console.LeftDifficulty", "B");
  ourDefaultProperties.set("Console.RightDifficulty", "B");
  ourDefaultProperties.set("Console.TelevisionType", "Color");

  ourDefaultProperties.set("Controller.Left", "Joystick");
  ourDefaultProperties.set("Controller.Right", "Joystick");

  ourDefaultProperties.set("Display.Format", "NTSC");
  ourDefaultProperties.set("Display.XStart", "0");
  ourDefaultProperties.set("Display.Width", "160");
  ourDefaultProperties.set("Display.YStart", "38");
  ourDefaultProperties.set("Display.Height", "210");

  ourDefaultProperties.set("Emulation.CPU", "Auto-detect");
  ourDefaultProperties.set("Emulation.HmoveBlanks", "Yes");

  return ourDefaultProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties Console::ourDefaultProperties;


#ifdef DEVELOPER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleFormat()
{
  string format = myProperties.get("Display.Format");
  string message;

  if(format == "NTSC")
  {
    message = "PAL Mode";
    myMediaSource->showMessage(message, 120);
    myProperties.set("Display.Format", "PAL");
  }
  else if(format == "PAL")
  {
    message = "NTSC Mode";
    myMediaSource->showMessage(message, 120);
    myProperties.set("Display.Format", "NTSC");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeXStart(const uInt32 direction)
{
  Int32 xstart = atoi(myProperties.get("Display.XStart").c_str());
  uInt32 width = atoi(myProperties.get("Display.Width").c_str());
  ostringstream strval;
  string message;

  if(direction == 1)    // increase XStart
  {
    xstart += 4;
    if(xstart > 80)
    {
      message = "XStart at maximum";
      myMediaSource->showMessage(message, 120);
      return;
    }
    else if((width + xstart) > 160)
    {
      message = "XStart no effect";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }
  else if(direction == 0)  // decrease XStart
  {
    xstart -= 4;
    if(xstart < 0)
    {
      message = "XStart at minimum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }

  strval << xstart;
  myProperties.set("Display.XStart", strval.str());
  mySystem->reset();

  message = "XStart ";
  message += strval.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeYStart(const uInt32 direction)
{
  Int32 ystart = atoi(myProperties.get("Display.YStart").c_str());
  ostringstream strval;
  string message;

  if(direction == 1)    // increase YStart
  {
    ystart++;
    if(ystart > 64)
    {
      message = "YStart at maximum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }
  else if(direction == 0)  // decrease YStart
  {
    ystart--;
    if(ystart < 0)
    {
      message = "YStart at minimum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }

  strval << ystart;
  myProperties.set("Display.YStart", strval.str());
  mySystem->reset();

  message = "YStart ";
  message += strval.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeWidth(const uInt32 direction)
{
  uInt32 xstart = atoi(myProperties.get("Display.XStart").c_str());
  Int32 width   = atoi(myProperties.get("Display.Width").c_str());
  ostringstream strval;
  string message;

  if(direction == 1)    // increase Width
  {
    width += 4;
    if((width > 160) || ((width % 4) != 0))
    {
      message = "Width at maximum";
      myMediaSource->showMessage(message, 120);
      return;
    }
    else if((width + xstart) > 160)
    {
      message = "Width no effect";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }
  else if(direction == 0)  // decrease Width
  {
    width -= 4;
    if(width < 80)
    {
      message = "Width at minimum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }

  strval << width;
  myProperties.set("Display.Width", strval.str());
  mySystem->reset();

  message = "Width ";
  message += strval.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeHeight(const uInt32 direction)
{
  Int32 height = atoi(myProperties.get("Display.Height").c_str());
  ostringstream strval;
  string message;

  if(direction == 1)    // increase Height
  {
    height++;
    if(height > 256)
    {
      message = "Height at maximum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }
  else if(direction == 0)  // decrease Height
  {
    height--;
    if(height < 100)
    {
      message = "Height at minimum";
      myMediaSource->showMessage(message, 120);
      return;
    }
  }

  strval << height;
  myProperties.set("Display.Height", strval.str());
  mySystem->reset();

  message = "Height ";
  message += strval.str();
  myMediaSource->showMessage(message, 120);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::saveProperties(string& filename)
{
  string message;

  // Replace all spaces in filename with underscores
  replace(filename.begin(), filename.end(), ' ', '_');
  ofstream out(filename.c_str(), ios::out);

  if(out && out.is_open())
  {
    myProperties.save(out);
    out.close();
    message = "Properties saved";
    myMediaSource->showMessage(message, 120);
  }
  else
  {
    message = "Properties not saved";
    myMediaSource->showMessage(message, 120);
  }
}
#endif
