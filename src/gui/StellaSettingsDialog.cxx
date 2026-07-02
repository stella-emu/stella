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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Launcher.hxx"
#include "PropsSet.hxx"
#include "ControllerDetector.hxx"
#include "NTSCFilter.hxx"
#include "PopUpWidget.hxx"
#include "MessageBox.hxx"
#include "TIASurface.hxx"
#include "Layout.hxx"

#include "StellaSettingsDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaSettingsDialog::StellaSettingsDialog(OSystem& osystem,
                                           DialogContainer& parent,
                                           AppMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Basic settings"),
    myMode{mode}
{
  const int buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("  Help  " + ELLIPSIS);
  WidgetArray wid;

  myAdvancedButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
    "Use Advanced Settings" + ELLIPSIS, kAdvancedSettings);
  wid.push_back(myAdvancedButton);
  myHelpButton = new ButtonWidget(this, _font, 0, 0, buttonWidth, buttonHeight,
    "Help" + ELLIPSIS, kHelp);
  wid.push_back(myHelpButton);

  myGlobalLabel = new StaticTextWidget(this, _font, 0, 0, "Global settings:");
  createUIOptions(wid);
  createVideoOptions(wid);

  myGameSettings = new StaticTextWidget(this, _font, 0, 0, "Game settings:");
  createGameOptions(wid);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, _font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StellaSettingsDialog::~StellaSettingsDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::createUIOptions(WidgetArray& wid)
{
  const int lineHeight = Dialog::lineHeight();
  VariantList items;
  const int pwidth = _font.getStringWidth("Right bottom"); // align width with other popup

  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Classic", "classic");
  VarList::push_back(items, "Light", "light");
  myThemePopup = new PopUpWidget(this, _font, 0, 0, pwidth, lineHeight, items,
    "UI theme           ");
  wid.push_back(myThemePopup);

  // Dialog position
  items.clear();
  VarList::push_back(items, "Centered", 0);
  VarList::push_back(items, "Left top", 1);
  VarList::push_back(items, "Right top", 2);
  VarList::push_back(items, "Right bottom", 3);
  VarList::push_back(items, "Left bottom", 4);
  myPositionPopup = new PopUpWidget(this, _font, 0, 0, pwidth, lineHeight,
    items, "Dialogs position   ");
  wid.push_back(myPositionPopup);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::createVideoOptions(WidgetArray& wid)
{
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth();
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  VariantList items;

  // TV effects options
  const int swidth = _font.getMaxCharWidth() * 11;

  // TV Mode
  VarList::push_back(items, "Disabled", static_cast<uInt32>(NTSCFilter::Preset::OFF));
  VarList::push_back(items, "RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  VarList::push_back(items, "S-Video", static_cast<uInt32>(NTSCFilter::Preset::SVIDEO));
  VarList::push_back(items, "Composite", static_cast<uInt32>(NTSCFilter::Preset::COMPOSITE));
  VarList::push_back(items, "Bad adjust", static_cast<uInt32>(NTSCFilter::Preset::BAD));
  const int pwidth = _font.getStringWidth("Right bottom");
  const int lwidth = _font.getStringWidth("Scanline intensity ");

  myTVMode = new PopUpWidget(this, _font, 0, 0, pwidth, lineHeight,
    items, "TV mode            ");
  wid.push_back(myTVMode);

  // Scanline intensity
  myTVScanIntense = new SliderWidget(this, _font, 0, 0, swidth, lineHeight,
    "Scanline intensity", lwidth, kScanlinesChanged, fontWidth * 3);
  myTVScanIntense->setMinValue(0); myTVScanIntense->setMaxValue(10);
  myTVScanIntense->setTickmarkIntervals(2);
  wid.push_back(myTVScanIntense);

  // TV Phosphor blend level
  myTVPhosLevel = new SliderWidget(this, _font, 0, 0, swidth, lineHeight,
    "Phosphor blend  ", lwidth, kPhosphorChanged, fontWidth * 3);
  myTVPhosLevel->setMinValue(0); myTVPhosLevel->setMaxValue(10);
  myTVPhosLevel->setTickmarkIntervals(2);
  wid.push_back(myTVPhosLevel);

  // FS overscan
  myTVOverscan = new SliderWidget(this, _font, 0, 0, swidth, lineHeight,
    "Overscan (*)    ", lwidth, kOverscanChanged, fontWidth * 3);
  myTVOverscan->setMinValue(0); myTVOverscan->setMaxValue(10);
  myTVOverscan->setTickmarkIntervals(2);
  wid.push_back(myTVOverscan);

  myOverscanInfo = new StaticTextWidget(this, ifont, 0, 0,
    "(*) Change requires launcher reboot");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::createGameOptions(WidgetArray& wid)
{
  const int lineHeight = Dialog::lineHeight();
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  VariantList ctrls;

  VarList::push_back(ctrls, "Auto-detect", "AUTO");
  VarList::push_back(ctrls, "Joystick", "JOYSTICK");
  VarList::push_back(ctrls, "Paddles", "PADDLES");
  VarList::push_back(ctrls, "Booster Grip", "BOOSTERGRIP");
  VarList::push_back(ctrls, "Driving", "DRIVING");
  VarList::push_back(ctrls, "Keyboard", "KEYBOARD");
  VarList::push_back(ctrls, "Amiga mouse", "AMIGAMOUSE");
  VarList::push_back(ctrls, "Atari mouse", "ATARIMOUSE");
  VarList::push_back(ctrls, "Trak-Ball", "TRAKBALL");
  VarList::push_back(ctrls, "Sega Genesis", "GENESIS");
  VarList::push_back(ctrls, "Joy2B+", "JOY_2B+"); // TODO: should work, but needs testing with real hardware
  VarList::push_back(ctrls, "QuadTari", "QUADTARI");

  const int pwidth = _font.getStringWidth("Sega Genesis");
  myLeftPortLabel = new StaticTextWidget(this, _font, 0, 0, "Left port  ");
  myLeftPort = new PopUpWidget(this, _font, 0, 0, pwidth, lineHeight,
    ctrls, "", 0, kLeftCChanged);
  wid.push_back(myLeftPort);
  myLeftPortDetected = new StaticTextWidget(this, ifont, 0, 0, "Sega Genesis detected");

  myRightPortLabel = new StaticTextWidget(this, _font, 0, 0, "Right port ");
  myRightPort = new PopUpWidget(this, _font, 0, 0, pwidth, lineHeight,
    ctrls, "", 0, kRightCChanged);
  wid.push_back(myRightPort);
  myRightPortDetected = new StaticTextWidget(this, ifont, 0, 0, "Sega Genesis detected");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::widgetItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const int iLineHeight = instance().frameBuffer().infoFont().getLineHeight();
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("  Help  " + ELLIPSIS),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap(),
            INDENT       = Dialog::indent();

  // Size the dialog from the current font so it reflows on font change
  _w = 35 * fontWidth + HBORDER * 2 + 5;
  _h = VBORDER * 2 + _th + 10 * (lineHeight + VGAP) + 3 * (iLineHeight + VGAP)
       + VGAP * 12 + buttonHeight * 2;

  // Top row: "Use Advanced Settings" fills the width, "Help" is fixed at right
  auto buttonRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  buttonRow->addStretch(widgetItem(myAdvancedButton));
  buttonRow->addSpace(8);
  buttonRow->addFixed(widgetItem(myHelpButton), buttonWidth);

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addFixed(std::move(buttonRow), buttonHeight);
  root->addSpace(VGAP * 2);

  // Global settings: header, then the indented UI and video options
  root->addFixed(anchoredItem(myGlobalLabel), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myThemePopup, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myPositionPopup, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addSpace(VGAP * 4);
  root->addFixed(indentedItem(myTVMode, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myTVScanIntense, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myTVPhosLevel, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myTVOverscan, INDENT), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myOverscanInfo, INDENT), iLineHeight);
  root->addSpace(VGAP);
  root->addSpace(VGAP * 4);

  // Game settings: header, then the indented controller port rows.  The port
  // pop-up sits directly right of its label; the "detected" line aligns under
  // the pop-up (indent = INDENT + label width).
  root->addFixed(anchoredItem(myGameSettings), lineHeight);
  root->addSpace(VGAP);

  auto portRow = [&](StaticTextWidget* label, PopUpWidget* popup) {
    auto box = std::make_unique<BoxLayout>(Dir::Horizontal);
    box->addSpace(INDENT);
    box->addFixed(anchoredItem(label), label->getWidth());
    box->addFixed(anchoredItem(popup), popup->getWidth());
    return box;
  };

  root->addFixed(portRow(myLeftPortLabel, myLeftPort), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myLeftPortDetected,
                              INDENT + myLeftPortLabel->getWidth()), iLineHeight);
  root->addSpace(VGAP);
  root->addFixed(portRow(myRightPortLabel, myRightPort), lineHeight);
  root->addSpace(VGAP);
  root->addFixed(indentedItem(myRightPortDetected,
                              INDENT + myRightPortLabel->getWidth()), iLineHeight);

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // UI palette
  const string& theme = settings.getString("uipalette");
  myThemePopup->setSelected(theme, "standard");
  // Dialog position
  myPositionPopup->setSelected(settings.getString("dialogpos"), "0");

  // TV Mode
  myTVMode->setSelected(
    settings.getString("tv.filter"), "0");

  // TV scanline intensity
  myTVScanIntense->setValue(valueToLevel(settings.getInt("tv.scanlines")));

  // TV phosphor blend
  myTVPhosLevel->setValue(valueToLevel(settings.getInt(PhosphorHandler::SETTING_BLEND)));

  // TV overscan
  myTVOverscan->setValue(settings.getInt("tia.fs_overscan"));

  handleOverscanChange();

  // Controllers
  if(instance().hasConsole())
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
  instance().frameBuffer().update(FrameBuffer::UpdateMode::REDRAW);

  // Dialog position
  settings.setValue("dialogpos", myPositionPopup->getSelectedTag().toString());

  // TV Mode
  instance().settings().setValue("tv.filter",
    myTVMode->getSelectedTag().toString());

  // TV phosphor mode
  instance().settings().setValue(PhosphorHandler::SETTING_MODE,
    myTVPhosLevel->getValue() > 0 ? PhosphorHandler::VALUE_ALWAYS : PhosphorHandler::VALUE_BYROM);
  // TV phosphor blend
  instance().settings().setValue(PhosphorHandler::SETTING_BLEND,
    levelToValue(myTVPhosLevel->getValue()));

  // TV scanline intensity and interpolation
  instance().settings().setValue("tv.scanlines",
    levelToValue(myTVScanIntense->getValue()));

  // TV overscan
  instance().settings().setValue("tia.fs_overscan", myTVOverscan->getValueLabel());

  // Controller properties
  myGameProperties.set(PropType::Controller_Left, myLeftPort->getSelectedTag().toString());
  myGameProperties.set(PropType::Controller_Right, myRightPort->getSelectedTag().toString());

  // Always insert; if the properties are already present, nothing will happen
  instance().propSet().insert(myGameProperties);
  instance().saveConfig();

  // In any event, inform the Console
  if(instance().hasConsole())
  {
    instance().console().setProperties(myGameProperties);
  }

  // Finally, issue a complete framebuffer re-initialization...
  instance().createFrameBuffer();

  // ... and apply potential setting changes to the TIA surface
  instance().frameBuffer().tiaSurface().updateSurfaceSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::setDefaults()
{
  // UI Theme
  myThemePopup->setSelected("standard");
  // Dialog position
  myPositionPopup->setSelected("0");

  // TV effects
  myTVMode->setSelected("RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  // TV scanline intensity
  myTVScanIntense->setValue(3); // 18
  // TV phosphor blend
  myTVPhosLevel->setValue(6); // = 45
  // TV overscan
  myTVOverscan->setValue(0);

  // Load the default game properties
  Properties defaultProperties;
  string_view md5 = myGameProperties.get(PropType::Cart_MD5);

  instance().propSet().getMD5(md5, defaultProperties, true);

  loadControllerProperties(defaultProperties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::handleCommand(CommandSender* sender, int cmd,
  int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case GuiObject::kOKCmd:
      saveConfig();
      [[fallthrough]];
    case GuiObject::kCloseCmd:
      if(myMode != AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    case kAdvancedSettings:
      switchSettingsMode();
      break;

    case kConfirmSwitchCmd:
      instance().settings().setValue("basic_settings", false);
      if(myMode != AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    case kHelp:
      openHelp();
      break;

    case kScanlinesChanged:
      if(myTVScanIntense->getValue() == 0)
        myTVScanIntense->setValueLabel("Off");
      break;

    case kPhosphorChanged:
      if(myTVPhosLevel->getValue() == 0)
        myTVPhosLevel->setValueLabel("Off");
      break;

    case kOverscanChanged:
      handleOverscanChange();
      break;

    case kLeftCChanged:
    case kRightCChanged:
      updateControllerStates();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::handleOverscanChange()
{
  if(myTVOverscan->getValue() == 0)
  {
    myTVOverscan->setValueLabel("Off");
    myTVOverscan->setValueUnit("");
  }
  else
    myTVOverscan->setValueUnit("%");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::switchSettingsMode()
{
  StringList msg;

  msg.emplace_back("Warning!");
  msg.emplace_back("");
  msg.emplace_back("Advanced settings should be");
  msg.emplace_back("handled with care! When in");
  msg.emplace_back("doubt, read the manual.");
  msg.emplace_back("");
  msg.emplace_back("If you are sure you want to");
  msg.emplace_back("proceed with the switch, click");
  msg.emplace_back("'OK', otherwise click 'Cancel'.");

  myConfirmMsg = std::make_unique<GUI::MessageBox>(this, _font, msg,
      _w-16, _h, kConfirmSwitchCmd, "OK", "Cancel", "Switch settings mode", false);
  myConfirmMsg->show();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::loadControllerProperties(const Properties& props)
{
  // Determine whether we should enable the "Game settings:"
  // We always enable it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  bool enable = false;

  switch(instance().eventHandler().state())
  {
    case EventHandlerState::LAUNCHER:
      enable = !instance().launcher().selectedRomMD5().empty();
      break;
    default:
      // Any in-game menu: enabled whenever a console is running behind it
      enable = instance().hasConsole();
      break;
  }

  myGameSettings->setEnabled(enable);
  myLeftPort->setEnabled(enable);
  myLeftPortLabel->setEnabled(enable);
  myLeftPortDetected->setEnabled(enable);
  myRightPort->setEnabled(enable);
  myRightPortLabel->setEnabled(enable);
  myRightPortDetected->setEnabled(enable);

  if(enable)
  {
    string controller{props.get(PropType::Controller_Left)};
    myLeftPort->setSelected(controller, "AUTO");
    controller = props.get(PropType::Controller_Right);
    myRightPort->setSelected(controller, "AUTO");

    updateControllerStates();
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
  static constexpr int NUM_LEVELS = 11;
  static constexpr std::array<uInt8, NUM_LEVELS> values = {
    0, 5, 11, 18, 26, 35, 45, 56, 68, 81, 95
  };

  return values[std::min(level, NUM_LEVELS - 1)];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int StellaSettingsDialog::valueToLevel(int value)
{
  static constexpr int NUM_LEVELS = 11;
  static constexpr std::array<uInt8, NUM_LEVELS> values = {
    0, 5, 11, 18, 26, 35, 45, 56, 68, 81, 95
  };

  for(int i = NUM_LEVELS - 1; i > 0; --i)
  {
    if(std::cmp_greater_equal(value, values[i]))
      return i;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::openHelp()
{
  // Create an help dialog, similar to the in-game one
  if(myHelpDialog == nullptr)
    myHelpDialog = std::make_unique<HelpDialog>(instance(), parent(), _font);
  myHelpDialog->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StellaSettingsDialog::updateControllerStates()
{
  ByteArray image;
  string md5{myGameProperties.get(PropType::Cart_MD5)};

  // try to load the image for auto detection
  if(!instance().hasConsole())
  {
    const FSNode& node = FSNode(instance().launcher().selectedRom());
    if(node.exists() && !node.isDirectory())
      image = instance().openROM(node, md5);
  }

  string label;
  Controller::Type type = Controller::getType(myLeftPort->getSelectedTag().toString());
  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
      label = (instance().console().leftController().name()) + " detected";
    else if(!image.empty())
      label = std::format("{} detected", ControllerDetector::detectName(image, type,
                                             Controller::Jack::Left,
                                             instance().settings()));
  }
  myLeftPortDetected->setLabel(label);

  label = "";
  type = Controller::getType(myRightPort->getSelectedTag().toString());
  if(type == Controller::Type::Unknown)
  {
    if(instance().hasConsole())
      label = (instance().console().rightController().name()) + " detected";
    else if(!image.empty())
      label = std::format("{} detected", ControllerDetector::detectName(image, type,
                                             Controller::Jack::Right,
                                             instance().settings()));
  }
  myRightPortDetected->setLabel(label);

  // Compumate bankswitching scheme doesn't allow to select controllers
  const bool enableSelectControl = myGameProperties.get(PropType::Cart_Type) != "CM";
  myLeftPortLabel->setEnabled(enableSelectControl);
  myRightPortLabel->setEnabled(enableSelectControl);
  myLeftPort->setEnabled(enableSelectControl);
  myRightPort->setEnabled(enableSelectControl);
}
