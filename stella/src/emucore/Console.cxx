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
// $Id: Console.cxx,v 1.107 2006-12-13 00:05:46 stephena Exp $
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
    myIsInitializedFlag(false),
    myUserPaletteDefined(false),
    ourUserNTSCPalette(NULL),
    ourUserPALPalette(NULL)
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

  // Load user-defined palette for this ROM
  loadUserPalette();

  // Make sure height is set properly for PAL ROM
  if(myProperties.get(Display_Format).compare(0, 3, "PAL") == 0)
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
  int leftPort, rightPort;
  if(myProperties.get(Console_SwapPorts) == "NO")
  {
    leftPort = 0; rightPort = 1;
  }
  else
  {
    leftPort = 1; rightPort = 0;
  }

  // Also check if we should swap the paddles plugged into a jack
  bool swapPaddles = myProperties.get(Controller_SwapPaddles) == "YES";

  // Construct left controller
  if(left == "BOOSTER-GRIP")
  {
    myControllers[leftPort] = new BoosterGrip(Controller::Left, *myEvent);
  }
  else if(left == "DRIVING")
  {
    myControllers[leftPort] = new Driving(Controller::Left, *myEvent);
  }
  else if((left == "KEYBOARD") || (left == "KEYPAD"))
  {
    myControllers[leftPort] = new Keyboard(Controller::Left, *myEvent);
  }
  else if(left == "PADDLES")
  {
    myControllers[leftPort] = new Paddles(Controller::Left, *myEvent, swapPaddles);
  }
  else
  {
    myControllers[leftPort] = new Joystick(Controller::Left, *myEvent);
  }
 
#ifdef ATARIVOX_SUPPORT 
  vox = 0;
#endif

  // Construct right controller
  if(right == "BOOSTER-GRIP")
  {
    myControllers[rightPort] = new BoosterGrip(Controller::Right, *myEvent);
  }
  else if(right == "DRIVING")
  {
    myControllers[rightPort] = new Driving(Controller::Right, *myEvent);
  }
  else if((right == "KEYBOARD") || (right == "KEYPAD"))
  {
    myControllers[rightPort] = new Keyboard(Controller::Right, *myEvent);
  }
  else if(right == "PADDLES")
  {
    myControllers[rightPort] = new Paddles(Controller::Right, *myEvent, swapPaddles);
  }
#ifdef ATARIVOX_SUPPORT 
  else if(right == "ATARIVOX")
  {
    myControllers[rightPort] = vox = new AtariVox(Controller::Right, *myEvent);
  }
#endif
  else
  {
    myControllers[rightPort] = new Joystick(Controller::Right, *myEvent);
  }

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
  Cartridge* cartridge = Cartridge::create(image, size, myProperties,
                                           myOSystem->settings());
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

  delete[] ourUserNTSCPalette;
  delete[] ourUserPALPalette;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Properties& Console::properties() const
{
  return myProperties;
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
    myProperties.set(Display_Format, "PAL60");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("PAL60 Mode");
    framerate = 60;
  }
  else if(format == "PAL60")
  {
    myProperties.set(Display_Format, "NTSC");
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("NTSC Mode");
    framerate = 60;
  }

  setPalette(myOSystem->settings().getString("palette"));
  myOSystem->setFramerate(framerate);
  myOSystem->sound().setFrameRate(framerate);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePalette()
{
  string palette, message;
  palette = myOSystem->settings().getString("palette");
 
  if(palette == "standard")       // switch to original
  {
    palette = "original";
    message = "Original Stella palette";
  }
  else if(palette == "original")  // switch to z26
  {
    palette = "z26";
    message = "Z26 palette";
  }
  else if(palette == "z26")       // switch to user or standard
  {
    // If we have a user-defined palette, it will come next in
    // the sequence; otherwise loop back to the standard one
    if(myUserPaletteDefined)
    {
      palette = "user";
      message = "User-defined palette";
    }
    else
    {
      palette = "standard";
      message = "Standard Stella palette";
    }
  }
  else if(palette == "user")  // switch to standard
  {
    palette = "standard";
    message = "Standard Stella palette";
  }
  else  // switch to standard mode if we get this far
  {
    palette = "standard";
    message = "Standard Stella palette";
  }

  myOSystem->settings().setString("palette", palette);
  myOSystem->frameBuffer().showMessage(message);

  setPalette(palette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setPalette(const string& type)
{
  // See which format we should be using
  const string& format = myProperties.get(Display_Format);

  const uInt32* palette = NULL;
  if(type == "standard")
    palette = (format.compare(0, 3, "PAL") == 0) ? ourPALPalette : ourNTSCPalette;
  else if(type == "original")
    palette = (format.compare(0, 3, "PAL") == 0) ? ourPALPalette11 : ourNTSCPalette11;
  else if(type == "z26")
    palette = (format.compare(0, 3, "PAL") == 0) ? ourPALPaletteZ26 : ourNTSCPaletteZ26;
  else if(type == "user" && myUserPaletteDefined)
    palette = (format.compare(0, 3, "PAL") == 0) ? ourUserPALPalette : ourUserNTSCPalette;
  else  // return normal palette by default
    palette = (format.compare(0, 3, "PAL") == 0) ? ourPALPalette : ourNTSCPalette;

  myOSystem->frameBuffer().setPalette(palette);

// FIXME - maybe add an error message that requested palette not available?
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePhosphor()
{
  const string& phosphor = myProperties.get(Display_Phosphor);
  int blend = atoi(myProperties.get(Display_PPBlend).c_str());
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

  myOSystem->frameBuffer().enablePhosphor(enable, blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setProperties(const Properties& props)
{
  myProperties = props;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initialize()
{
  // Set the correct framerate based on the format of the ROM
  // This can be overridden by changing the framerate in the
  // VideoDialog box or on the commandline, but it can't be saved
  // (ie, framerate is now solely determined based on ROM format).
  const string& format = myProperties.get(Display_Format);
  int framerate = myOSystem->settings().getInt("framerate");
  if(framerate == -1)
  {
    if(format == "NTSC" || format == "PAL60")
      framerate = 60;
    else if(format == "PAL")
      framerate = 50;
    else
      framerate = 60;
  }
  myOSystem->setFramerate(framerate);

  // Initialize the sound interface.
  // The # of channels can be overridden in the AudioDialog box or on
  // the commandline, but it can't be saved.
  uInt32 channels;
  const string& sound = myProperties.get(Cartridge_Sound);
  if(sound == "STEREO")
    channels = 2;
  else if(sound == "MONO")
    channels = 1;
  else
    channels = 1;

  myOSystem->sound().close();
  myOSystem->sound().setChannels(channels);
  myOSystem->sound().setFrameRate(framerate);
  myOSystem->sound().initialize();

  // Initialize the options menu system with updated values from the framebuffer
  myOSystem->menu().initialize();

  // Initialize the command menu system with updated values from the framebuffer
  myOSystem->commandMenu().initialize();

#ifdef DEVELOPER_SUPPORT
  // Finally, initialize the debugging system, since it depends on the current ROM
  myOSystem->debugger().setConsole(this);
  myOSystem->debugger().initialize();
#endif

  myIsInitializedFlag = true;
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
  int blend = atoi(myProperties.get(Display_PPBlend).c_str());
  myOSystem->frameBuffer().enablePhosphor(enable, blend);
  setPalette(myOSystem->settings().getString("palette"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeAudio()
{
  myMediaSource->setSound(myOSystem->sound());
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

  s = settings.getString("cp");
  if(s != "")
    myProperties.set(Controller_SwapPaddles, s);

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

  s = settings.getString("ppblend");
  if(s != "")
    myProperties.set(Display_PPBlend, s);

  s = settings.getString("hmove");
  if(s != "")
    myProperties.set(Emulation_HmoveBlanks, s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::loadUserPalette()
{
  const string& palette = myOSystem->baseDir() +
      BSPF_PATH_SEPARATOR + "stella.pal";

  ifstream in(palette.c_str(), ios::binary);
  if(!in)
    return;

  // Make sure the contains enough data for both the NTSC and PAL palettes
  // This means 128 colours each, at 3 bytes per pixel = 768 bytes
  in.seekg(0, ios::end);
  streampos length = in.tellg();
  in.seekg(0, ios::beg);
  if(length < 128 * 3 * 2)
  {
    in.close();
    cerr << "ERROR: invalid palette file " << palette << endl;
    return;
  }

  // Now that we have valid data, create the user-defined palettes
  ourUserNTSCPalette = new uInt32[256];
  ourUserPALPalette  = new uInt32[256];
  uInt8 pixbuf[3];  // Temporary buffer for one 24-bit pixel

  for(int i = 0; i < 128; i++)  // NTSC palette
  {
    in.read((char*)pixbuf, 3);
    uInt32 pixel = ((int)pixbuf[0] << 16) + ((int)pixbuf[1] << 8) + (int)pixbuf[2];
    ourUserNTSCPalette[(i<<1)] = ourUserNTSCPalette[(i<<1)+1] = pixel;
  }
  for(int i = 0; i < 128; i++)  // PAL palette
  {
    in.read((char*)pixbuf, 3);
    uInt32 pixel1 = ((int)pixbuf[0] << 16) + ((int)pixbuf[1] << 8) + (int)pixbuf[2];
    int r = (int)((float)pixbuf[0] * 0.2989);
    int g = (int)((float)pixbuf[1] * 0.5870);
    int b = (int)((float)pixbuf[2] * 0.1140);
    uInt32 pixel2 = (r << 16) + (g << 8) + b;
    ourUserPALPalette[(i<<1)]   = pixel1;
    ourUserPALPalette[(i<<1)+1] = pixel2;  // calculated colour-loss effect
  }

  in.close();
  myUserPaletteDefined = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourNTSCPalette[256] = {
  0x000000, 0x000000, 0x4a4a4a, 0x4a4a4a, 
  0x6f6f6f, 0x6f6f6f, 0x8e8e8e, 0x8e8e8e, 
  0xaaaaaa, 0xaaaaaa, 0xc0c0c0, 0xc0c0c0, 
  0xd6d6d6, 0xd6d6d6, 0xececec, 0xececec, 
  0x484800, 0x484800, 0x69690f, 0x69690f, 
  0x86861d, 0x86861d, 0xa2a22a, 0xa2a22a, 
  0xbbbb35, 0xbbbb35, 0xd2d240, 0xd2d240, 
  0xe8e84a, 0xe8e84a, 0xfcfc54, 0xfcfc54, 
  0x7c2c00, 0x7c2c00, 0x904811, 0x904811, 
  0xa26221, 0xa26221, 0xb47a30, 0xb47a30, 
  0xc3903d, 0xc3903d, 0xd2a44a, 0xd2a44a, 
  0xdfb755, 0xdfb755, 0xecc860, 0xecc860, 
  0x901c00, 0x901c00, 0xa33915, 0xa33915, 
  0xb55328, 0xb55328, 0xc66c3a, 0xc66c3a, 
  0xd5824a, 0xd5824a, 0xe39759, 0xe39759, 
  0xf0aa67, 0xf0aa67, 0xfcbc74, 0xfcbc74, 
  0x940000, 0x940000, 0xa71a1a, 0xa71a1a, 
  0xb83232, 0xb83232, 0xc84848, 0xc84848, 
  0xd65c5c, 0xd65c5c, 0xe46f6f, 0xe46f6f, 
  0xf08080, 0xf08080, 0xfc9090, 0xfc9090, 
  0x840064, 0x840064, 0x97197a, 0x97197a, 
  0xa8308f, 0xa8308f, 0xb846a2, 0xb846a2, 
  0xc659b3, 0xc659b3, 0xd46cc3, 0xd46cc3, 
  0xe07cd2, 0xe07cd2, 0xec8ce0, 0xec8ce0, 
  0x500084, 0x500084, 0x68199a, 0x68199a, 
  0x7d30ad, 0x7d30ad, 0x9246c0, 0x9246c0, 
  0xa459d0, 0xa459d0, 0xb56ce0, 0xb56ce0, 
  0xc57cee, 0xc57cee, 0xd48cfc, 0xd48cfc, 
  0x140090, 0x140090, 0x331aa3, 0x331aa3, 
  0x4e32b5, 0x4e32b5, 0x6848c6, 0x6848c6, 
  0x7f5cd5, 0x7f5cd5, 0x956fe3, 0x956fe3, 
  0xa980f0, 0xa980f0, 0xbc90fc, 0xbc90fc, 
  0x000094, 0x000094, 0x181aa7, 0x181aa7, 
  0x2d32b8, 0x2d32b8, 0x4248c8, 0x4248c8, 
  0x545cd6, 0x545cd6, 0x656fe4, 0x656fe4, 
  0x7580f0, 0x7580f0, 0x8490fc, 0x8490fc, 
  0x001c88, 0x001c88, 0x183b9d, 0x183b9d, 
  0x2d57b0, 0x2d57b0, 0x4272c2, 0x4272c2, 
  0x548ad2, 0x548ad2, 0x65a0e1, 0x65a0e1, 
  0x75b5ef, 0x75b5ef, 0x84c8fc, 0x84c8fc, 
  0x003064, 0x003064, 0x185080, 0x185080, 
  0x2d6d98, 0x2d6d98, 0x4288b0, 0x4288b0, 
  0x54a0c5, 0x54a0c5, 0x65b7d9, 0x65b7d9, 
  0x75cceb, 0x75cceb, 0x84e0fc, 0x84e0fc, 
  0x004030, 0x004030, 0x18624e, 0x18624e, 
  0x2d8169, 0x2d8169, 0x429e82, 0x429e82, 
  0x54b899, 0x54b899, 0x65d1ae, 0x65d1ae, 
  0x75e7c2, 0x75e7c2, 0x84fcd4, 0x84fcd4, 
  0x004400, 0x004400, 0x1a661a, 0x1a661a, 
  0x328432, 0x328432, 0x48a048, 0x48a048, 
  0x5cba5c, 0x5cba5c, 0x6fd26f, 0x6fd26f, 
  0x80e880, 0x80e880, 0x90fc90, 0x90fc90, 
  0x143c00, 0x143c00, 0x355f18, 0x355f18, 
  0x527e2d, 0x527e2d, 0x6e9c42, 0x6e9c42, 
  0x87b754, 0x87b754, 0x9ed065, 0x9ed065, 
  0xb4e775, 0xb4e775, 0xc8fc84, 0xc8fc84, 
  0x303800, 0x303800, 0x505916, 0x505916, 
  0x6d762b, 0x6d762b, 0x88923e, 0x88923e, 
  0xa0ab4f, 0xa0ab4f, 0xb7c25f, 0xb7c25f, 
  0xccd86e, 0xccd86e, 0xe0ec7c, 0xe0ec7c, 
  0x482c00, 0x482c00, 0x694d14, 0x694d14, 
  0x866a26, 0x866a26, 0xa28638, 0xa28638, 
  0xbb9f47, 0xbb9f47, 0xd2b656, 0xd2b656, 
  0xe8cc63, 0xe8cc63, 0xfce070, 0xfce070
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourPALPalette[256] = {
  0x000000, 0x000000, 0x2b2b2b, 0x2a2a2a, 
  0x525252, 0x515151, 0x767676, 0x757575, 
  0x979797, 0x969696, 0xb6b6b6, 0xb5b5b5, 
  0xd2d2d2, 0xd1d1d1, 0xececec, 0xebebeb, 
  0x000000, 0x000000, 0x2b2b2b, 0x2a2a2a, 
  0x525252, 0x515151, 0x767676, 0x757575, 
  0x979797, 0x969696, 0xb6b6b6, 0xb5b5b5, 
  0xd2d2d2, 0xd1d1d1, 0xececec, 0xebebeb, 
  0x805800, 0x595959, 0x96711a, 0x727272, 
  0xab8732, 0x888888, 0xbe9c48, 0x9c9c9c, 
  0xcfaf5c, 0xafafaf, 0xdfc06f, 0xc0c0c0, 
  0xeed180, 0xd0d0d0, 0xfce090, 0xdfdfdf, 
  0x445c00, 0x4a4a4a, 0x5e791a, 0x666666, 
  0x769332, 0x7f7f7f, 0x8cac48, 0x979797, 
  0xa0c25c, 0xacacac, 0xb3d76f, 0xc0c0c0, 
  0xc4ea80, 0xd2d2d2, 0xd4fc90, 0xe3e3e3, 
  0x703400, 0x404040, 0x89511a, 0x5b5b5b, 
  0xa06b32, 0x747474, 0xb68448, 0x8c8c8c, 
  0xc99a5c, 0xa0a0a0, 0xdcaf6f, 0xb5b5b5, 
  0xecc280, 0xc7c7c7, 0xfcd490, 0xd8d8d8, 
  0x006414, 0x3c3c3c, 0x1a8035, 0x585858, 
  0x329852, 0x717171, 0x48b06e, 0x898989, 
  0x5cc587, 0x9e9e9e, 0x6fd99e, 0xb2b2b2, 
  0x80ebb4, 0xc4c4c4, 0x90fcc8, 0xd5d5d5, 
  0x700014, 0x232323, 0x891a35, 0x3e3e3e, 
  0xa03252, 0x565656, 0xb6486e, 0x6d6d6d, 
  0xc95c87, 0x818181, 0xdc6f9e, 0x949494, 
  0xec80b4, 0xa6a6a6, 0xfc90c8, 0xb6b6b6, 
  0x005c5c, 0x404040, 0x1a7676, 0x5a5a5a, 
  0x328e8e, 0x727272, 0x48a4a4, 0x888888, 
  0x5cb8b8, 0x9c9c9c, 0x6fcbcb, 0xafafaf, 
  0x80dcdc, 0xc0c0c0, 0x90ecec, 0xd0d0d0, 
  0x70005c, 0x2b2b2b, 0x841a74, 0x434343, 
  0x963289, 0x595959, 0xa8489e, 0x6e6e6e, 
  0xb75cb0, 0x808080, 0xc66fc1, 0x929292, 
  0xd380d1, 0xa2a2a2, 0xe090e0, 0xb1b1b1, 
  0x003c70, 0x2f2f2f, 0x195a89, 0x4b4b4b, 
  0x2f75a0, 0x646464, 0x448eb6, 0x7c7c7c, 
  0x57a5c9, 0x919191, 0x68badc, 0xa5a5a5, 
  0x79ceec, 0xb7b7b7, 0x88e0fc, 0xc8c8c8, 
  0x580070, 0x272727, 0x6e1a89, 0x3f3f3f, 
  0x8332a0, 0x565656, 0x9648b6, 0x6b6b6b, 
  0xa75cc9, 0x7e7e7e, 0xb76fdc, 0x909090, 
  0xc680ec, 0xa1a1a1, 0xd490fc, 0xb0b0b0, 
  0x002070, 0x1f1f1f, 0x193f89, 0x3c3c3c, 
  0x2f5aa0, 0x555555, 0x4474b6, 0x6d6d6d, 
  0x578bc9, 0x828282, 0x68a1dc, 0x969696, 
  0x79b5ec, 0xa9a9a9, 0x88c8fc, 0xbababa, 
  0x340080, 0x1e1e1e, 0x4a1a96, 0x363636, 
  0x5f32ab, 0x4d4d4d, 0x7248be, 0x616161, 
  0x835ccf, 0x747474, 0x936fdf, 0x868686, 
  0xa280ee, 0x969696, 0xb090fc, 0xa5a5a5, 
  0x000088, 0x0f0f0f, 0x1a1a9d, 0x282828, 
  0x3232b0, 0x404040, 0x4848c2, 0x555555, 
  0x5c5cd2, 0x696969, 0x6f6fe1, 0x7b7b7b, 
  0x8080ef, 0x8c8c8c, 0x9090fc, 0x9c9c9c, 
  0x000000, 0x000000, 0x2b2b2b, 0x2a2a2a, 
  0x525252, 0x515151, 0x767676, 0x757575, 
  0x979797, 0x969696, 0xb6b6b6, 0xb5b5b5, 
  0xd2d2d2, 0xd1d1d1, 0xececec, 0xebebeb, 
  0x000000, 0x000000, 0x2b2b2b, 0x2a2a2a, 
  0x525252, 0x515151, 0x767676, 0x757575, 
  0x979797, 0x969696, 0xb6b6b6, 0xb5b5b5, 
  0xd2d2d2, 0xd1d1d1, 0xececec, 0xebebeb
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourNTSCPalette11[256] = {
  0x000000, 0x000000, 0x393939, 0x393939, 
  0x797979, 0x797979, 0xababab, 0xababab, 
  0xcdcdcd, 0xcdcdcd, 0xe6e6e6, 0xe6e6e6, 
  0xf2f2f2, 0xf2f2f2, 0xffffff, 0xffffff, 
  0x391701, 0x391701, 0x833008, 0x833008, 
  0xc85f24, 0xc85f24, 0xff911d, 0xff911d, 
  0xffc51d, 0xffc51d, 0xffd84c, 0xffd84c, 
  0xfff456, 0xfff456, 0xffff98, 0xffff98, 
  0x451904, 0x451904, 0x9f241e, 0x9f241e, 
  0xc85122, 0xc85122, 0xff811e, 0xff811e, 
  0xff982c, 0xff982c, 0xffc545, 0xffc545, 
  0xffc66d, 0xffc66d, 0xffe4a1, 0xffe4a1, 
  0x4a1704, 0x4a1704, 0xb21d17, 0xb21d17, 
  0xdf251c, 0xdf251c, 0xfa5255, 0xfa5255, 
  0xff706e, 0xff706e, 0xff8f8f, 0xff8f8f, 
  0xffabad, 0xffabad, 0xffc7ce, 0xffc7ce, 
  0x050568, 0x050568, 0x712272, 0x712272, 
  0xa532a6, 0xa532a6, 0xcd3ecf, 0xcd3ecf, 
  0xea51eb, 0xea51eb, 0xfe6dff, 0xfe6dff, 
  0xff87fb, 0xff87fb, 0xffa4ff, 0xffa4ff, 
  0x280479, 0x280479, 0x590f90, 0x590f90, 
  0x8839aa, 0x8839aa, 0xc04adc, 0xc04adc, 
  0xe05eff, 0xe05eff, 0xf27cff, 0xf27cff, 
  0xff98ff, 0xff98ff, 0xfeabff, 0xfeabff, 
  0x35088a, 0x35088a, 0x500cd0, 0x500cd0, 
  0x7945d0, 0x7945d0, 0xa251d9, 0xa251d9, 
  0xbe60ff, 0xbe60ff, 0xcc77ff, 0xcc77ff, 
  0xd790ff, 0xd790ff, 0xdfaaff, 0xdfaaff, 
  0x051e81, 0x051e81, 0x082fca, 0x082fca, 
  0x444cde, 0x444cde, 0x5a68ff, 0x5a68ff, 
  0x7183ff, 0x7183ff, 0x90a0ff, 0x90a0ff, 
  0x9fb2ff, 0x9fb2ff, 0xc0cbff, 0xc0cbff, 
  0x0c048b, 0x0c048b, 0x382db5, 0x382db5, 
  0x584fda, 0x584fda, 0x6b64ff, 0x6b64ff, 
  0x8a84ff, 0x8a84ff, 0x9998ff, 0x9998ff, 
  0xb1aeff, 0xb1aeff, 0xc0c2ff, 0xc0c2ff, 
  0x1d295a, 0x1d295a, 0x1d4892, 0x1d4892, 
  0x1c71c6, 0x1c71c6, 0x489bd9, 0x489bd9, 
  0x55b6ff, 0x55b6ff, 0x8cd8ff, 0x8cd8ff, 
  0x9bdfff, 0x9bdfff, 0xc3e9ff, 0xc3e9ff, 
  0x2f4302, 0x2f4302, 0x446103, 0x446103, 
  0x3e9421, 0x3e9421, 0x57ab3b, 0x57ab3b, 
  0x61d070, 0x61d070, 0x72f584, 0x72f584, 
  0x87ff97, 0x87ff97, 0xadffb6, 0xadffb6, 
  0x0a4108, 0x0a4108, 0x10680d, 0x10680d, 
  0x169212, 0x169212, 0x1cb917, 0x1cb917, 
  0x21d91b, 0x21d91b, 0x6ef040, 0x6ef040, 
  0x83ff5b, 0x83ff5b, 0xb2ff9a, 0xb2ff9a, 
  0x04410b, 0x04410b, 0x066611, 0x066611, 
  0x088817, 0x088817, 0x0baf1d, 0x0baf1d, 
  0x86d922, 0x86d922, 0x99f927, 0x99f927, 
  0xb7ff5b, 0xb7ff5b, 0xdcff81, 0xdcff81, 
  0x02350f, 0x02350f, 0x0c4a1c, 0x0c4a1c, 
  0x4f7420, 0x4f7420, 0x649228, 0x649228, 
  0xa1b034, 0xa1b034, 0xb2d241, 0xb2d241, 
  0xd6e149, 0xd6e149, 0xf2ff53, 0xf2ff53, 
  0x263001, 0x263001, 0x234005, 0x234005, 
  0x806931, 0x806931, 0xaf993a, 0xaf993a, 
  0xd5b543, 0xd5b543, 0xe1cb38, 0xe1cb38, 
  0xe3e534, 0xe3e534, 0xfbff7d, 0xfbff7d, 
  0x401a02, 0x401a02, 0x702408, 0x702408, 
  0xab511f, 0xab511f, 0xbf7730, 0xbf7730, 
  0xe19344, 0xe19344, 0xf9ad58, 0xf9ad58, 
  0xffc160, 0xffc160, 0xffcb83, 0xffcb83
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourPALPalette11[256] = {
  0x000000, 0x000000, 0x242424, 0x232323, 
  0x484848, 0x474747, 0x6d6d6d, 0x6c6c6c, 
  0x919191, 0x909090, 0xb6b6b6, 0xb5b5b5, 
  0xdadada, 0xd9d9d9, 0xffffff, 0xfefefe, 
  0x000000, 0x000000, 0x242424, 0x232323, 
  0x484848, 0x474747, 0x6d6d6d, 0x6c6c6c, 
  0x919191, 0x909090, 0xb6b6b6, 0xb5b5b5, 
  0xdadada, 0xd9d9d9, 0xffffff, 0xfefefe, 
  0x4a3700, 0x363636, 0x705813, 0x575757, 
  0x8c732a, 0x727272, 0xa68d46, 0x8c8c8c, 
  0xbea767, 0xa6a6a6, 0xd4c18b, 0xc0c0c0, 
  0xeadcb3, 0xdbdbdb, 0xfff6de, 0xf5f5f5, 
  0x284a00, 0x373737, 0x44700f, 0x575757, 
  0x5c8c21, 0x717171, 0x74a638, 0x8a8a8a, 
  0x8cbe51, 0xa2a2a2, 0xa6d46e, 0xbababa, 
  0xc0ea8e, 0xd2d2d2, 0xdbffb0, 0xebebeb, 
  0x4a1300, 0x212121, 0x70280f, 0x3a3a3a, 
  0x8c3d21, 0x515151, 0xa65438, 0x696969, 
  0xbe6d51, 0x828282, 0xd4886e, 0x9b9b9b, 
  0xeaa58e, 0xb6b6b6, 0xffc4b0, 0xd3d3d3, 
  0x004a22, 0x2f2f2f, 0x0f703b, 0x4c4c4c, 
  0x218c52, 0x656565, 0x38a66a, 0x7e7e7e, 
  0x51be83, 0x969696, 0x6ed49d, 0xafafaf, 
  0x8eeab8, 0xc8c8c8, 0xb0ffd4, 0xe2e2e2, 
  0x4a0028, 0x1a1a1a, 0x700f44, 0x323232, 
  0x8c215c, 0x474747, 0xa63874, 0x5f5f5f, 
  0xbe518c, 0x787878, 0xd46ea6, 0x929292, 
  0xea8ec0, 0xafafaf, 0xffb0db, 0xcccccc, 
  0x00404a, 0x2e2e2e, 0x0f6370, 0x4b4b4b, 
  0x217e8c, 0x636363, 0x3897a6, 0x7c7c7c, 
  0x51afbe, 0x949494, 0x6ec7d4, 0xadadad, 
  0x8edeea, 0xc7c7c7, 0xb0f4ff, 0xe0e0e0, 
  0x43002c, 0x191919, 0x650f4b, 0x2f2f2f, 
  0x7e2165, 0x444444, 0x953880, 0x5c5c5c, 
  0xa6519a, 0x727272, 0xbf6eb7, 0x8e8e8e, 
  0xd38ed3, 0xaaaaaa, 0xe5b0f1, 0xc7c7c7, 
  0x001d4a, 0x191919, 0x0f3870, 0x323232, 
  0x21538c, 0x4a4a4a, 0x386ea6, 0x646464, 
  0x518dbe, 0x808080, 0x6ea8d4, 0x9b9b9b, 
  0x8ec8ea, 0xbababa, 0xb0e9ff, 0xdadada, 
  0x37004a, 0x181818, 0x570f70, 0x2f2f2f, 
  0x70218c, 0x444444, 0x8938a6, 0x5c5c5c, 
  0xa151be, 0x757575, 0xba6ed4, 0x909090, 
  0xd28eea, 0xacacac, 0xeab0ff, 0xcacaca, 
  0x00184a, 0x161616, 0x0f2e70, 0x2c2c2c, 
  0x21448c, 0x414141, 0x385ba6, 0x595959, 
  0x5174be, 0x717171, 0x6e8fd4, 0x8c8c8c, 
  0x8eabea, 0xa9a9a9, 0xb0c9ff, 0xc7c7c7, 
  0x13004a, 0x0e0e0e, 0x280f70, 0x212121, 
  0x3d218c, 0x353535, 0x5438a6, 0x4c4c4c, 
  0x6d51be, 0x656565, 0x886ed4, 0x818181, 
  0xa58eea, 0x9f9f9f, 0xc4b0ff, 0xbebebe, 
  0x00014a, 0x090909, 0x0f1170, 0x1b1b1b, 
  0x21248c, 0x2e2e2e, 0x383aa6, 0x454545, 
  0x5153be, 0x5e5e5e, 0x6e70d4, 0x7a7a7a, 
  0x8e8fea, 0x999999, 0xb0b2ff, 0xbababa, 
  0x000000, 0x000000, 0x242424, 0x232323, 
  0x484848, 0x474747, 0x6d6d6d, 0x6c6c6c, 
  0x919191, 0x909090, 0xb6b6b6, 0xb5b5b5, 
  0xdadada, 0xd9d9d9, 0xffffff, 0xfefefe, 
  0x000000, 0x000000, 0x242424, 0x232323, 
  0x484848, 0x474747, 0x6d6d6d, 0x6c6c6c, 
  0x919191, 0x909090, 0xb6b6b6, 0xb5b5b5, 
  0xdadada, 0xd9d9d9, 0xffffff, 0xfefefe
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourNTSCPaletteZ26[256] = {
  0x000000, 0x000000, 0x505050, 0x505050, 
  0x646464, 0x646464, 0x787878, 0x787878, 
  0x8c8c8c, 0x8c8c8c, 0xa0a0a0, 0xa0a0a0, 
  0xb4b4b4, 0xb4b4b4, 0xc8c8c8, 0xc8c8c8, 
  0x445400, 0x445400, 0x586800, 0x586800, 
  0x6c7c00, 0x6c7c00, 0x809000, 0x809000, 
  0x94a414, 0x94a414, 0xa8b828, 0xa8b828, 
  0xbccc3c, 0xbccc3c, 0xd0e050, 0xd0e050, 
  0x673900, 0x673900, 0x7b4d00, 0x7b4d00, 
  0x8f6100, 0x8f6100, 0xa37513, 0xa37513, 
  0xb78927, 0xb78927, 0xcb9d3b, 0xcb9d3b, 
  0xdfb14f, 0xdfb14f, 0xf3c563, 0xf3c563, 
  0x7b2504, 0x7b2504, 0x8f3918, 0x8f3918, 
  0xa34d2c, 0xa34d2c, 0xb76140, 0xb76140, 
  0xcb7554, 0xcb7554, 0xdf8968, 0xdf8968, 
  0xf39d7c, 0xf39d7c, 0xffb190, 0xffb190, 
  0x7d122c, 0x7d122c, 0x912640, 0x912640, 
  0xa53a54, 0xa53a54, 0xb94e68, 0xb94e68, 
  0xcd627c, 0xcd627c, 0xe17690, 0xe17690, 
  0xf58aa4, 0xf58aa4, 0xff9eb8, 0xff9eb8, 
  0x730871, 0x730871, 0x871c85, 0x871c85, 
  0x9b3099, 0x9b3099, 0xaf44ad, 0xaf44ad, 
  0xc358c1, 0xc358c1, 0xd76cd5, 0xd76cd5, 
  0xeb80e9, 0xeb80e9, 0xff94fd, 0xff94fd, 
  0x5d0b92, 0x5d0b92, 0x711fa6, 0x711fa6, 
  0x8533ba, 0x8533ba, 0x9947ce, 0x9947ce, 
  0xad5be2, 0xad5be2, 0xc16ff6, 0xc16ff6, 
  0xd583ff, 0xd583ff, 0xe997ff, 0xe997ff, 
  0x401599, 0x401599, 0x5429ad, 0x5429ad, 
  0x683dc1, 0x683dc1, 0x7c51d5, 0x7c51d5, 
  0x9065e9, 0x9065e9, 0xa479fd, 0xa479fd, 
  0xb88dff, 0xb88dff, 0xcca1ff, 0xcca1ff, 
  0x252593, 0x252593, 0x3939a7, 0x3939a7, 
  0x4d4dbb, 0x4d4dbb, 0x6161cf, 0x6161cf, 
  0x7575e3, 0x7575e3, 0x8989f7, 0x8989f7, 
  0x9d9dff, 0x9d9dff, 0xb1b1ff, 0xb1b1ff, 
  0x0f3480, 0x0f3480, 0x234894, 0x234894, 
  0x375ca8, 0x375ca8, 0x4b70bc, 0x4b70bc, 
  0x5f84d0, 0x5f84d0, 0x7398e4, 0x7398e4, 
  0x87acf8, 0x87acf8, 0x9bc0ff, 0x9bc0ff, 
  0x04425a, 0x04425a, 0x18566e, 0x18566e, 
  0x2c6a82, 0x2c6a82, 0x407e96, 0x407e96, 
  0x5492aa, 0x5492aa, 0x68a6be, 0x68a6be, 
  0x7cbad2, 0x7cbad2, 0x90cee6, 0x90cee6, 
  0x044f30, 0x044f30, 0x186344, 0x186344, 
  0x2c7758, 0x2c7758, 0x408b6c, 0x408b6c, 
  0x549f80, 0x549f80, 0x68b394, 0x68b394, 
  0x7cc7a8, 0x7cc7a8, 0x90dbbc, 0x90dbbc, 
  0x0f550a, 0x0f550a, 0x23691e, 0x23691e, 
  0x377d32, 0x377d32, 0x4b9146, 0x4b9146, 
  0x5fa55a, 0x5fa55a, 0x73b96e, 0x73b96e, 
  0x87cd82, 0x87cd82, 0x9be196, 0x9be196, 
  0x1f5100, 0x1f5100, 0x336505, 0x336505, 
  0x477919, 0x477919, 0x5b8d2d, 0x5b8d2d, 
  0x6fa141, 0x6fa141, 0x83b555, 0x83b555, 
  0x97c969, 0x97c969, 0xabdd7d, 0xabdd7d, 
  0x344600, 0x344600, 0x485a00, 0x485a00, 
  0x5c6e14, 0x5c6e14, 0x708228, 0x708228, 
  0x84963c, 0x84963c, 0x98aa50, 0x98aa50, 
  0xacbe64, 0xacbe64, 0xc0d278, 0xc0d278, 
  0x463e00, 0x463e00, 0x5a5205, 0x5a5205, 
  0x6e6619, 0x6e6619, 0x827a2d, 0x827a2d, 
  0x968e41, 0x968e41, 0xaaa255, 0xaaa255, 
  0xbeb669, 0xbeb669, 0xd2ca7d, 0xd2ca7d
}; 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Console::ourPALPaletteZ26[256] = {
  0x000000, 0x000000, 0x4c4c4c, 0x4b4b4b, 
  0x606060, 0x5f5f5f, 0x747474, 0x737373, 
  0x888888, 0x878787, 0x9c9c9c, 0x9b9b9b, 
  0xb0b0b0, 0xafafaf, 0xc4c4c4, 0xc3c3c3, 
  0x000000, 0x000000, 0x4c4c4c, 0x4b4b4b, 
  0x606060, 0x5f5f5f, 0x747474, 0x737373, 
  0x888888, 0x878787, 0x9c9c9c, 0x9b9b9b, 
  0xb0b0b0, 0xafafaf, 0xc4c4c4, 0xc3c3c3, 
  0x533a00, 0x3a3a3a, 0x674e00, 0x4c4c4c, 
  0x7b6203, 0x5e5e5e, 0x8f7617, 0x727272, 
  0xa38a2b, 0x868686, 0xb79e3f, 0x9a9a9a, 
  0xcbb253, 0xaeaeae, 0xdfc667, 0xc2c2c2, 
  0x1b5800, 0x3b3b3b, 0x2f6c00, 0x4d4d4d, 
  0x438001, 0x5f5f5f, 0x579415, 0x737373, 
  0x6ba829, 0x878787, 0x7fbc3d, 0x9b9b9b, 
  0x93d051, 0xafafaf, 0xa7e465, 0xc3c3c3, 
  0x6a2900, 0x373737, 0x7e3d12, 0x4b4b4b, 
  0x925126, 0x5f5f5f, 0xa6653a, 0x737373, 
  0xba794e, 0x878787, 0xce8d62, 0x9b9b9b, 
  0xe2a176, 0xafafaf, 0xf6b58a, 0xc3c3c3, 
  0x075b00, 0x373737, 0x1b6f11, 0x4b4b4b, 
  0x2f8325, 0x5f5f5f, 0x439739, 0x737373, 
  0x57ab4d, 0x878787, 0x6bbf61, 0x9b9b9b, 
  0x7fd375, 0xafafaf, 0x93e789, 0xc3c3c3, 
  0x741b2f, 0x373737, 0x882f43, 0x4b4b4b, 
  0x9c4357, 0x5f5f5f, 0xb0576b, 0x737373, 
  0xc46b7f, 0x878787, 0xd87f93, 0x9b9b9b, 
  0xec93a7, 0xafafaf, 0xffa7bb, 0xc3c3c3, 
  0x00572e, 0x383838, 0x106b42, 0x4b4b4b, 
  0x247f56, 0x5f5f5f, 0x38936a, 0x737373, 
  0x4ca77e, 0x878787, 0x60bb92, 0x9b9b9b, 
  0x74cfa6, 0xafafaf, 0x88e3ba, 0xc3c3c3, 
  0x6d165f, 0x383838, 0x812a73, 0x4c4c4c, 
  0x953e87, 0x606060, 0xa9529b, 0x747474, 
  0xbd66af, 0x888888, 0xd17ac3, 0x9c9c9c, 
  0xe58ed7, 0xb0b0b0, 0xf9a2eb, 0xc4c4c4, 
  0x014c5e, 0x373737, 0x156072, 0x4b4b4b, 
  0x297486, 0x5f5f5f, 0x3d889a, 0x737373, 
  0x519cae, 0x878787, 0x65b0c2, 0x9b9b9b, 
  0x79c4d6, 0xafafaf, 0x8dd8ea, 0xc3c3c3, 
  0x5f1588, 0x383838, 0x73299c, 0x4c4c4c, 
  0x873db0, 0x606060, 0x9b51c4, 0x747474, 
  0xaf65d8, 0x888888, 0xc379ec, 0x9c9c9c, 
  0xd78dff, 0xb0b0b0, 0xeba1ff, 0xc1c1c1, 
  0x123b87, 0x373737, 0x264f9b, 0x4b4b4b, 
  0x3a63af, 0x5f5f5f, 0x4e77c3, 0x737373, 
  0x628bd7, 0x878787, 0x769feb, 0x9b9b9b, 
  0x8ab3ff, 0xafafaf, 0x9ec7ff, 0xc1c1c1, 
  0x451e9d, 0x383838, 0x5932b1, 0x4c4c4c, 
  0x6d46c5, 0x606060, 0x815ad9, 0x747474, 
  0x956eed, 0x888888, 0xa982ff, 0x9b9b9b, 
  0xbd96ff, 0xadadad, 0xd1aaff, 0xbfbfbf, 
  0x2a2b9e, 0x373737, 0x3e3fb2, 0x4b4b4b, 
  0x5253c6, 0x5f5f5f, 0x6667da, 0x737373, 
  0x7a7bee, 0x878787, 0x8e8fff, 0x9b9b9b, 
  0xa2a3ff, 0xadadad, 0xb6b7ff, 0xbebebe, 
  0x000000, 0x000000, 0x4c4c4c, 0x4b4b4b, 
  0x606060, 0x5f5f5f, 0x747474, 0x737373, 
  0x888888, 0x878787, 0x9c9c9c, 0x9b9b9b, 
  0xb0b0b0, 0xafafaf, 0xc4c4c4, 0xc3c3c3, 
  0x000000, 0x000000, 0x4c4c4c, 0x4b4b4b, 
  0x606060, 0x5f5f5f, 0x747474, 0x737373, 
  0x888888, 0x878787, 0x9c9c9c, 0x9b9b9b, 
  0xb0b0b0, 0xafafaf, 0xc4c4c4, 0xc3c3c3
}; 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(const Console& console)
  : myOSystem(console.myOSystem)
{
  // TODO: Write this method
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console& Console::operator = (const Console&)
{
  // TODO: Write this method
  assert(false);

  return *this;
}
