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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Console.cxx,v 1.38 2004-08-12 23:54:36 stephena Exp $
//============================================================================

#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "Booster.hxx"
#include "Cart.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Driving.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
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
#include "Settings.hxx" 
#include "Sound.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "FrameBuffer.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, const char* filename,
    Settings& settings, PropertiesSet& propertiesSet, 
    FrameBuffer& framebuffer, Sound& sound)
    : mySettings(settings),
      myPropSet(propertiesSet),
      myFrameBuffer(framebuffer),
      mySound(sound)
{
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;
  myEvent = 0;

  // Create an event handler which will collect and dispatch events
  myEventHandler = new EventHandler(this);
  myEvent = myEventHandler->event();

  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Search for the properties based on MD5
  myPropSet.getMD5(md5, myProperties);

  // Make sure the MD5 value of the cartridge is set in the properties
  if(myProperties.get("Cartridge.MD5") == "")
  {
    myProperties.set("Cartridge.MD5", md5);
  }

  // Setup the controllers based on properties
  string left = myProperties.get("Controller.Left");
  string right = myProperties.get("Controller.Right");

  // Construct left controller
  if(left == "Booster-Grip")
  {
    myControllers[0] = new BoosterGrip(Controller::Left, *myEvent);
  }
  else if(left == "Driving")
  {
    myControllers[0] = new Driving(Controller::Left, *myEvent);
  }
  else if((left == "Keyboard") || (left == "Keypad"))
  {
    myControllers[0] = new Keyboard(Controller::Left, *myEvent);
  }
  else if(left == "Paddles")
  {
    myControllers[0] = new Paddles(Controller::Left, *myEvent);
  }
  else
  {
    myControllers[0] = new Joystick(Controller::Left, *myEvent);
  }
  
  // Construct right controller
  if(right == "Booster-Grip")
  {
    myControllers[1] = new BoosterGrip(Controller::Right, *myEvent);
  }
  else if(right == "Driving")
  {
    myControllers[1] = new Driving(Controller::Right, *myEvent);
  }
  else if((right == "Keyboard") || (right == "Keypad"))
  {
    myControllers[1] = new Keyboard(Controller::Right, *myEvent);
  }
  else if(right == "Paddles")
  {
    myControllers[1] = new Paddles(Controller::Right, *myEvent);
  }
  else
  {
    myControllers[1] = new Joystick(Controller::Right, *myEvent);
  }

  // Create switches for the console
  mySwitches = new Switches(*myEvent, myProperties);

  // Now, we can construct the system and components
  mySystem = new System(13, 6);

  M6502* m6502;
  if(myProperties.get("Emulation.CPU") == "Low")
  {
    m6502 = new M6502Low(1);
  }
  else
  {
    m6502 = new M6502High(1);
  }

  M6532* m6532 = new M6532(*this);
  TIA* tia = new TIA(*this, mySound);
  Cartridge* cartridge = Cartridge::create(image, size, myProperties);

  mySystem->attach(m6502);
  mySystem->attach(m6532);
  mySystem->attach(tia);
  mySystem->attach(cartridge);

  // Remember what my media source is
  myMediaSource = tia;

  // Reset, the system to its power-on state
  mySystem->reset();

  // Set the correct framerate based on the format of the ROM
  // This can be overridden by the '-framerate' option
  myFrameRate = 60;
  if(mySettings.getInt("framerate") > 0)
    myFrameRate = mySettings.getInt("framerate");
//  else if(myProperties.get("Display.Format") == "NTSC")
//    myFrameRate = 60;
//  else if(myProperties.get("Display.Format") == "PAL")
//    myFrameRate = 50;
//  mySettings.setInt("framerate", myFrameRate, false);
  mySettings.setInt("framerate", myFrameRate);

  // Initialize the framebuffer interface.
  // This must be done *after* a reset, since it needs updated values.
  myFrameBuffer.initDisplay(this, myMediaSource);

  // Initialize the sound interface.
  uInt32 soundFrameRate = (myProperties.get("Display.Format") == "PAL") ? 50 : 60;
  mySound.init(this, myMediaSource, mySystem, soundFrameRate);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
    : mySettings(console.mySettings),
      myPropSet(console.myPropSet),
      myFrameBuffer(console.myFrameBuffer),
      mySound(console.mySound)
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
  delete myEventHandler;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::update()
{
  myFrameBuffer.update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& Console::properties() const
{
  return myProperties;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings& Console::settings() const
{
  return mySettings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameBuffer& Console::frameBuffer() const
{
  return myFrameBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::frameRate() const
{
  return myFrameRate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound& Console::sound() const
{
  return mySound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console& Console::operator = (const Console&)
{
  // TODO: Write this method
  assert(false);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleFormat()
{
  string format = myProperties.get("Display.Format");

  if(format == "NTSC")
  {
    myProperties.set("Display.Format", "PAL");
    mySystem->reset();
    myFrameBuffer.showMessage("PAL Mode");
  }
  else if(format == "PAL")
  {
    myProperties.set("Display.Format", "NTSC");
    mySystem->reset();
    myFrameBuffer.showMessage("NTSC Mode");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePalette()
{
  string type = mySettings.getString("palette");

  if(type == "standard")  // switch to original
  {
    myFrameBuffer.showMessage("Original Stella colors");
    mySettings.setString("palette", "original");
  }
  else if(type == "original")  // switch to z26
  {
    myFrameBuffer.showMessage("Z26 colors");
    mySettings.setString("palette", "z26");
  }
  else if(type == "z26")  // switch to standard
  {
    myFrameBuffer.showMessage("Standard Stella colors");
    mySettings.setString("palette", "standard");
  }
  else  // switch to standard mode if we get this far
  {
    myFrameBuffer.showMessage("Standard Stella colors");
    mySettings.setString("palette", "standard");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::saveProperties(string filename, bool merge)
{
  // Merge the current properties into the PropertiesSet file
  if(merge)
  {
    if(myPropSet.merge(myProperties, filename))
      myFrameBuffer.showMessage("Properties merged");
    else
      myFrameBuffer.showMessage("Properties not merged");
  }
  else  // Save to the specified file directly
  {
    ofstream out(filename.c_str(), ios::out);

    if(out && out.is_open())
    {
      myProperties.save(out);
      out.close();
      myFrameBuffer.showMessage("Properties saved");
    }
    else
    {
      myFrameBuffer.showMessage("Properties not saved");
    }
  }
}

#ifdef DEVELOPER_SUPPORT
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
      myFrameBuffer.showMessage("XStart at maximum");
      return;
    }
    else if((width + xstart) > 160)
    {
      myFrameBuffer.showMessage("XStart no effect");
      return;
    }
  }
  else if(direction == 0)  // decrease XStart
  {
    xstart -= 4;
    if(xstart < 0)
    {
      myFrameBuffer.showMessage("XStart at minimum");
      return;
    }
  }

  strval << xstart;
  myProperties.set("Display.XStart", strval.str());
  mySystem->reset();

  message = "XStart ";
  message += strval.str();
  myFrameBuffer.showMessage(message);
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
      myFrameBuffer.showMessage("YStart at maximum");
      return;
    }
  }
  else if(direction == 0)  // decrease YStart
  {
    ystart--;
    if(ystart < 0)
    {
      myFrameBuffer.showMessage("YStart at minimum");
      return;
    }
  }

  strval << ystart;
  myProperties.set("Display.YStart", strval.str());
  mySystem->reset();

  message = "YStart ";
  message += strval.str();
  myFrameBuffer.showMessage(message);
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
      myFrameBuffer.showMessage("Width at maximum");
      return;
    }
    else if((width + xstart) > 160)
    {
      myFrameBuffer.showMessage("Width no effect");
      return;
    }
  }
  else if(direction == 0)  // decrease Width
  {
    width -= 4;
    if(width < 80)
    {
      myFrameBuffer.showMessage("Width at minimum");
      return;
    }
  }

  strval << width;
  myProperties.set("Display.Width", strval.str());
  mySystem->reset();

  message = "Width ";
  message += strval.str();
  myFrameBuffer.showMessage(message);
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
      myFrameBuffer.showMessage("Height at maximum");
      return;
    }
  }
  else if(direction == 0)  // decrease Height
  {
    height--;
    if(height < 100)
    {
      myFrameBuffer.showMessage("Height at minimum");
      return;
    }
  }

  strval << height;
  myProperties.set("Display.Height", strval.str());
  mySystem->reset();

  message = "Height ";
  message += strval.str();
  myFrameBuffer.showMessage(message);
}
#endif
