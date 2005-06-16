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
// $Id: Console.cxx,v 1.57 2005-06-16 12:28:53 stephena Exp $
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
#include "OSystem.hxx"
#include "Menu.hxx"
#include "Debugger.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, OSystem* osystem)
    : myOSystem(osystem)
{
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;
  myEvent = 0;

  // Attach the event subsystem to the current console
  myEvent = myOSystem->eventHandler().event();

  // Get the MD5 message-digest for the ROM image
  string md5 = MD5(image, size);

  // Search for the properties based on MD5
  myOSystem->propSet().getMD5(md5, myProperties);

  // Make sure the MD5 value of the cartridge is set in the properties
  if(myProperties.get("Cartridge.MD5") == "")
    myProperties.set("Cartridge.MD5", md5);

#ifdef DEVELOPER_SUPPORT
  // A developer can override properties from the commandline
  setDeveloperProperties();
#endif

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
  m6502->attach(myOSystem->debugger());

  M6532* m6532 = new M6532(*this);
  TIA* tia = new TIA(*this, myOSystem->settings());
  tia->setSound(myOSystem->sound());
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
  // This can be overridden by changing the framerate in the
  // VideoDialog box or on the commandline, but it can't be saved
  // (ie, framerate is now solely determined based on ROM format).
  uInt32 framerate = myOSystem->settings().getInt("framerate");
  if(framerate == 0)
  {
    if(myProperties.get("Display.Format") == "NTSC")
      framerate = 60;
    else if(myProperties.get("Display.Format") == "PAL")
      framerate = 50;
    else
      framerate = 60;
  }
  myOSystem->setFramerate(framerate);

  // Initialize the framebuffer interface.
  // This must be done *after* a reset, since it needs updated values.
  initializeVideo();

  // Initialize the sound interface.
  myOSystem->sound().setFrameRate(framerate);
  myOSystem->sound().initialize();

  // Initialize the menuing system with updated values from the framebuffer
  myOSystem->menu().initialize();
  myOSystem->menu().setGameProfile(myProperties);

  // Finally, initialize the debugging system, since it depends on the current ROM
  myOSystem->debugger().setConsole(this);
  myOSystem->debugger().initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
    : myOSystem(console.myOSystem)
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
void Console::toggleFormat()
{
  string format = myProperties.get("Display.Format");
  uInt32 framerate = 60;

  if(format == "NTSC")
  {
    myProperties.set("Display.Format", "PAL");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("PAL Mode");
    framerate = 50;
  }
  else if(format == "PAL")
  {
    myProperties.set("Display.Format", "NTSC");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("NTSC Mode");
    framerate = 60;
  }

  setPalette();
  myOSystem->setFramerate(framerate);
//FIXME - should be change sound rate as well??
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePalette(const string& palette)
{
  string message, type;

  // Since the toggle cycles in order, if we know which palette to switch
  // to we set the 'type' to the previous one in the sequence
  if(palette != "")
  {
    if(palette == "Standard")
      type = "z26";
    else if(palette == "Original")
      type = "standard";
    else if(palette == "Z26")
      type = "original";
    else
      type = "";
  }
  else
    type = myOSystem->settings().getString("palette");

 
  if(type == "standard")  // switch to original
  {
    type    = "original";
    message = "Original Stella colors";
  }
  else if(type == "original")  // switch to z26
  {
    type    = "z26";
    message = "Z26 colors";
  }
  else if(type == "z26")  // switch to standard
  {
    type    = "standard";
    message = "Standard Stella colors";
  }
  else  // switch to standard mode if we get this far
  {
    type    = "standard";
    message = "Standard Stella colors";
  }

  myOSystem->settings().setString("palette", type);
  myOSystem->frameBuffer().showMessage(message);

  setPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::saveProperties(string filename, bool merge)
{
  // Merge the current properties into the PropertiesSet file
  if(merge)
  {
    if(myOSystem->propSet().merge(myProperties, filename))
      myOSystem->frameBuffer().showMessage("Properties merged");
    else
      myOSystem->frameBuffer().showMessage("Properties not merged");
  }
  else  // Save to the specified file directly
  {
    ofstream out(filename.c_str(), ios::out);

    if(out && out.is_open())
    {
      myProperties.save(out);
      out.close();
      myOSystem->frameBuffer().showMessage("Properties saved");
    }
    else
    {
      myOSystem->frameBuffer().showMessage("Properties not saved");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeVideo()
{
  string title = "Stella: \"" + myProperties.get("Cartridge.Name") + "\"";
  myOSystem->frameBuffer().initialize(title,
                                      myMediaSource->width() << 1,
                                      myMediaSource->height());
  setPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeAudio()
{
  myMediaSource->setSound(myOSystem->sound());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setPalette()
{
  myOSystem->frameBuffer().setPalette(myMediaSource->palette());
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
      myOSystem->frameBuffer().showMessage("XStart at maximum");
      return;
    }
    else if((width + xstart) > 160)
    {
      myOSystem->frameBuffer().showMessage("XStart no effect");
      return;
    }
  }
  else if(direction == 0)  // decrease XStart
  {
    xstart -= 4;
    if(xstart < 0)
    {
      myOSystem->frameBuffer().showMessage("XStart at minimum");
      return;
    }
  }

  strval << xstart;
  myProperties.set("Display.XStart", strval.str());
  mySystem->reset();
  initializeVideo();

  message = "XStart ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
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
      myOSystem->frameBuffer().showMessage("YStart at maximum");
      return;
    }
  }
  else if(direction == 0)  // decrease YStart
  {
    ystart--;
    if(ystart < 0)
    {
      myOSystem->frameBuffer().showMessage("YStart at minimum");
      return;
    }
  }

  strval << ystart;
  myProperties.set("Display.YStart", strval.str());
  mySystem->reset();
  initializeVideo();

  message = "YStart ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
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
      myOSystem->frameBuffer().showMessage("Width at maximum");
      return;
    }
    else if((width + xstart) > 160)
    {
      myOSystem->frameBuffer().showMessage("Width no effect");
      return;
    }
  }
  else if(direction == 0)  // decrease Width
  {
    width -= 4;
    if(width < 80)
    {
      myOSystem->frameBuffer().showMessage("Width at minimum");
      return;
    }
  }

  strval << width;
  myProperties.set("Display.Width", strval.str());
  mySystem->reset();
  initializeVideo();

  message = "Width ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
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
      myOSystem->frameBuffer().showMessage("Height at maximum");
      return;
    }
  }
  else if(direction == 0)  // decrease Height
  {
    height--;
    if(height < 100)
    {
      myOSystem->frameBuffer().showMessage("Height at minimum");
      return;
    }
  }

  strval << height;
  myProperties.set("Display.Height", strval.str());
  mySystem->reset();
  initializeVideo();

  message = "Height ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleTIABit(TIA::TIABit bit, const string& bitname, bool show)
{
  bool result = ((TIA*)myMediaSource)->toggleBit(bit);
  string message = bitname + (result ? " enabled" : " disabled");
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::enableBits(bool enable)
{
  ((TIA*)myMediaSource)->enableBits(enable);
  string message = string("TIA bits") + (enable ? " enabled" : " disabled");
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setDeveloperProperties()
{
  Settings& settings = myOSystem->settings();
  string s;

  s = settings.getString("type");
  if(s != "")
    myProperties.set("Cartridge.Type", s);

  s = settings.getString("ld");
  if(s != "")
    myProperties.set("Console.LeftDifficulty", s);

  s = settings.getString("rd");
  if(s != "")
    myProperties.set("Console.RightDifficulty", s);

  s = settings.getString("tv");
  if(s != "")
    myProperties.set("Console.TelevisionType", s);

  s = settings.getString("lc");
  if(s != "")
    myProperties.set("Controller.Left", s);

  s = settings.getString("rc");
  if(s != "")
    myProperties.set("Controller.Right", s);

  s = settings.getString("bc");
  if(s != "")
  {
    myProperties.set("Controller.Left", s);
    myProperties.set("Controller.Right", s);
  }

  s = settings.getString("format");
  if(s != "")
    myProperties.set("Display.Format", s);

  s = settings.getString("xstart");
  if(s != "")
    myProperties.set("Display.XStart", s);

  s = settings.getString("ystart");
  if(s != "")
    myProperties.set("Display.YStart", s);

  s = settings.getString("width");
  if(s != "")
    myProperties.set("Display.Width", s);

  s = settings.getString("height");
  if(s != "")
    myProperties.set("Display.Height", s);

  s = settings.getString("cpu");
  if(s != "")
    myProperties.set("Emulation.CPU", s);

  s = settings.getString("hmove");
  if(s != "")
    myProperties.set("Emulation.HmoveBlanks", s);
}

#endif
