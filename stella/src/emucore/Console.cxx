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
// $Id: Console.cxx,v 1.93 2006-08-09 02:38:03 bwmott Exp $
//============================================================================

#include <assert.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "AtariVox.hxx"
#include "Booster.hxx"
#include "Cart.hxx"
#include "Console.hxx"
#include "Control.hxx"
#include "Driving.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "Joystick.hxx"
#include "Keyboard.hxx"
#include "M6502Hi.hxx"
#include "M6502Low.hxx"
#include "M6532.hxx"
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
#include "CommandMenu.hxx"
#include "Version.hxx"

#ifdef SNAPSHOT_SUPPORT
  #include "Snapshot.hxx"
#endif

#ifdef DEVELOPER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const uInt8* image, uInt32 size, const string& md5,
                 OSystem* osystem)
  : myOSystem(osystem),
    myIsInitializedFlag(false)
{
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;
  myEvent = 0;

  // Attach the event subsystem to the current console
  myEvent = myOSystem->eventHandler().event();

  // Search for the properties based on MD5
  myOSystem->propSet().getMD5(md5, myProperties);

  // A developer can override properties from the commandline
  setDeveloperProperties();

  // Make sure height is set properly for PAL ROM
  if(myProperties.get(Display_Format) == "PAL")
    if(myProperties.get(Display_Height) == "210")
      myProperties.set(Display_Height, "250");

  // Make sure this ROM can fit in the screen dimensions
  int sWidth, sHeight, iWidth, iHeight;
  myOSystem->getScreenDimensions(sWidth, sHeight);
  iWidth  = atoi(myProperties.get(Display_Width).c_str()) << 1;
  iHeight = atoi(myProperties.get(Display_Height).c_str());
  if(iWidth > sWidth || iHeight > sHeight)
  {
    myOSystem->frameBuffer().showMessage("PAL ROMS not supported, screen too small",
                                          kMiddleCenter, kTextColorEm);
    return;
  }

  // Setup the controllers based on properties
  string left  = myProperties.get(Controller_Left);
  string right = myProperties.get(Controller_Right);

  // Swap the ports if necessary
  // Note that this doesn't swap the actual controllers,
  // just the ports that they're attached to
  Controller::Jack leftjack, rightjack;
  if(myProperties.get(Console_SwapPorts) == "NO")
  {
    leftjack  = Controller::Left;
    rightjack = Controller::Right;
  }
  else
  {
    leftjack  = Controller::Right;
    rightjack = Controller::Left;
  }

  // Construct left controller
  if(left == "BOOSTER-GRIP")
  {
    myControllers[0] = new BoosterGrip(leftjack, *myEvent);
  }
  else if(left == "DRIVING")
  {
    myControllers[0] = new Driving(leftjack, *myEvent);
  }
  else if((left == "KEYBOARD") || (left == "KEYPAD"))
  {
    myControllers[0] = new Keyboard(leftjack, *myEvent);
  }
  else if(left == "PADDLES")
  {
    myControllers[0] = new Paddles(leftjack, *myEvent);
  }
  else
  {
    myControllers[0] = new Joystick(leftjack, *myEvent);
  }
 
#ifdef ATARIVOX_SUPPORT 
  vox = 0;
#endif

  // Construct right controller
  if(right == "BOOSTER-GRIP")
  {
    myControllers[1] = new BoosterGrip(rightjack, *myEvent);
  }
  else if(right == "DRIVING")
  {
    myControllers[1] = new Driving(rightjack, *myEvent);
  }
  else if((right == "KEYBOARD") || (right == "KEYPAD"))
  {
    myControllers[1] = new Keyboard(rightjack, *myEvent);
  }
  else if(right == "PADDLES")
  {
    myControllers[1] = new Paddles(rightjack, *myEvent);
  }
#ifdef ATARIVOX_SUPPORT 
  else if(right == "ATARIVOX")
  {
    myControllers[1] = vox = new AtariVox(rightjack, *myEvent);
  }
#endif
  else
  {
    myControllers[1] = new Joystick(rightjack, *myEvent);
  }

#if 0 // this isn't production ready yet
  // Make a guess at which paddle the mouse should emulate,
  // by using the 'first' paddle in the pair
  if(myControllers[0]->type() == Controller::Paddles)
    myOSystem->eventHandler().setPaddleMode(0);
  else if(myControllers[1]->type() == Controller::Paddles)
    myOSystem->eventHandler().setPaddleMode(2);
#endif

  // Create switches for the console
  mySwitches = new Switches(*myEvent, myProperties);

  // Now, we can construct the system and components
  mySystem = new System(13, 6);

  // AtariVox is a smart peripheral; it needs access to the system
  // cycles counter, so it needs a reference to the System
#ifdef ATARIVOX_SUPPORT 
  if(vox)
    vox->setSystem(mySystem);
#endif

  M6502* m6502;
  if(myOSystem->settings().getString("cpu") == "low")
    m6502 = new M6502Low(1);
  else
    m6502 = new M6502High(1);
#ifdef DEVELOPER_SUPPORT
  m6502->attach(myOSystem->debugger());
#endif

  M6532* m6532 = new M6532(*this);
  TIA *tia = new TIA(*this, myOSystem->settings());
  tia->setSound(myOSystem->sound());
  Cartridge* cartridge = Cartridge::create(image, size, myProperties);
  if(!cartridge)
    return;

  mySystem->attach(m6502);
  mySystem->attach(m6532);
  mySystem->attach(tia);
  mySystem->attach(cartridge);

  // Remember what my media source is
  myMediaSource = tia;
  myCart = cartridge;
  myRiot = m6532;

  // Reset, the system to its power-on state
  mySystem->reset();

  // Set the correct framerate based on the format of the ROM
  // This can be overridden by changing the framerate in the
  // VideoDialog box or on the commandline, but it can't be saved
  // (ie, framerate is now solely determined based on ROM format).
  uInt32 framerate = myOSystem->settings().getInt("framerate");
  if(framerate == 0)
  {
    const string& s = myProperties.get(Display_Format);
    if(s == "NTSC")
      framerate = 60;
    else if(s == "PAL")
      framerate = 50;
    else
      framerate = 60;
  }
  myOSystem->setFramerate(framerate);

  // Initialize the framebuffer interface.
  // This must be done *after* a reset, since it needs updated values.
  initializeVideo();

  // Initialize the sound interface.
  // The # of channels can be overridden in the AudioDialog box or on
  // the commandline, but it can't be saved.
  uInt32 channels;
  const string& s = myProperties.get(Cartridge_Sound);
  if(s == "STEREO")
    channels = 2;
  else if(s == "MONO")
    channels = 1;
  else
    channels = 1;

  myOSystem->sound().close();
  myOSystem->sound().setChannels(channels);
  myOSystem->sound().setFrameRate(framerate);
  myOSystem->sound().initialize();

  // Initialize the options menu system with updated values from the framebuffer
  myOSystem->menu().initialize();
  myOSystem->menu().setGameProfile(myProperties);

  // Initialize the command menu system with updated values from the framebuffer
  myOSystem->commandMenu().initialize();

#ifdef DEVELOPER_SUPPORT
  // Finally, initialize the debugging system, since it depends on the current ROM
  myOSystem->debugger().setConsole(this);
  myOSystem->debugger().initialize();
#endif

  // If we get this far, the console must be valid
  myIsInitializedFlag = true;
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
#ifdef CHEATCODE_SUPPORT
  myOSystem->cheat().saveCheats(myProperties.get(Cartridge_MD5));
#endif

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
  const string& format = myProperties.get(Display_Format);
  uInt32 framerate = 60;

  if(format == "NTSC")
  {
    myProperties.set(Display_Format, "PAL");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("PAL Mode");
    framerate = 50;
  }
  else if(format == "PAL")
  {
    myProperties.set(Display_Format, "NTSC");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("NTSC Mode");
    framerate = 60;
  }

  setPalette();
  myOSystem->setFramerate(framerate);
  myOSystem->sound().setFrameRate(framerate);
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
void Console::togglePhosphor()
{
  const string& phosphor = myProperties.get(Display_Phosphor);
  bool enable;
  if(phosphor == "YES")
  {
    myProperties.set(Display_Phosphor, "No");
    enable = false;
    myOSystem->frameBuffer().showMessage("Phosphor effect disabled");
  }
  else
  {
    myProperties.set(Display_Phosphor, "Yes");
    enable = true;
    myOSystem->frameBuffer().showMessage("Phosphor effect enabled");
  }

  myOSystem->frameBuffer().enablePhosphor(enable);
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
      myOSystem->frameBuffer().showMessage("Error merging properties");
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
      myOSystem->frameBuffer().showMessage("Error saving properties");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeVideo()
{
  string title = string("Stella ") + STELLA_VERSION +
                 ": \"" + myProperties.get(Cartridge_Name) + "\"";
  myOSystem->frameBuffer().initialize(title,
                                      myMediaSource->width() << 1,
                                      myMediaSource->height());
  bool enable = myProperties.get(Display_Phosphor) == "YES";
  myOSystem->frameBuffer().enablePhosphor(enable);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setChannels(int channels)
{
  myOSystem->sound().setChannels(channels);

  // Save to properties
  string sound = channels == 2 ? "Stereo" : "Mono";
  myProperties.set(Cartridge_Sound, sound);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/* Original frying research and code by Fred Quimby.
   I've tried the following variations on this code:
   - Both OR and Exclusive OR instead of AND. This generally crashes the game
     without ever giving us realistic "fried" effects.
   - Loop only over the RIOT RAM. This still gave us frying-like effects, but
     it seemed harder to duplicate most effects. I have no idea why, but
     munging the TIA regs seems to have some effect (I'd think it wouldn't).

   Fred says he also tried mangling the PC and registers, but usually it'd just
   crash the game (e.g. black screen, no way out of it).

   It's definitely easier to get some effects (e.g. 255 lives in Battlezone)
   with this code than it is on a real console. My guess is that most "good"
   frying effects come from a RIOT location getting cleared to 0. Fred's
   code is more likely to accomplish this than frying a real console is...

   Until someone comes up with a more accurate way to emulate frying, I'm
   leaving this as Fred posted it.   -- B.
*/
void Console::fry()
{
  for (int ZPmem=0; ZPmem<0x100; ZPmem += rand() % 4)
    mySystem->poke(ZPmem, mySystem->peek(ZPmem) & (uInt8)rand() % 256);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeXStart(int direction)
{
  Int32 xstart = atoi(myProperties.get(Display_XStart).c_str());
  uInt32 width = atoi(myProperties.get(Display_Width).c_str());
  ostringstream strval;
  string message;

  if(direction == +1)    // increase XStart
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
  else if(direction == -1)  // decrease XStart
  {
    xstart -= 4;
    if(xstart < 0)
    {
      myOSystem->frameBuffer().showMessage("XStart at minimum");
      return;
    }
  }
  else
    return;

  strval << xstart;
  myProperties.set(Display_XStart, strval.str());
  mySystem->reset();
  initializeVideo();

  message = "XStart ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeYStart(int direction)
{
  Int32 ystart = atoi(myProperties.get(Display_YStart).c_str());
  ostringstream strval;
  string message;

  if(direction == +1)    // increase YStart
  {
    ystart++;
    if(ystart > 64)
    {
      myOSystem->frameBuffer().showMessage("YStart at maximum");
      return;
    }
  }
  else if(direction == -1)  // decrease YStart
  {
    ystart--;
    if(ystart < 0)
    {
      myOSystem->frameBuffer().showMessage("YStart at minimum");
      return;
    }
  }
  else
    return;

  strval << ystart;
  myProperties.set(Display_YStart, strval.str());
  mySystem->reset();
  initializeVideo();

  message = "YStart ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeWidth(int direction)
{
  uInt32 xstart = atoi(myProperties.get(Display_XStart).c_str());
  Int32 width   = atoi(myProperties.get(Display_Width).c_str());
  ostringstream strval;
  string message;

  if(direction == +1)    // increase Width
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
  else if(direction == -1)  // decrease Width
  {
    width -= 4;
    if(width < 80)
    {
      myOSystem->frameBuffer().showMessage("Width at minimum");
      return;
    }
  }
  else
    return;

  strval << width;
  myProperties.set(Display_Width, strval.str());
  mySystem->reset();
  initializeVideo();

  message = "Width ";
  message += strval.str();
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeHeight(int direction)
{
  Int32 height = atoi(myProperties.get(Display_Height).c_str());
  ostringstream strval;
  string message;

  if(direction == +1)    // increase Height
  {
    height++;
    if(height > 256)
    {
      myOSystem->frameBuffer().showMessage("Height at maximum");
      return;
    }
  }
  else if(direction == -1)  // decrease Height
  {
    height--;
    if(height < 100)
    {
      myOSystem->frameBuffer().showMessage("Height at minimum");
      return;
    }
  }
  else
    return;

  strval << height;
  myProperties.set(Display_Height, strval.str());
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
    myProperties.set(Cartridge_Type, s);

  s = settings.getString("ld");
  if(s != "")
    myProperties.set(Console_LeftDifficulty, s);

  s = settings.getString("rd");
  if(s != "")
    myProperties.set(Console_RightDifficulty, s);

  s = settings.getString("tv");
  if(s != "")
    myProperties.set(Console_TelevisionType, s);

  s = settings.getString("sp");
  if(s != "")
    myProperties.set(Console_SwapPorts, s);

  s = settings.getString("lc");
  if(s != "")
    myProperties.set(Controller_Left, s);

  s = settings.getString("rc");
  if(s != "")
    myProperties.set(Controller_Right, s);

  s = settings.getString("bc");
  if(s != "")
  {
    myProperties.set(Controller_Left, s);
    myProperties.set(Controller_Right, s);
  }

  s = settings.getString("format");
  if(s != "")
    myProperties.set(Display_Format, s);

  s = settings.getString("xstart");
  if(s != "")
    myProperties.set(Display_XStart, s);

  s = settings.getString("ystart");
  if(s != "")
    myProperties.set(Display_YStart, s);

  s = settings.getString("width");
  if(s != "")
    myProperties.set(Display_Width, s);

  s = settings.getString("height");
  if(s != "")
    myProperties.set(Display_Height, s);

  s = settings.getString("pp");
  if(s != "")
    myProperties.set(Display_Phosphor, s);

  s = settings.getString("hmove");
  if(s != "")
    myProperties.set(Emulation_HmoveBlanks, s);
}
