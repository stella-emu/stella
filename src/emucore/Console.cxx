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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cassert>
#include <stdexcept>
#include <regex>

#include "AtariVox.hxx"
#include "Booster.hxx"
#include "Cart.hxx"
#include "Control.hxx"
#include "Cart.hxx"
#include "Driving.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
#include "ControllerDetector.hxx"
#include "Joystick.hxx"
#include "Keyboard.hxx"
#include "KidVid.hxx"
#include "Genesis.hxx"
#include "MindLink.hxx"
#include "CompuMate.hxx"
#include "M6502.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Paddles.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "SaveKey.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "AmigaMouse.hxx"
#include "AtariMouse.hxx"
#include "TrakBall.hxx"
#include "Lightgun.hxx"
#include "FrameBuffer.hxx"
#include "TIASurface.hxx"
#include "OSystem.hxx"
#include "Serializable.hxx"
#include "Serializer.hxx"
#include "TimerManager.hxx"
#include "Version.hxx"
#include "TIAConstants.hxx"
#include "FrameLayout.hxx"
#include "AudioQueue.hxx"
#include "AudioSettings.hxx"
#include "frame-manager/FrameManager.hxx"
#include "frame-manager/FrameLayoutDetector.hxx"
#include "PaletteHandler.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#include "Console.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::Console(OSystem& osystem, unique_ptr<Cartridge>& cart,
                 const Properties& props, AudioSettings& audioSettings)
  : myOSystem(osystem),
    myEvent(osystem.eventHandler().event()),
    myProperties(props),
    myCart(std::move(cart)),
    myAudioSettings(audioSettings)
{
  // Create subsystems for the console
  my6502 = make_unique<M6502>(myOSystem.settings());
  myRiot = make_unique<M6532>(*this, myOSystem.settings());
  myTIA  = make_unique<TIA>(*this, [this]() { return timing(); },  myOSystem.settings());
  myFrameManager = make_unique<FrameManager>();
  mySwitches = make_unique<Switches>(myEvent, myProperties, myOSystem.settings());
  myPaletteHandler = make_unique<PaletteHandler>(myOSystem);

  myTIA->setFrameManager(myFrameManager.get());

  // Reinitialize the RNG
  myOSystem.random().initSeed(static_cast<uInt32>(TimerManager::getTicks()));

  // Construct the system and components
  mySystem = make_unique<System>(myOSystem.random(), *my6502, *myRiot, *myTIA, *myCart);

  // The real controllers for this console will be added later
  // For now, we just add dummy joystick controllers, since autodetection
  // runs the emulation for a while, and this may interfere with 'smart'
  // controllers such as the AVox and SaveKey
  myLeftControl  = make_unique<Joystick>(Controller::Jack::Left, myEvent, *mySystem);
  myRightControl = make_unique<Joystick>(Controller::Jack::Right, myEvent, *mySystem);

  // Let the cart know how to query for the 'Cartridge.StartBank' property
  myCart->setStartBankFromPropsFunc([this]() {
    const string& startbank = myProperties.get(PropType::Cart_StartBank);
    return (startbank == EmptyString || BSPF::equalsIgnoreCase(startbank, "AUTO"))
        ? -1 : BSPF::stringToInt(startbank);
  });

  // We can only initialize after all the devices/components have been created
  mySystem->initialize();

  // Auto-detect NTSC/PAL mode if it's requested
  string autodetected = "";
  myDisplayFormat = myProperties.get(PropType::Display_Format);

  if (myDisplayFormat == "AUTO")
    myDisplayFormat = formatFromFilename();

  // Add the real controllers for this system
  // This must be done before the debugger is initialized
  const string& md5 = myProperties.get(PropType::Cart_MD5);
  setControllers(md5);

  // Mute audio and clear framebuffer while autodetection runs
  myOSystem.sound().mute(1);
  myOSystem.frameBuffer().clear();

  if(myDisplayFormat == "AUTO" || myOSystem.settings().getBool("rominfo"))
  {
    autodetectFrameLayout();

    if(myProperties.get(PropType::Display_Format) == "AUTO")
    {
      autodetected = "*";
      myCurrentFormat = 0;
      myFormatAutodetected = true;
    }
  }

  myConsoleInfo.DisplayFormat = myDisplayFormat + autodetected;

  // Set up the correct properties used when toggling format
  // Note that this can be overridden if a format is forced
  //   For example, if a PAL ROM is forced to be NTSC, it will use NTSC-like
  //   properties (60Hz, 262 scanlines, etc), but likely result in flicker
  if(myDisplayFormat == "NTSC")
  {
    myCurrentFormat = 1;
  }
  else if(myDisplayFormat == "PAL")
  {
    myCurrentFormat = 2;
  }
  else if(myDisplayFormat == "SECAM")
  {
    myCurrentFormat = 3;
  }
  else if(myDisplayFormat == "NTSC50")
  {
    myCurrentFormat = 4;
  }
  else if(myDisplayFormat == "PAL60")
  {
    myCurrentFormat = 5;
  }
  else if(myDisplayFormat == "SECAM60")
  {
    myCurrentFormat = 6;
  }
  setConsoleTiming();

  setTIAProperties();

  bool joyallow4 = myOSystem.settings().getBool("joyallow4");
  myOSystem.eventHandler().allowAllDirections(joyallow4);

  // Reset the system to its power-on state
  mySystem->reset();
  myRiot->update();

  // Finally, add remaining info about the console
  myConsoleInfo.CartName   = myProperties.get(PropType::Cart_Name);
  myConsoleInfo.CartMD5    = myProperties.get(PropType::Cart_MD5);
  bool swappedPorts = properties().get(PropType::Console_SwapPorts) == "YES";
  myConsoleInfo.Control0   = myLeftControl->about(swappedPorts);
  myConsoleInfo.Control1   = myRightControl->about(swappedPorts);
  myConsoleInfo.BankSwitch = myCart->about();

  // Some carts have an associated nvram file
  myCart->setNVRamFile(myOSystem.nvramDir(), myConsoleInfo.CartName);

  // Let the other devices know about the new console
  mySystem->consoleChanged(myConsoleTiming);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console::~Console()
{
  // Some smart controllers need to be informed that the console is going away
  myLeftControl->close();
  myRightControl->close();

  // Close audio to prevent invalid access to myConsoleTiming from the audio
  // callback
  myOSystem.sound().close();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setConsoleTiming()
{
  if (myDisplayFormat == "NTSC" || myDisplayFormat == "NTSC50")
  {
    myConsoleTiming = ConsoleTiming::ntsc;
  }
  else if (myDisplayFormat == "PAL" || myDisplayFormat == "PAL60")
  {
    myConsoleTiming = ConsoleTiming::pal;
  }
  else if (myDisplayFormat == "SECAM" || myDisplayFormat == "SECAM60")
  {
    myConsoleTiming = ConsoleTiming::secam;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::autodetectFrameLayout(bool reset)
{
  // Run the TIA, looking for PAL scanline patterns
  // We turn off the SuperCharger progress bars, otherwise the SC BIOS
  // will take over 250 frames!
  // The 'fastscbios' option must be changed before the system is reset
  bool fastscbios = myOSystem.settings().getBool("fastscbios");
  myOSystem.settings().setValue("fastscbios", true);

  FrameLayoutDetector frameLayoutDetector;
  myTIA->setFrameManager(&frameLayoutDetector);

  if (reset) {
    mySystem->reset(true);
    myRiot->update();
  }

  for(int i = 0; i < 60; ++i) myTIA->update();

  myTIA->setFrameManager(myFrameManager.get());

  myDisplayFormat = frameLayoutDetector.detectedLayout() == FrameLayout::pal ? "PAL" : "NTSC";

  // Don't forget to reset the SC progress bars again
  myOSystem.settings().setValue("fastscbios", fastscbios);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::redetectFrameLayout()
{
  Serializer s;

  myOSystem.sound().close();
  save(s);

  autodetectFrameLayout(false);

  load(s);
  initializeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Console::formatFromFilename() const
{
  static const BSPF::array2D<string, 6, 2> Pattern = {{
    { R"([ _\-(\[<]+NTSC[ _-]?50)",     "NTSC50"  },
    { R"([ _\-(\[<]+PAL[ _-]?60)",      "PAL60"   },
    { R"([ _\-(\[<]+SECAM[ _-]?60)",    "SECAM60" },
    { R"([ _\-(\[<]+NTSC[ _\-)\]>]?)",  "NTSC"    },
    { R"([ _\-(\[<]+PAL[ _\-)\]>]?)",   "PAL"     },
    { R"([ _\-(\[<]+SECAM[ _\-)\]>]?)", "SECAM"   }
  }};

  // Get filename *without* extension, and search using regex's above
  const string& filename = myOSystem.romFile().getNameWithExt("");
  for(size_t i = 0; i < Pattern.size(); ++i)
  {
    try
    {
      std::regex rgx(Pattern[i][0], std::regex_constants::icase);
      if(std::regex_search(filename, rgx))
        return Pattern[i][1];
    }
    catch(...)
    {
      continue;
    }
  }

  // Nothing found
  return "AUTO";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Console::save(Serializer& out) const
{
  try
  {
    // First save state for the system
    if(!mySystem->save(out))
      return false;

    // Now save the console controllers and switches
    if(!(myLeftControl->save(out) && myRightControl->save(out) &&
         mySwitches->save(out)))
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: Console::save" << endl;
    return false;
  }

  return true;  // success
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Console::load(Serializer& in)
{
  try
  {
    // First load state for the system
    if(!mySystem->load(in))
      return false;

    // Then load the console controllers and switches
    if(!(myLeftControl->load(in) && myRightControl->load(in) &&
         mySwitches->load(in)))
      return false;
  }
  catch(...)
  {
    cerr << "ERROR: Console::load" << endl;
    return false;
  }

  return true;  // success
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleFormat(int direction)
{
  string saveformat, message;
  uInt32 format = myCurrentFormat;

  if(direction == 1)
    format = (myCurrentFormat + 1) % 7;
  else if(direction == -1)
    format = myCurrentFormat > 0 ? (myCurrentFormat - 1) : 6;

  setFormat(format);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setFormat(uInt32 format)
{
  if(myCurrentFormat == format)
    return;

  string saveformat, message;
  string autodetected = "";

  myCurrentFormat = format;
  switch(myCurrentFormat)
  {
    case 0:  // auto-detect
    {
      if (myFormatAutodetected) return;

      myDisplayFormat = formatFromFilename();
      if (myDisplayFormat == "AUTO")
      {
        redetectFrameLayout();
        myFormatAutodetected = true;
        autodetected = "*";
        message = "Auto-detect mode: " + myDisplayFormat;
      }
      else
      {
        message = myDisplayFormat + " mode";
      }
      saveformat = "AUTO";
      setConsoleTiming();
      break;
    }
    case 1:
      saveformat = myDisplayFormat = "NTSC";
      myConsoleTiming = ConsoleTiming::ntsc;
      message = "NTSC mode";
      myFormatAutodetected = false;
      break;
    case 2:
      saveformat = myDisplayFormat = "PAL";
      myConsoleTiming = ConsoleTiming::pal;
      message = "PAL mode";
      myFormatAutodetected = false;
      break;
    case 3:
      saveformat = myDisplayFormat = "SECAM";
      myConsoleTiming = ConsoleTiming::secam;
      message = "SECAM mode";
      myFormatAutodetected = false;
      break;
    case 4:
      saveformat = myDisplayFormat = "NTSC50";
      myConsoleTiming = ConsoleTiming::ntsc;
      message = "NTSC50 mode";
      myFormatAutodetected = false;
      break;
    case 5:
      saveformat = myDisplayFormat = "PAL60";
      myConsoleTiming = ConsoleTiming::pal;
      message = "PAL60 mode";
      myFormatAutodetected = false;
      break;
    case 6:
      saveformat = myDisplayFormat = "SECAM60";
      myConsoleTiming = ConsoleTiming::secam;
      message = "SECAM60 mode";
      myFormatAutodetected = false;
      break;
  }
  myProperties.set(PropType::Display_Format, saveformat);

  myConsoleInfo.DisplayFormat = myDisplayFormat + autodetected;

  myPaletteHandler->setPalette();
  setTIAProperties();
  initializeVideo();  // takes care of refreshing the screen
  initializeAudio(); // ensure that audio synthesis is set up to match emulation speed
  myOSystem.resetFps(); // Reset FPS measurement

  myOSystem.frameBuffer().showMessage(message);

  // Let the other devices know about the console change
  mySystem->consoleChanged(myConsoleTiming);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleColorLoss()
{
  bool colorloss = !myTIA->colorLossEnabled();
  if(myTIA->enableColorLoss(colorloss))
  {
    myOSystem.settings().setValue(
      myOSystem.settings().getBool("dev.settings") ? "dev.colorloss" : "plr.colorloss", colorloss);

    string message = string("PAL color-loss ") +
                     (colorloss ? "enabled" : "disabled");
    myOSystem.frameBuffer().showMessage(message);
  }
  else
    myOSystem.frameBuffer().showMessage(
      "PAL color-loss not available in non PAL modes");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::enableColorLoss(bool state)
{
  myTIA->enableColorLoss(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleInter()
{
  bool enabled = myOSystem.settings().getBool("tia.inter");

  myOSystem.settings().setValue("tia.inter", !enabled);

  // ... and apply potential setting changes to the TIA surface
  myOSystem.frameBuffer().tiaSurface().updateSurfaceSettings();
  ostringstream ss;

  ss << "Interpolation " << (!enabled ? "enabled" : "disabled");
  myOSystem.frameBuffer().showMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleTurbo()
{
  bool enabled = myOSystem.settings().getBool("turbo");

  myOSystem.settings().setValue("turbo", !enabled);

  // update speed
  initializeAudio();

  // update VSync
  initializeVideo();

  ostringstream ss;
  ss << "Turbo mode " << (!enabled ? "enabled" : "disabled");
  myOSystem.frameBuffer().showMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::togglePhosphor()
{
  if(myOSystem.frameBuffer().tiaSurface().phosphorEnabled())
  {
    myProperties.set(PropType::Display_Phosphor, "NO");
    myOSystem.frameBuffer().tiaSurface().enablePhosphor(false);
    myOSystem.frameBuffer().showMessage("Phosphor effect disabled");
  }
  else
  {
    myProperties.set(PropType::Display_Phosphor, "YES");
    myOSystem.frameBuffer().tiaSurface().enablePhosphor(true);
    myOSystem.frameBuffer().showMessage("Phosphor effect enabled");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changePhosphor(int direction)
{
  int blend = BSPF::stringToInt(myProperties.get(PropType::Display_PPBlend));

  if(direction == +1)       // increase blend
  {
    if(blend >= 100)
    {
      myOSystem.frameBuffer().showMessage("Phosphor blend at maximum");
      myOSystem.frameBuffer().tiaSurface().enablePhosphor(true, 100);
      return;
    }
    else
      blend = std::min(blend+2, 100);
  }
  else if(direction == -1)  // decrease blend
  {
    if(blend <= 2)
    {
      myOSystem.frameBuffer().showMessage("Phosphor blend at minimum");
      myOSystem.frameBuffer().tiaSurface().enablePhosphor(true, 0);
      return;
    }
    else
      blend = std::max(blend-2, 0);
  }
  else
    return;

  ostringstream val;
  val << blend;
  myProperties.set(PropType::Display_PPBlend, val.str());
  myOSystem.frameBuffer().showMessage("Phosphor blend " + val.str());
  myOSystem.frameBuffer().tiaSurface().enablePhosphor(true, blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setProperties(const Properties& props)
{
  myProperties = props;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus Console::initializeVideo(bool full)
{
  FBInitStatus fbstatus = FBInitStatus::Success;

  if(full)
  {
    bool devSettings = myOSystem.settings().getBool("dev.settings");
    const string& title = string("Stella ") + STELLA_VERSION +
                   ": \"" + myProperties.get(PropType::Cart_Name) + "\"";
    fbstatus = myOSystem.frameBuffer().createDisplay(title, FrameBuffer::BufferType::Emulator,
        TIAConstants::viewableWidth, TIAConstants::viewableHeight, false);
    if(fbstatus != FBInitStatus::Success)
      return fbstatus;

    myOSystem.frameBuffer().showFrameStats(
      myOSystem.settings().getBool(devSettings ? "dev.stats" : "plr.stats"));
    myPaletteHandler->generatePalettes();
  }
  myPaletteHandler->setPalette();

  return fbstatus;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::initializeAudio()
{
  myOSystem.sound().close();

  myEmulationTiming
    .updatePlaybackRate(myAudioSettings.sampleRate())
    .updatePlaybackPeriod(myAudioSettings.fragmentSize())
    .updateAudioQueueExtraFragments(myAudioSettings.bufferSize())
    .updateAudioQueueHeadroom(myAudioSettings.headroom())
    .updateSpeedFactor(myOSystem.settings().getBool("turbo")
      ? 20.0F
      : myOSystem.settings().getFloat("speed"));

  createAudioQueue();
  myTIA->setAudioQueue(myAudioQueue);

  myOSystem.sound().open(myAudioQueue, &myEmulationTiming);
}

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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::fry() const
{
  for(int i = 0; i < 0x100; i += mySystem->randGenerator().next() % 4)
    mySystem->poke(i, mySystem->peek(i) & mySystem->randGenerator().next());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeVerticalCenter(int direction)
{
  Int32 vcenter = myTIA->vcenter();

  if(direction == +1)       // increase vcenter
  {
    if(vcenter >= myTIA->maxVcenter())
    {
      myOSystem.frameBuffer().showMessage("V-Center at maximum");
      return;
    }
    ++vcenter;
  }
  else if(direction == -1)  // decrease vcenter
  {
    if (vcenter <= myTIA->minVcenter())
    {
      myOSystem.frameBuffer().showMessage("V-Center at minimum");
      return;
    }
    --vcenter;
  }
  else
    return;

  ostringstream ss;
  ss << vcenter;

  myProperties.set(PropType::Display_VCenter, ss.str());
  if (vcenter != myTIA->vcenter()) myTIA->setVcenter(vcenter);

  ss.str("");
  ss << "V-Center ";
  if (!vcenter)
    ss << "default";
  else
    ss << (vcenter > 0 ? "+" : "") << vcenter << "px";

  myOSystem.frameBuffer().showMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::updateVcenter(Int32 vcenter)
{
  if ((vcenter > TIAConstants::maxVcenter) || (vcenter < TIAConstants::minVcenter))
    return;

  if (vcenter != myTIA->vcenter()) myTIA->setVcenter(vcenter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::changeScanlineAdjust(int direction)
{
  Int32 newAdjustVSize = myTIA->adjustVSize();

  if (direction != -1 && direction != +1) return;

  if(direction == +1)       // increase scanline adjustment
  {
    if (newAdjustVSize >= 5)
    {
      myOSystem.frameBuffer().showMessage("V-Size at maximum");
      return;
    }
    newAdjustVSize++;
  }
  else if(direction == -1)  // decrease scanline adjustment
  {
    if (newAdjustVSize <= -5)
    {
      myOSystem.frameBuffer().showMessage("V-Size at minimum");
      return;
    }
    newAdjustVSize--;
  }

  if (newAdjustVSize != myTIA->adjustVSize()) {
      myTIA->setAdjustVSize(newAdjustVSize);
      myOSystem.settings().setValue("tia.vsizeadjust", newAdjustVSize);
      initializeVideo();
  }

  ostringstream ss;

  ss << "V-Size ";
  if (!newAdjustVSize)
    ss << "default";
  else
    ss << (newAdjustVSize > 0 ? "+" : "") << newAdjustVSize << "%";

  myOSystem.frameBuffer().showMessage(ss.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setTIAProperties()
{
  Int32 vcenter = BSPF::clamp(
    static_cast<Int32>(BSPF::stringToInt(myProperties.get(PropType::Display_VCenter))), TIAConstants::minVcenter, TIAConstants::maxVcenter
  );

  if(myDisplayFormat == "NTSC" || myDisplayFormat == "PAL60" ||
     myDisplayFormat == "SECAM60")
  {
    // Assume we've got ~262 scanlines (NTSC-like format)
    myTIA->setLayout(FrameLayout::ntsc);
  }
  else
  {
    // Assume we've got ~312 scanlines (PAL-like format)
    myTIA->setLayout(FrameLayout::pal);
  }

  myTIA->setAdjustVSize(myOSystem.settings().getInt("tia.vsizeadjust"));
  myTIA->setVcenter(vcenter);

  myEmulationTiming.updateFrameLayout(myTIA->frameLayout());
  myEmulationTiming.updateConsoleTiming(myConsoleTiming);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::createAudioQueue()
{
  bool useStereo = myOSystem.settings().getBool(AudioSettings::SETTING_STEREO)
    || myProperties.get(PropType::Cart_Sound) == "STEREO";

  myAudioQueue = make_shared<AudioQueue>(
    myEmulationTiming.audioFragmentSize(),
    myEmulationTiming.audioQueueCapacity(),
    useStereo
  );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::setControllers(const string& romMd5)
{
  // Check for CompuMate scheme; it is special in that a handler creates both
  // controllers for us, and associates them with the bankswitching class
  if(myCart->detectedType() == "CM")
  {
    myCMHandler = make_shared<CompuMate>(*this, myEvent, *mySystem);

    // A somewhat ugly bit of code that casts to CartridgeCM to
    // add the CompuMate, and then back again for the actual
    // Cartridge
    unique_ptr<CartridgeCM> cartcm(static_cast<CartridgeCM*>(myCart.release()));
    cartcm->setCompuMate(myCMHandler);
    myCart = std::move(cartcm);

    myLeftControl  = std::move(myCMHandler->leftController());
    myRightControl = std::move(myCMHandler->rightController());
    myOSystem.eventHandler().defineKeyControllerMappings(Controller::Type::CompuMate, Controller::Jack::Left);
    myOSystem.eventHandler().defineJoyControllerMappings(Controller::Type::CompuMate, Controller::Jack::Left);
  }
  else
  {
    // Setup the controllers based on properties
    Controller::Type leftType = Controller::getType(myProperties.get(PropType::Controller_Left));
    Controller::Type rightType = Controller::getType(myProperties.get(PropType::Controller_Right));
    size_t size = 0;
    const uInt8* image = myCart->getImage(size);
    const bool swappedPorts = myProperties.get(PropType::Console_SwapPorts) == "YES";

    // Try to detect controllers
    if(image != nullptr && size != 0)
    {
      Logger::debug(myProperties.get(PropType::Cart_Name) + ":");
      leftType = ControllerDetector::detectType(image, size, leftType,
          !swappedPorts ? Controller::Jack::Left : Controller::Jack::Right, myOSystem.settings());
      rightType = ControllerDetector::detectType(image, size, rightType,
          !swappedPorts ? Controller::Jack::Right : Controller::Jack::Left, myOSystem.settings());
    }

    unique_ptr<Controller> leftC = getControllerPort(leftType, Controller::Jack::Left, romMd5),
      rightC = getControllerPort(rightType, Controller::Jack::Right, romMd5);

    // Swap the ports if necessary
    if(!swappedPorts)
    {
      myLeftControl = std::move(leftC);
      myRightControl = std::move(rightC);
    }
    else
    {
      myLeftControl = std::move(rightC);
      myRightControl = std::move(leftC);
    }
  }

  myTIA->bindToControllers();

  // now that we know the controllers, enable the event mappings
  myOSystem.eventHandler().enableEmulationKeyMappings();
  myOSystem.eventHandler().enableEmulationJoyMappings();

  myOSystem.eventHandler().setMouseControllerMode(myOSystem.settings().getString("usemouse"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Controller> Console::getControllerPort(const Controller::Type type,
                                                  const Controller::Jack port, const string& romMd5)
{
  unique_ptr<Controller> controller;

  myOSystem.eventHandler().defineKeyControllerMappings(type, port);
  myOSystem.eventHandler().defineJoyControllerMappings(type, port);

  switch(type)
  {
    case Controller::Type::BoosterGrip:
      controller = make_unique<BoosterGrip>(port, myEvent, *mySystem);
      break;

    case Controller::Type::Driving:
      controller = make_unique<Driving>(port, myEvent, *mySystem);
      break;

    case Controller::Type::Keyboard:
      controller = make_unique<Keyboard>(port, myEvent, *mySystem);
      break;

    case Controller::Type::Paddles:
    case Controller::Type::PaddlesIAxis:
    case Controller::Type::PaddlesIAxDr:
    {
      // Also check if we should swap the paddles plugged into a jack
      bool swapPaddles = myProperties.get(PropType::Controller_SwapPaddles) == "YES";
      bool swapAxis = false, swapDir = false;
      if(type == Controller::Type::PaddlesIAxis)
        swapAxis = true;
      else if(type == Controller::Type::PaddlesIAxDr)
        swapAxis = swapDir = true;

      Paddles::setAnalogXCenter(BSPF::stringToInt(myProperties.get(PropType::Controller_PaddlesXCenter)));
      Paddles::setAnalogYCenter(BSPF::stringToInt(myProperties.get(PropType::Controller_PaddlesYCenter)));
      Paddles::setAnalogSensitivity(myOSystem.settings().getInt("psense"));

      controller = make_unique<Paddles>(port, myEvent, *mySystem,
                                        swapPaddles, swapAxis, swapDir);
      break;
    }
    case Controller::Type::AmigaMouse:
      controller = make_unique<AmigaMouse>(port, myEvent, *mySystem);
      break;

    case Controller::Type::AtariMouse:
      controller = make_unique<AtariMouse>(port, myEvent, *mySystem);
      break;

    case Controller::Type::TrakBall:
      controller = make_unique<TrakBall>(port, myEvent, *mySystem);
      break;

    case Controller::Type::AtariVox:
    {
      const string& nvramfile = myOSystem.nvramDir() + "atarivox_eeprom.dat";
      Controller::onMessageCallback callback = [&os = myOSystem](const string& msg) {
        bool devSettings = os.settings().getBool("dev.settings");
        if(os.settings().getBool(devSettings ? "dev.eepromaccess" : "plr.eepromaccess"))
          os.frameBuffer().showMessage(msg);
      };
      controller = make_unique<AtariVox>(port, myEvent, *mySystem,
          myOSystem.settings().getString("avoxport"), nvramfile, callback);
      break;
    }
    case Controller::Type::SaveKey:
    {
      const string& nvramfile = myOSystem.nvramDir() + "savekey_eeprom.dat";
      Controller::onMessageCallback callback = [&os = myOSystem](const string& msg) {
        bool devSettings = os.settings().getBool("dev.settings");
        if(os.settings().getBool(devSettings ? "dev.eepromaccess" : "plr.eepromaccess"))
          os.frameBuffer().showMessage(msg);
      };
      controller = make_unique<SaveKey>(port, myEvent, *mySystem, nvramfile, callback);
      break;
    }
    case Controller::Type::Genesis:
      controller = make_unique<Genesis>(port, myEvent, *mySystem);
      break;

    case Controller::Type::KidVid:
      controller = make_unique<KidVid>(port, myEvent, *mySystem, romMd5);
      break;

    case Controller::Type::MindLink:
      controller = make_unique<MindLink>(port, myEvent, *mySystem);
      break;

    case Controller::Type::Lightgun:
      controller = make_unique<Lightgun>(port, myEvent, *mySystem, romMd5, myOSystem.frameBuffer());
      break;

    default:
      // What else can we do?
      // always create because it may have been changed by user dialog
      controller = make_unique<Joystick>(port, myEvent, *mySystem);
  }

  return controller;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Console::getFramerate() const
{
  return
    (myConsoleTiming == ConsoleTiming::ntsc ? 262.F * 60.F : 312.F * 50.F) /
     myTIA->frameBufferScanlinesLastFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleTIABit(TIABit bit, const string& bitname, bool show) const
{
  bool result = myTIA->toggleBit(bit);
  string message = bitname + (result ? " enabled" : " disabled");
  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleBits() const
{
  bool enabled = myTIA->toggleBits();
  string message = string("TIA bits") + (enabled ? " enabled" : " disabled");
  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleTIACollision(TIABit bit, const string& bitname, bool show) const
{
  bool result = myTIA->toggleCollision(bit);
  string message = bitname + (result ? " collision enabled" : " collision disabled");
  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleCollisions() const
{
  bool enabled = myTIA->toggleCollisions();
  string message = string("TIA collisions") + (enabled ? " enabled" : " disabled");
  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleFixedColors() const
{
  if(myTIA->toggleFixedColors())
    myOSystem.frameBuffer().showMessage("Fixed debug colors enabled");
  else
    myOSystem.frameBuffer().showMessage("Fixed debug colors disabled");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::toggleJitter() const
{
  bool enabled = myTIA->toggleJitter();
  string message = string("TV scanline jitter") + (enabled ? " enabled" : " disabled");
  myOSystem.frameBuffer().showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::attachDebugger(Debugger& dbg)
{
#ifdef DEBUGGER_SUPPORT
//  myOSystem.createDebugger(*this);
  mySystem->m6502().attach(dbg);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Console::stateChanged(EventHandlerState state)
{
  // only the CompuMate used to care about state changes
}
