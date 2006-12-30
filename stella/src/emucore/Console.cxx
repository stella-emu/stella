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
// $Id: Console.cxx,v 1.116 2006-12-30 22:26:28 stephena Exp $
//============================================================================

#include <cassert>
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

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(OSystem* osystem, Cartridge* cart, const Properties& props)
  : myOSystem(osystem),
    myProperties(props),
    myUserPaletteDefined(false)
{
  myControllers[0] = 0;
  myControllers[1] = 0;
  myMediaSource = 0;
  mySwitches = 0;
  mySystem = 0;
  myEvent = 0;

  // Attach the event subsystem to the current console
  myEvent = myOSystem->eventHandler().event();

  // Load user-defined palette for this ROM
  loadUserPalette();

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
#ifdef DEBUGGER_SUPPORT
  m6502->attach(myOSystem->debugger());
#endif

  M6532* m6532 = new M6532(*this);
  TIA *tia = new TIA(*this, myOSystem->settings());
  tia->setSound(myOSystem->sound());

  mySystem->attach(m6502);
  mySystem->attach(m6532);
  mySystem->attach(tia);
  mySystem->attach(cart);

  // Remember what my media source is
  myMediaSource = tia;
  myCart = cart;
  myRiot = m6532;

  // Query some info about this console
  ostringstream buf;
  buf << "  Cart Name: " << myProperties.get(Cartridge_Name) << endl
      << "  Cart MD5:  " << myProperties.get(Cartridge_MD5) << endl;

  // Auto-detect NTSC/PAL mode if it's requested
  myDisplayFormat = myProperties.get(Display_Format);
  buf << "  Specified display format: " << myDisplayFormat << endl;
  if(myDisplayFormat == "AUTO-DETECT" ||
     myOSystem->settings().getBool("rominfo"))
  {
    // Run the system for 60 frames, looking for PAL scanline patterns
    // We assume the first 30 frames are garbage, and only consider
    // the second 30 (useful to get past SuperCharger BIOS)
    // Unfortunately, this means we have to always enable 'fastscbios',
    // since otherwise the BIOS loading will take over 250 frames!
    mySystem->reset();
    int palCount = 0;
    for(int i = 0; i < 60; ++i)
    {
      myMediaSource->update();
      if(i >= 30 && myMediaSource->scanlines() > 285)
        ++palCount;
    }

    myDisplayFormat = (palCount >= 15) ? "PAL" : "NTSC";
    if(myProperties.get(Display_Format) == "AUTO-DETECT")
      buf << "  Auto-detected display format: " << myDisplayFormat << endl;
  }
  buf << cart->about();

  // Make sure height is set properly for PAL ROM
  if(myDisplayFormat.compare(0, 3, "PAL") == 0)
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

  // Reset, the system to its power-on state
  mySystem->reset();

  myAboutString = buf.str();
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
void Console::toggleFormat()
{
  int framerate = 60;
  if(myDisplayFormat == "NTSC")
  {
    myDisplayFormat = "PAL";
    myProperties.set(Display_Format, myDisplayFormat);
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("PAL Mode");
    framerate = 50;
  }
  else if(myDisplayFormat == "PAL")
  {
    myDisplayFormat = "PAL60";
    myProperties.set(Display_Format, myDisplayFormat);
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("PAL60 Mode");
    framerate = 60;
  }
  else if(myDisplayFormat == "PAL60")
  {
    myDisplayFormat = "NTSC";
    myProperties.set(Display_Format, myDisplayFormat);
    mySystem->reset();
    myOSystem->frameBuffer().showMessage("NTSC Mode");
    framerate = 60;
  }

  setPalette(myOSystem->settings().getString("palette"));
  myOSystem->setFramerate(framerate);
  myOSystem->sound().setFrameRate(framerate);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleColorLoss()
{
  bool colorloss = !myOSystem->settings().getBool("colorloss");
  myOSystem->settings().setBool("colorloss", colorloss);
  setColorLossPalette(colorloss);
  setPalette(myOSystem->settings().getString("palette"));

  string message = string("PAL color-loss ") +
                   (colorloss ? "enabled" : "disabled");
  myOSystem->frameBuffer().showMessage(message);
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
  const uInt32* palette = NULL;
  if(type == "standard")
    palette = (myDisplayFormat.compare(0, 3, "PAL") == 0) ? ourPALPalette : ourNTSCPalette;
  else if(type == "original")
    palette = (myDisplayFormat.compare(0, 3, "PAL") == 0) ? ourPALPalette11 : ourNTSCPalette11;
  else if(type == "z26")
    palette = (myDisplayFormat.compare(0, 3, "PAL") == 0) ? ourPALPaletteZ26 : ourNTSCPaletteZ26;
  else if(type == "user" && myUserPaletteDefined)
    palette = (myDisplayFormat.compare(0, 3, "PAL") == 0) ? ourUserPALPalette : ourUserNTSCPalette;
  else  // return normal palette by default
    palette = (myDisplayFormat.compare(0, 3, "PAL") == 0) ? ourPALPalette : ourNTSCPalette;

  myOSystem->frameBuffer().setTIAPalette(palette);

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
void Console::initializeVideo(bool full)
{
  if(full)
  {
    string title = string("Stella ") + STELLA_VERSION +
                   ": \"" + myProperties.get(Cartridge_Name) + "\"";
    myOSystem->frameBuffer().initialize(title,
                                        myMediaSource->width() << 1,
                                        myMediaSource->height());
  }

  bool enable = myProperties.get(Display_Phosphor) == "YES";
  int blend = atoi(myProperties.get(Display_PPBlend).c_str());
  myOSystem->frameBuffer().enablePhosphor(enable, blend);
  setColorLossPalette(myOSystem->settings().getBool("colorloss"));
  setPalette(myOSystem->settings().getString("palette"));

  myOSystem->setFramerate(getFrameRate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeAudio()
{
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
  myOSystem->sound().setFrameRate(getFrameRate());
  myOSystem->sound().initialize();
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
void Console::fry() const
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
    if(height < 200)
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
void Console::toggleTIABit(TIA::TIABit bit, const string& bitname, bool show) const
{
  bool result = ((TIA*)myMediaSource)->toggleBit(bit);
  string message = bitname + (result ? " enabled" : " disabled");
  myOSystem->frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::enableBits(bool enable) const
{
  ((TIA*)myMediaSource)->enableBits(enable);
  string message = string("TIA bits") + (enable ? " enabled" : " disabled");
  myOSystem->frameBuffer().showMessage(message);
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
  uInt8 pixbuf[3];  // Temporary buffer for one 24-bit pixel

  for(int i = 0; i < 128; i++)  // NTSC palette
  {
    in.read((char*)pixbuf, 3);
    uInt32 pixel = ((int)pixbuf[0] << 16) + ((int)pixbuf[1] << 8) + (int)pixbuf[2];
    ourUserNTSCPalette[(i<<1)] = pixel;
  }
  for(int i = 0; i < 128; i++)  // PAL palette
  {
    in.read((char*)pixbuf, 3);
    uInt32 pixel = ((int)pixbuf[0] << 16) + ((int)pixbuf[1] << 8) + (int)pixbuf[2];
    ourUserPALPalette[(i<<1)] = pixel;
  }

  in.close();
  myUserPaletteDefined = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setColorLossPalette(bool loss)
{
  // Look at all the palettes, since we don't know which one is
  // currently active
  uInt32* palette[8] = {
    &ourNTSCPalette[0],    &ourPALPalette[0],
    &ourNTSCPalette11[0],  &ourPALPalette11[0],
    &ourNTSCPaletteZ26[0], &ourPALPaletteZ26[0],
    0, 0
  };
  if(myUserPaletteDefined)
  {
    palette[6] = &ourUserNTSCPalette[0];
    palette[7] = &ourUserPALPalette[0];
  }

  for(int i = 0; i < 8; ++i)
  {
    if(palette[i] == 0)
      continue;

    // If color-loss is enabled, fill the odd numbered palette entries
    // with gray values (calculated using the standard RGB -> grayscale
    // conversion formula)
    for(int j = 0; j < 128; ++j)
    {
      uInt32 pixel = palette[i][(j<<1)];
      if(loss)
      {
        uInt8 r = (pixel >> 16) & 0xff;
        uInt8 g = (pixel >> 8)  & 0xff;
        uInt8 b = (pixel >> 0)  & 0xff;
        uInt8 sum = (uInt8) (((float)r * 0.2989) +
                             ((float)g * 0.5870) +
                             ((float)b * 0.1140));
        pixel = (sum << 16) + (sum << 8) + sum;
      }
      palette[i][(j<<1)+1] = pixel;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::getFrameRate() const
{
  // Set the correct framerate based on the format of the ROM
  // This can be overridden by changing the framerate in the
  // VideoDialog box or on the commandline, but it can't be saved
  // (ie, framerate is now solely determined based on ROM format).
  int framerate = myOSystem->settings().getInt("framerate");
  if(framerate == -1)
  {
    if(myDisplayFormat == "NTSC" || myDisplayFormat == "PAL60")
      framerate = 60;
    else if(myDisplayFormat == "PAL")
      framerate = 50;
    else
      framerate = 60;
  }

  return framerate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourNTSCPalette[256] = {
  0x000000, 0, 0x4a4a4a, 0, 0x6f6f6f, 0, 0x8e8e8e, 0,
  0xaaaaaa, 0, 0xc0c0c0, 0, 0xd6d6d6, 0, 0xececec, 0,
  0x484800, 0, 0x69690f, 0, 0x86861d, 0, 0xa2a22a, 0,
  0xbbbb35, 0, 0xd2d240, 0, 0xe8e84a, 0, 0xfcfc54, 0,
  0x7c2c00, 0, 0x904811, 0, 0xa26221, 0, 0xb47a30, 0,
  0xc3903d, 0, 0xd2a44a, 0, 0xdfb755, 0, 0xecc860, 0,
  0x901c00, 0, 0xa33915, 0, 0xb55328, 0, 0xc66c3a, 0,
  0xd5824a, 0, 0xe39759, 0, 0xf0aa67, 0, 0xfcbc74, 0,
  0x940000, 0, 0xa71a1a, 0, 0xb83232, 0, 0xc84848, 0,
  0xd65c5c, 0, 0xe46f6f, 0, 0xf08080, 0, 0xfc9090, 0,
  0x840064, 0, 0x97197a, 0, 0xa8308f, 0, 0xb846a2, 0,
  0xc659b3, 0, 0xd46cc3, 0, 0xe07cd2, 0, 0xec8ce0, 0,
  0x500084, 0, 0x68199a, 0, 0x7d30ad, 0, 0x9246c0, 0,
  0xa459d0, 0, 0xb56ce0, 0, 0xc57cee, 0, 0xd48cfc, 0,
  0x140090, 0, 0x331aa3, 0, 0x4e32b5, 0, 0x6848c6, 0,
  0x7f5cd5, 0, 0x956fe3, 0, 0xa980f0, 0, 0xbc90fc, 0,
  0x000094, 0, 0x181aa7, 0, 0x2d32b8, 0, 0x4248c8, 0,
  0x545cd6, 0, 0x656fe4, 0, 0x7580f0, 0, 0x8490fc, 0,
  0x001c88, 0, 0x183b9d, 0, 0x2d57b0, 0, 0x4272c2, 0,
  0x548ad2, 0, 0x65a0e1, 0, 0x75b5ef, 0, 0x84c8fc, 0,
  0x003064, 0, 0x185080, 0, 0x2d6d98, 0, 0x4288b0, 0,
  0x54a0c5, 0, 0x65b7d9, 0, 0x75cceb, 0, 0x84e0fc, 0,
  0x004030, 0, 0x18624e, 0, 0x2d8169, 0, 0x429e82, 0,
  0x54b899, 0, 0x65d1ae, 0, 0x75e7c2, 0, 0x84fcd4, 0,
  0x004400, 0, 0x1a661a, 0, 0x328432, 0, 0x48a048, 0,
  0x5cba5c, 0, 0x6fd26f, 0, 0x80e880, 0, 0x90fc90, 0,
  0x143c00, 0, 0x355f18, 0, 0x527e2d, 0, 0x6e9c42, 0,
  0x87b754, 0, 0x9ed065, 0, 0xb4e775, 0, 0xc8fc84, 0,
  0x303800, 0, 0x505916, 0, 0x6d762b, 0, 0x88923e, 0,
  0xa0ab4f, 0, 0xb7c25f, 0, 0xccd86e, 0, 0xe0ec7c, 0,
  0x482c00, 0, 0x694d14, 0, 0x866a26, 0, 0xa28638, 0,
  0xbb9f47, 0, 0xd2b656, 0, 0xe8cc63, 0, 0xfce070, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourPALPalette[256] = {
  0x000000, 0, 0x2b2b2b, 0, 0x525252, 0, 0x767676, 0,
  0x979797, 0, 0xb6b6b6, 0, 0xd2d2d2, 0, 0xececec, 0,
  0x000000, 0, 0x2b2b2b, 0, 0x525252, 0, 0x767676, 0,
  0x979797, 0, 0xb6b6b6, 0, 0xd2d2d2, 0, 0xececec, 0,
  0x805800, 0, 0x96711a, 0, 0xab8732, 0, 0xbe9c48, 0,
  0xcfaf5c, 0, 0xdfc06f, 0, 0xeed180, 0, 0xfce090, 0,
  0x445c00, 0, 0x5e791a, 0, 0x769332, 0, 0x8cac48, 0,
  0xa0c25c, 0, 0xb3d76f, 0, 0xc4ea80, 0, 0xd4fc90, 0,
  0x703400, 0, 0x89511a, 0, 0xa06b32, 0, 0xb68448, 0,
  0xc99a5c, 0, 0xdcaf6f, 0, 0xecc280, 0, 0xfcd490, 0,
  0x006414, 0, 0x1a8035, 0, 0x329852, 0, 0x48b06e, 0,
  0x5cc587, 0, 0x6fd99e, 0, 0x80ebb4, 0, 0x90fcc8, 0,
  0x700014, 0, 0x891a35, 0, 0xa03252, 0, 0xb6486e, 0,
  0xc95c87, 0, 0xdc6f9e, 0, 0xec80b4, 0, 0xfc90c8, 0,
  0x005c5c, 0, 0x1a7676, 0, 0x328e8e, 0, 0x48a4a4, 0,
  0x5cb8b8, 0, 0x6fcbcb, 0, 0x80dcdc, 0, 0x90ecec, 0,
  0x70005c, 0, 0x841a74, 0, 0x963289, 0, 0xa8489e, 0,
  0xb75cb0, 0, 0xc66fc1, 0, 0xd380d1, 0, 0xe090e0, 0,
  0x003c70, 0, 0x195a89, 0, 0x2f75a0, 0, 0x448eb6, 0,
  0x57a5c9, 0, 0x68badc, 0, 0x79ceec, 0, 0x88e0fc, 0,
  0x580070, 0, 0x6e1a89, 0, 0x8332a0, 0, 0x9648b6, 0,
  0xa75cc9, 0, 0xb76fdc, 0, 0xc680ec, 0, 0xd490fc, 0,
  0x002070, 0, 0x193f89, 0, 0x2f5aa0, 0, 0x4474b6, 0,
  0x578bc9, 0, 0x68a1dc, 0, 0x79b5ec, 0, 0x88c8fc, 0,
  0x340080, 0, 0x4a1a96, 0, 0x5f32ab, 0, 0x7248be, 0,
  0x835ccf, 0, 0x936fdf, 0, 0xa280ee, 0, 0xb090fc, 0,
  0x000088, 0, 0x1a1a9d, 0, 0x3232b0, 0, 0x4848c2, 0,
  0x5c5cd2, 0, 0x6f6fe1, 0, 0x8080ef, 0, 0x9090fc, 0,
  0x000000, 0, 0x2b2b2b, 0, 0x525252, 0, 0x767676, 0,
  0x979797, 0, 0xb6b6b6, 0, 0xd2d2d2, 0, 0xececec, 0,
  0x000000, 0, 0x2b2b2b, 0, 0x525252, 0, 0x767676, 0,
  0x979797, 0, 0xb6b6b6, 0, 0xd2d2d2, 0, 0xececec, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourNTSCPalette11[256] = {
  0x000000, 0, 0x393939, 0, 0x797979, 0, 0xababab, 0,
  0xcdcdcd, 0, 0xe6e6e6, 0, 0xf2f2f2, 0, 0xffffff, 0,
  0x391701, 0, 0x833008, 0, 0xc85f24, 0, 0xff911d, 0,
  0xffc51d, 0, 0xffd84c, 0, 0xfff456, 0, 0xffff98, 0,
  0x451904, 0, 0x9f241e, 0, 0xc85122, 0, 0xff811e, 0,
  0xff982c, 0, 0xffc545, 0, 0xffc66d, 0, 0xffe4a1, 0,
  0x4a1704, 0, 0xb21d17, 0, 0xdf251c, 0, 0xfa5255, 0,
  0xff706e, 0, 0xff8f8f, 0, 0xffabad, 0, 0xffc7ce, 0,
  0x050568, 0, 0x712272, 0, 0xa532a6, 0, 0xcd3ecf, 0,
  0xea51eb, 0, 0xfe6dff, 0, 0xff87fb, 0, 0xffa4ff, 0,
  0x280479, 0, 0x590f90, 0, 0x8839aa, 0, 0xc04adc, 0,
  0xe05eff, 0, 0xf27cff, 0, 0xff98ff, 0, 0xfeabff, 0,
  0x35088a, 0, 0x500cd0, 0, 0x7945d0, 0, 0xa251d9, 0,
  0xbe60ff, 0, 0xcc77ff, 0, 0xd790ff, 0, 0xdfaaff, 0,
  0x051e81, 0, 0x082fca, 0, 0x444cde, 0, 0x5a68ff, 0,
  0x7183ff, 0, 0x90a0ff, 0, 0x9fb2ff, 0, 0xc0cbff, 0,
  0x0c048b, 0, 0x382db5, 0, 0x584fda, 0, 0x6b64ff, 0,
  0x8a84ff, 0, 0x9998ff, 0, 0xb1aeff, 0, 0xc0c2ff, 0,
  0x1d295a, 0, 0x1d4892, 0, 0x1c71c6, 0, 0x489bd9, 0,
  0x55b6ff, 0, 0x8cd8ff, 0, 0x9bdfff, 0, 0xc3e9ff, 0,
  0x2f4302, 0, 0x446103, 0, 0x3e9421, 0, 0x57ab3b, 0,
  0x61d070, 0, 0x72f584, 0, 0x87ff97, 0, 0xadffb6, 0,
  0x0a4108, 0, 0x10680d, 0, 0x169212, 0, 0x1cb917, 0,
  0x21d91b, 0, 0x6ef040, 0, 0x83ff5b, 0, 0xb2ff9a, 0,
  0x04410b, 0, 0x066611, 0, 0x088817, 0, 0x0baf1d, 0,
  0x86d922, 0, 0x99f927, 0, 0xb7ff5b, 0, 0xdcff81, 0,
  0x02350f, 0, 0x0c4a1c, 0, 0x4f7420, 0, 0x649228, 0,
  0xa1b034, 0, 0xb2d241, 0, 0xd6e149, 0, 0xf2ff53, 0,
  0x263001, 0, 0x234005, 0, 0x806931, 0, 0xaf993a, 0,
  0xd5b543, 0, 0xe1cb38, 0, 0xe3e534, 0, 0xfbff7d, 0,
  0x401a02, 0, 0x702408, 0, 0xab511f, 0, 0xbf7730, 0,
  0xe19344, 0, 0xf9ad58, 0, 0xffc160, 0, 0xffcb83, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourPALPalette11[256] = {
  0x000000, 0, 0x242424, 0, 0x484848, 0, 0x6d6d6d, 0,
  0x919191, 0, 0xb6b6b6, 0, 0xdadada, 0, 0xffffff, 0,
  0x000000, 0, 0x242424, 0, 0x484848, 0, 0x6d6d6d, 0,
  0x919191, 0, 0xb6b6b6, 0, 0xdadada, 0, 0xffffff, 0,
  0x4a3700, 0, 0x705813, 0, 0x8c732a, 0, 0xa68d46, 0,
  0xbea767, 0, 0xd4c18b, 0, 0xeadcb3, 0, 0xfff6de, 0,
  0x284a00, 0, 0x44700f, 0, 0x5c8c21, 0, 0x74a638, 0,
  0x8cbe51, 0, 0xa6d46e, 0, 0xc0ea8e, 0, 0xdbffb0, 0,
  0x4a1300, 0, 0x70280f, 0, 0x8c3d21, 0, 0xa65438, 0,
  0xbe6d51, 0, 0xd4886e, 0, 0xeaa58e, 0, 0xffc4b0, 0,
  0x004a22, 0, 0x0f703b, 0, 0x218c52, 0, 0x38a66a, 0,
  0x51be83, 0, 0x6ed49d, 0, 0x8eeab8, 0, 0xb0ffd4, 0,
  0x4a0028, 0, 0x700f44, 0, 0x8c215c, 0, 0xa63874, 0,
  0xbe518c, 0, 0xd46ea6, 0, 0xea8ec0, 0, 0xffb0db, 0,
  0x00404a, 0, 0x0f6370, 0, 0x217e8c, 0, 0x3897a6, 0,
  0x51afbe, 0, 0x6ec7d4, 0, 0x8edeea, 0, 0xb0f4ff, 0,
  0x43002c, 0, 0x650f4b, 0, 0x7e2165, 0, 0x953880, 0,
  0xa6519a, 0, 0xbf6eb7, 0, 0xd38ed3, 0, 0xe5b0f1, 0,
  0x001d4a, 0, 0x0f3870, 0, 0x21538c, 0, 0x386ea6, 0,
  0x518dbe, 0, 0x6ea8d4, 0, 0x8ec8ea, 0, 0xb0e9ff, 0,
  0x37004a, 0, 0x570f70, 0, 0x70218c, 0, 0x8938a6, 0,
  0xa151be, 0, 0xba6ed4, 0, 0xd28eea, 0, 0xeab0ff, 0,
  0x00184a, 0, 0x0f2e70, 0, 0x21448c, 0, 0x385ba6, 0,
  0x5174be, 0, 0x6e8fd4, 0, 0x8eabea, 0, 0xb0c9ff, 0,
  0x13004a, 0, 0x280f70, 0, 0x3d218c, 0, 0x5438a6, 0,
  0x6d51be, 0, 0x886ed4, 0, 0xa58eea, 0, 0xc4b0ff, 0,
  0x00014a, 0, 0x0f1170, 0, 0x21248c, 0, 0x383aa6, 0,
  0x5153be, 0, 0x6e70d4, 0, 0x8e8fea, 0, 0xb0b2ff, 0,
  0x000000, 0, 0x242424, 0, 0x484848, 0, 0x6d6d6d, 0,
  0x919191, 0, 0xb6b6b6, 0, 0xdadada, 0, 0xffffff, 0,
  0x000000, 0, 0x242424, 0, 0x484848, 0, 0x6d6d6d, 0,
  0x919191, 0, 0xb6b6b6, 0, 0xdadada, 0, 0xffffff, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourNTSCPaletteZ26[256] = {
  0x000000, 0, 0x505050, 0, 0x646464, 0, 0x787878, 0,
  0x8c8c8c, 0, 0xa0a0a0, 0, 0xb4b4b4, 0, 0xc8c8c8, 0,
  0x445400, 0, 0x586800, 0, 0x6c7c00, 0, 0x809000, 0,
  0x94a414, 0, 0xa8b828, 0, 0xbccc3c, 0, 0xd0e050, 0,
  0x673900, 0, 0x7b4d00, 0, 0x8f6100, 0, 0xa37513, 0,
  0xb78927, 0, 0xcb9d3b, 0, 0xdfb14f, 0, 0xf3c563, 0,
  0x7b2504, 0, 0x8f3918, 0, 0xa34d2c, 0, 0xb76140, 0,
  0xcb7554, 0, 0xdf8968, 0, 0xf39d7c, 0, 0xffb190, 0,
  0x7d122c, 0, 0x912640, 0, 0xa53a54, 0, 0xb94e68, 0,
  0xcd627c, 0, 0xe17690, 0, 0xf58aa4, 0, 0xff9eb8, 0,
  0x730871, 0, 0x871c85, 0, 0x9b3099, 0, 0xaf44ad, 0,
  0xc358c1, 0, 0xd76cd5, 0, 0xeb80e9, 0, 0xff94fd, 0,
  0x5d0b92, 0, 0x711fa6, 0, 0x8533ba, 0, 0x9947ce, 0,
  0xad5be2, 0, 0xc16ff6, 0, 0xd583ff, 0, 0xe997ff, 0,
  0x401599, 0, 0x5429ad, 0, 0x683dc1, 0, 0x7c51d5, 0,
  0x9065e9, 0, 0xa479fd, 0, 0xb88dff, 0, 0xcca1ff, 0,
  0x252593, 0, 0x3939a7, 0, 0x4d4dbb, 0, 0x6161cf, 0,
  0x7575e3, 0, 0x8989f7, 0, 0x9d9dff, 0, 0xb1b1ff, 0,
  0x0f3480, 0, 0x234894, 0, 0x375ca8, 0, 0x4b70bc, 0,
  0x5f84d0, 0, 0x7398e4, 0, 0x87acf8, 0, 0x9bc0ff, 0,
  0x04425a, 0, 0x18566e, 0, 0x2c6a82, 0, 0x407e96, 0,
  0x5492aa, 0, 0x68a6be, 0, 0x7cbad2, 0, 0x90cee6, 0,
  0x044f30, 0, 0x186344, 0, 0x2c7758, 0, 0x408b6c, 0,
  0x549f80, 0, 0x68b394, 0, 0x7cc7a8, 0, 0x90dbbc, 0,
  0x0f550a, 0, 0x23691e, 0, 0x377d32, 0, 0x4b9146, 0,
  0x5fa55a, 0, 0x73b96e, 0, 0x87cd82, 0, 0x9be196, 0,
  0x1f5100, 0, 0x336505, 0, 0x477919, 0, 0x5b8d2d, 0,
  0x6fa141, 0, 0x83b555, 0, 0x97c969, 0, 0xabdd7d, 0,
  0x344600, 0, 0x485a00, 0, 0x5c6e14, 0, 0x708228, 0,
  0x84963c, 0, 0x98aa50, 0, 0xacbe64, 0, 0xc0d278, 0,
  0x463e00, 0, 0x5a5205, 0, 0x6e6619, 0, 0x827a2d, 0,
  0x968e41, 0, 0xaaa255, 0, 0xbeb669, 0, 0xd2ca7d, 0
}; 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourPALPaletteZ26[256] = {
  0x000000, 0, 0x4c4c4c, 0, 0x606060, 0, 0x747474, 0,
  0x888888, 0, 0x9c9c9c, 0, 0xb0b0b0, 0, 0xc4c4c4, 0,
  0x000000, 0, 0x4c4c4c, 0, 0x606060, 0, 0x747474, 0,
  0x888888, 0, 0x9c9c9c, 0, 0xb0b0b0, 0, 0xc4c4c4, 0,
  0x533a00, 0, 0x674e00, 0, 0x7b6203, 0, 0x8f7617, 0,
  0xa38a2b, 0, 0xb79e3f, 0, 0xcbb253, 0, 0xdfc667, 0,
  0x1b5800, 0, 0x2f6c00, 0, 0x438001, 0, 0x579415, 0,
  0x6ba829, 0, 0x7fbc3d, 0, 0x93d051, 0, 0xa7e465, 0,
  0x6a2900, 0, 0x7e3d12, 0, 0x925126, 0, 0xa6653a, 0,
  0xba794e, 0, 0xce8d62, 0, 0xe2a176, 0, 0xf6b58a, 0,
  0x075b00, 0, 0x1b6f11, 0, 0x2f8325, 0, 0x439739, 0,
  0x57ab4d, 0, 0x6bbf61, 0, 0x7fd375, 0, 0x93e789, 0,
  0x741b2f, 0, 0x882f43, 0, 0x9c4357, 0, 0xb0576b, 0,
  0xc46b7f, 0, 0xd87f93, 0, 0xec93a7, 0, 0xffa7bb, 0,
  0x00572e, 0, 0x106b42, 0, 0x247f56, 0, 0x38936a, 0,
  0x4ca77e, 0, 0x60bb92, 0, 0x74cfa6, 0, 0x88e3ba, 0,
  0x6d165f, 0, 0x812a73, 0, 0x953e87, 0, 0xa9529b, 0,
  0xbd66af, 0, 0xd17ac3, 0, 0xe58ed7, 0, 0xf9a2eb, 0,
  0x014c5e, 0, 0x156072, 0, 0x297486, 0, 0x3d889a, 0,
  0x519cae, 0, 0x65b0c2, 0, 0x79c4d6, 0, 0x8dd8ea, 0,
  0x5f1588, 0, 0x73299c, 0, 0x873db0, 0, 0x9b51c4, 0,
  0xaf65d8, 0, 0xc379ec, 0, 0xd78dff, 0, 0xeba1ff, 0,
  0x123b87, 0, 0x264f9b, 0, 0x3a63af, 0, 0x4e77c3, 0,
  0x628bd7, 0, 0x769feb, 0, 0x8ab3ff, 0, 0x9ec7ff, 0,
  0x451e9d, 0, 0x5932b1, 0, 0x6d46c5, 0, 0x815ad9, 0,
  0x956eed, 0, 0xa982ff, 0, 0xbd96ff, 0, 0xd1aaff, 0,
  0x2a2b9e, 0, 0x3e3fb2, 0, 0x5253c6, 0, 0x6667da, 0,
  0x7a7bee, 0, 0x8e8fff, 0, 0xa2a3ff, 0, 0xb6b7ff, 0,
  0x000000, 0, 0x4c4c4c, 0, 0x606060, 0, 0x747474, 0,
  0x888888, 0, 0x9c9c9c, 0, 0xb0b0b0, 0, 0xc4c4c4, 0,
  0x000000, 0, 0x4c4c4c, 0, 0x606060, 0, 0x747474, 0,
  0x888888, 0, 0x9c9c9c, 0, 0xb0b0b0, 0, 0xc4c4c4, 0
}; 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourUserNTSCPalette[256] = { 0 }; // filled from external file

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Console::ourUserPALPalette[256]  = { 0 }; // filled from external file

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
