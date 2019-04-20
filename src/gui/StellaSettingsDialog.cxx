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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Console.hxx"
#include "EventHandler.hxx"
#include "Launcher.hxx"
#include "PropsSet.hxx"
#include "ControllerDetector.hxx"
#include "NTSCFilter.hxx"
#include "PopUpWidget.hxx"

#include "StellaSettingsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaSettingsDialog::StellaSettingsDialog(OSystem& osystem, DialogContainer& parent,
  const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Stella settings")
{
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 10;
  const int INDENT = 20;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
  int xpos, ypos;

  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  setSize(33 * fontWidth + HBORDER * 2, 15 * (lineHeight + VGAP) + _th, max_w, max_h);

  xpos = HBORDER;
  ypos = VBORDER + _th;

  new StaticTextWidget(this, font, xpos, ypos + 1, "Global settings:");
  xpos += INDENT;
  ypos += lineHeight + VGAP;

  addUIOptions(wid, xpos, ypos, font);
  ypos += VGAP * 4;
  addVideoOptions(wid, xpos, ypos, font);
  ypos += VGAP * 4;

  xpos -= INDENT;
  myGameSettings = new StaticTextWidget(this, font, xpos, ypos + 1, "Game settings:");
  xpos += INDENT;
  ypos += lineHeight + VGAP;

  addGameOptions(wid, xpos, ypos, font);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::addUIOptions(WidgetArray& wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int lineHeight = font.getLineHeight();
  VariantList items;
  int pwidth = font.getStringWidth("Bad adjust");

  ypos += 1;
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  myThemePopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight, items, "UI theme ");
  wid.push_back(myThemePopup);
  ypos += lineHeight + VGAP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::addVideoOptions(WidgetArray& wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int INDENT = 20;
  const int lineHeight = font.getLineHeight(),
    fontWidth = font.getMaxCharWidth();
  VariantList items;

  // TV effects options
  int swidth = font.getMaxCharWidth() * 8 - 4;

  // TV Mode
  items.clear();
  VarList::push_back(items, "Disabled", static_cast<uInt32>(NTSCFilter::Preset::OFF));
  VarList::push_back(items, "RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  VarList::push_back(items, "S-Video", static_cast<uInt32>(NTSCFilter::Preset::SVIDEO));
  VarList::push_back(items, "Composite", static_cast<uInt32>(NTSCFilter::Preset::COMPOSITE));
  VarList::push_back(items, "Bad adjust", static_cast<uInt32>(NTSCFilter::Preset::BAD));
  int lwidth = font.getStringWidth("TV mode  ");
  int pwidth = font.getStringWidth("Bad adjust");

  myTVMode = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
    items, "TV mode ", lwidth);
  wid.push_back(myTVMode);
  ypos += lineHeight + VGAP * 2;

  lwidth = font.getStringWidth("Intensity ");
  swidth = font.getMaxCharWidth() * 10;

  // Scanline intensity
  myTVScanlines = new StaticTextWidget(this, font, xpos, ypos + 1, "Scanlines:");
  ypos += lineHeight;
  myTVScanIntense = new SliderWidget(this, font, xpos + INDENT, ypos-1, swidth, lineHeight,
    "Intensity ", lwidth, kScanlinesChanged, fontWidth * 3);
  myTVScanIntense->setMinValue(0); myTVScanIntense->setMaxValue(10);
  myTVScanIntense->setTickmarkInterval(2);
  wid.push_back(myTVScanIntense);
  ypos += lineHeight + VGAP;

  // TV Phosphor effect
  new StaticTextWidget(this, font, xpos, ypos + 1, "Phosphor effect:");
  ypos += lineHeight;
  // TV Phosphor blend level
  myTVPhosLevel = new SliderWidget(this, font, xpos + INDENT, ypos-1, swidth, lineHeight,
    "Blend     ", lwidth, kPhosphorChanged, fontWidth * 3);
  myTVPhosLevel->setMinValue(0); myTVPhosLevel->setMaxValue(10);
  myTVPhosLevel->setTickmarkInterval(2);
  wid.push_back(myTVPhosLevel);
  ypos += lineHeight + VGAP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::addGameOptions(WidgetArray& wid, int& xpos, int& ypos, const GUI::Font& font)
{
  const int VGAP = 4;
  const int lineHeight = font.getLineHeight();
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  VariantList ctrls;

  ctrls.clear();
  VarList::push_back(ctrls, "Auto-detect", "AUTO");
  VarList::push_back(ctrls, "Joystick", "JOYSTICK");
  VarList::push_back(ctrls, "Paddles", "PADDLES");
  VarList::push_back(ctrls, "BoosterGrip", "BOOSTERGRIP");
  VarList::push_back(ctrls, "Driving", "DRIVING");
  VarList::push_back(ctrls, "Keyboard", "KEYBOARD");
  VarList::push_back(ctrls, "AmigaMouse", "AMIGAMOUSE");
  VarList::push_back(ctrls, "AtariMouse", "ATARIMOUSE");
  VarList::push_back(ctrls, "Trakball", "TRAKBALL");
  VarList::push_back(ctrls, "Sega Genesis", "GENESIS");

  int pwidth = font.getStringWidth("Sega Genesis");
  myLeftPortLabel = new StaticTextWidget(this, font, xpos, ypos + 1, "Left port  ");
  myLeftPort = new PopUpWidget(this, font, myLeftPortLabel->getRight(),
    myLeftPortLabel->getTop() - 1, pwidth, lineHeight, ctrls, "");
  wid.push_back(myLeftPort);
  ypos += lineHeight + VGAP;

  myLeftPortDetected = new StaticTextWidget(this, ifont, myLeftPort->getLeft(), ypos,
    "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP;

  myRightPortLabel = new StaticTextWidget(this, font, xpos, ypos + 1, "Right port ");
  myRightPort = new PopUpWidget(this, font, myRightPortLabel->getRight(),
    myRightPortLabel->getTop() - 1, pwidth, lineHeight, ctrls, "");
  wid.push_back(myRightPort);
  ypos += lineHeight + VGAP;
  myRightPortDetected = new StaticTextWidget(this, ifont, myRightPort->getLeft(), ypos,
    "Sega Genesis detected");
  ypos += ifont.getLineHeight() + VGAP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // UI palette
  const string& theme = settings.getString("uipalette");
  myThemePopup->setSelected(theme, "standard");

  // TV Mode
  myTVMode->setSelected(
    settings.getString("tv.filter"), "0");

  // TV scanline intensity
  myTVScanIntense->setValue(valueToLevel(settings.getInt("tv.scanlines")));

  // TV phosphor blend
  myTVPhosLevel->setValue(valueToLevel(settings.getInt("tv.phosblend")));

  // Controllers
  Properties props;

  if (instance().hasConsole())
  {
    myGameProperties = instance().console().properties();
  }
  else
  {
    const string& md5 = instance().launcher().selectedRomMD5();
    instance().propSet().getMD5(md5, myGameProperties);
  }
  loadControllerProperties(myGameProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // UI palette
  settings.setValue("uipalette",
    myThemePopup->getSelectedTag().toString());
  instance().frameBuffer().setUIPalette();

  // TV Mode
  instance().settings().setValue("tv.filter",
    myTVMode->getSelectedTag().toString());

  // TV phosphor mode
  instance().settings().setValue("tv.phosphor",
    myTVPhosLevel->getValue() > 0 ? "always" : "byrom");
  // TV phosphor blend
  instance().settings().setValue("tv.phosblend",
    levelToValue(myTVPhosLevel->getValue()));

  // TV scanline intensity and interpolation
  instance().settings().setValue("tv.scanlines",
    levelToValue(myTVScanIntense->getValue()));

  // Controller properties
  myGameProperties.set(PropType::Controller_Left, myLeftPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Controller_Right, myRightPort->getSelectedTag().toString());

  // Always insert; if the properties are already present, nothing will happen
  instance().propSet().insert(myGameProperties);
  instance().saveConfig();

  // In any event, inform the Console
  if (instance().hasConsole())
  {
    instance().console().setProperties(myGameProperties);
  }

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::setDefaults()
{
  // UI Theme
  myThemePopup->setSelected("standard");

  // TV effects
  myTVMode->setSelected("0", "0");
  // TV scanline intensity
  myTVScanIntense->setValue(0); // = off
  // TV phosphor blend
  myTVPhosLevel->setValue(0); // = off

  // Load the default game properties
  Properties defaultProperties;
  const string& md5 = myGameProperties.get(PropType::Cart_MD5);

  instance().propSet().getMD5(md5, defaultProperties, true);

  loadControllerProperties(defaultProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::handleCommand(CommandSender* sender, int cmd,
  int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kScanlinesChanged:
      if(myTVScanIntense->getValue() == 0)
        myTVScanIntense->setValueLabel("Off");
      break;

    case kPhosphorChanged:
      if(myTVPhosLevel->getValue() == 0)
        myTVPhosLevel->setValueLabel("Off");
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::loadControllerProperties(const Properties& props)
{
  // Determine whether we should enable the "Game settings:"
  // We always enable it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  bool enable = false;

  // Note: The state returned seems not consistent here
  switch (instance().eventHandler().state())
  {
    case EventHandlerState::OPTIONSMENU: // game is running!
      enable = true;
      break;
    case EventHandlerState::LAUNCHER:
      enable = (instance().launcher().selectedRomMD5() != "");
      break;
    default:
      break;
  }

  myGameSettings->setEnabled(enable);
  myLeftPort->setEnabled(enable);
  myLeftPortLabel->setEnabled(enable);
  myLeftPortDetected->setEnabled(enable);
  myRightPort->setEnabled(enable);
  myRightPortLabel->setEnabled(enable);
  myRightPortDetected->setEnabled(enable);

  if (enable)
  {
    bool autoDetect = false;
    BytePtr image;
    string md5 = props.get(PropType::Cart_MD5);
    uInt32 size = 0;
    const FilesystemNode& node = FilesystemNode(instance().launcher().selectedRom());

    // try to load the image for auto detection
    if (!instance().hasConsole() &&
      node.exists() && !node.isDirectory() && (image = instance().openROM(node, md5, size)) != nullptr)
      autoDetect = true;

    string label = "";
    string controller = props.get(PropType::Controller_Left);
    bool swapPorts = props.get(PropType::Console_SwapPorts) == "YES";

    myLeftPort->setSelected(controller, "AUTO");
    if (myLeftPort->getSelectedTag().toString() == "AUTO")
    {
      if (instance().hasConsole())
        label = (!swapPorts ? instance().console().leftController().name()
          : instance().console().rightController().name()) + " detected";
      else if (autoDetect)
        label = ControllerDetector::detectName(image.get(), size, controller,
          !swapPorts ? Controller::Jack::Left : Controller::Jack::Right,
          instance().settings()) + " detected";
    }
    myLeftPortDetected->setLabel(label);

    label = "";
    controller = props.get(PropType::Controller_Right);

    myRightPort->setSelected(controller, "AUTO");
    if (myRightPort->getSelectedTag().toString() == "AUTO")
    {
      if (instance().hasConsole())
        label = (!swapPorts ? instance().console().rightController().name()
          : instance().console().leftController().name()) + " detected";
      else if (autoDetect)
        label = ControllerDetector::detectName(image.get(), size, controller,
          !swapPorts ? Controller::Jack::Right : Controller::Jack::Left,
          instance().settings()) + " detected";
    }
    myRightPortDetected->setLabel(label);
  }
  else
  {
    myLeftPort->clearSelection();
    myRightPort->clearSelection();
    myLeftPortDetected->setLabel("");
    myRightPortDetected->setLabel("");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int StellaSettingsDialog::levelToValue(int level)
{
  const int NUM_LEVELS = 11;
  uInt8 values[NUM_LEVELS] = { 0, 5, 11, 18, 26, 35, 45, 56, 68, 81, 95 };

  return values[std::min(level, NUM_LEVELS - 1)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int StellaSettingsDialog::valueToLevel(int value)
{
  const int NUM_LEVELS = 11;
  uInt8 values[NUM_LEVELS] = { 0, 5, 11, 18, 26, 35, 45, 56, 68, 81, 95 };

  for (uInt32 i = NUM_LEVELS - 1; i > 0; --i)
  {
    if (value >= values[i])
      return i;
  }
  return 0;
}
