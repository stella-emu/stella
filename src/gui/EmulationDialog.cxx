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
#include "FrameBuffer.hxx"
#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "Layout.hxx"
#include "RadioButtonWidget.hxx"
#include "TIASurface.hxx"

#include "EmulationDialog.hxx"

namespace {
  // Emulation speed is a positive float that multiplies the framerate. However,
  // the UI controls adjust speed in terms of a speedup factor (1/10,
  // 1/9 .. 1/2, 1, 2, 3, .., 10). The following mapping and formatting
  // functions implement this conversion. The speedup factor is represented
  // by an integer value between -900 and 900 (0 means no speedup).

  constexpr int MAX_SPEED = 900;
  constexpr int MIN_SPEED = -900;
  constexpr int SPEED_STEP = 10;

  int mapSpeed(float speed)
  {
    speed = std::abs(speed);

    return BSPF::clamp(
      static_cast<int>(std::round(100 * (speed >= 1 ? speed - 1 : -1 / speed + 1))),
      MIN_SPEED, MAX_SPEED
    );
  }

  constexpr float unmapSpeed(int speed)
  {
    const float f_speed = static_cast<float>(speed) / 100;

    return speed < 0 ? -1 / (f_speed - 1) : 1 + f_speed;
  }

  string formatSpeed(int speed) {
    return std::format("{:3.0f}", unmapSpeed(speed) * 100);
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationDialog::EmulationDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font)
  : Dialog(osystem, parent, font, "Emulation settings"),
    mySaveOnExitGroup{std::make_unique<RadioButtonGroup>()}
{
  const int fontWidth = Dialog::fontWidth();
  const int swidth = fontWidth * 10;
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font, so the dialog reflows on font change.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Speed
  mySpeed = new SliderWidget(this, _font, swidth,
                             "Emulation speed", 0, kSpeedupChanged, fontWidth * 5, "%");
  mySpeed->setMinValue(MIN_SPEED); mySpeed->setMaxValue(MAX_SPEED);
  mySpeed->setStepValue(SPEED_STEP);
  mySpeed->setTickmarkIntervals(2);
  mySpeed->setToolTip(Event::DecreaseSpeed, Event::IncreaseSpeed);
  wid.push_back(mySpeed);

  // Use sync to vblank
  myUseVSync = new CheckboxWidget(this, _font, "VSync");
  myUseVSync->setToolTip("Check to enable vertical synced display updates.");
  wid.push_back(myUseVSync);

  myTurbo = new CheckboxWidget(this, _font, "Turbo mode");
  myTurbo->setToolTip(Event::ToggleTurbo);
  wid.push_back(myTurbo);

  // Use multi-threading
  myUseThreads = new CheckboxWidget(this, _font, "Multi-threading");
  wid.push_back(myUseThreads);

  // Skip progress load bars for SuperCharger ROMs
  // Doesn't really belong here, but I couldn't find a better place for it
  myFastSCBios = new CheckboxWidget(this, _font, "Fast SuperCharger load");
  wid.push_back(myFastSCBios);

  // Show UI messages onscreen
  myUIMessages = new CheckboxWidget(this, _font, "Show UI messages");
  wid.push_back(myUIMessages);

  // Automatically pause emulation when focus is lost
  myAutoPauseWidget = new CheckboxWidget(this, _font, "Automatic pause");
  myAutoPauseWidget->setToolTip("Check for automatic pause/continue of\nemulation when Stella loses/gains focus.");
  wid.push_back(myAutoPauseWidget);

  // Confirm dialog when exiting emulation
  myConfirmExitWidget = new CheckboxWidget(this, _font, "Confirm exiting emulation");
  wid.push_back(myConfirmExitWidget);

  // Save on exit
  mySaveOnExitLabel = new StaticTextWidget(this, font,
                                           "When entering/exiting emulation:");
  mySaveOnExitButtons[0] = new RadioButtonWidget(this, font,
                                                 "Do nothing", mySaveOnExitGroup.get());
  wid.push_back(mySaveOnExitButtons[0]);
  mySaveOnExitButtons[1] = new RadioButtonWidget(this, font,
                                                 "Save current state in current slot", mySaveOnExitGroup.get());
  wid.push_back(mySaveOnExitButtons[1]);
  mySaveOnExitButtons[2] = new RadioButtonWidget(this, font,
                                                 "Load/save all Time Machine states", mySaveOnExitGroup.get());
  wid.push_back(mySaveOnExitButtons[2]);

  myAutoSlotWidget = new CheckboxWidget(this, font,
                                        "Automatically change save state slots");
  myAutoSlotWidget->setToolTip("Cycle to next state slot after saving.", Event::ToggleAutoSlot);
  wid.push_back(myAutoSlotWidget);

  // State directory
  myStatePathButton = new ButtonWidget(this, font,
                                       "State path" + ELLIPSIS, kChooseStateDir);
  myStatePathButton->setToolTip("Select the directory to load/save state files from/to.");
  wid.push_back(myStatePathButton);
  myStatePath = new EditTextWidget(this, font, 1, "");
  wid.push_back(myStatePath);

  // Save/load states in the ROM directory
  myStateWithRom = new CheckboxWidget(this, font,
                                      "Load/save states in ROM directory", kStateWithRom);
  myStateWithRom->setToolTip("Use the current ROM's directory for state files.");
  wid.push_back(myStateWithRom);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);

  setHelpAnchor("Emulation");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EmulationDialog::~EmulationDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap(),
            INDENT       = Dialog::indent();

  // State-path row: a button plus an edit field that fills the remaining width;
  // the outer VBox supplies the HBORDER inset (so marginH 0 here).  The edit
  // keeps its natural height, vertically centered in the taller button row.
  auto pathRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  pathRow->addAuto(anchoredItem(myStatePathButton));
  pathRow->addSpace(fontWidth);
  // The path widens with the dialog, but it says how much room a path needs —
  // which is what the dialog's own width is derived from
  pathRow->addStretch(stretchedItem(myStatePath,
                                    EditTextWidget::calcWidth(_font, 24)));

  // Vertical stack; the button group sits below it, positioned separately by
  // layoutButtonGroup().  The self-labeling slider, header and checkboxes keep
  // their natural size, so all are anchored; the save-on-exit radio buttons are
  // indented under their header.  Explicit addSpace() calls reproduce the
  // original's irregular inter-row gaps.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(anchoredItem(mySpeed));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myUseVSync));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myTurbo));
  root->addSpace(VGAP * 3);
  root->addAuto(anchoredItem(myUseThreads));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myFastSCBios));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myUIMessages));
  root->addSpace(VGAP * 4);
  root->addAuto(anchoredItem(myAutoPauseWidget));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myConfirmExitWidget));
  root->addSpace(VGAP * 3);
  root->addAuto(anchoredItem(mySaveOnExitLabel));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySaveOnExitButtons[0], INDENT));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySaveOnExitButtons[1], INDENT));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySaveOnExitButtons[2], INDENT));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myAutoSlotWidget));
  root->addSpace(VGAP * 3);
  root->addAuto(std::move(pathRow));
  root->addSpace(VGAP);
  root->addAuto(anchoredItem(myStateWithRom));

  // The dialog is as large as its content asks to be, and at least wide enough
  // for the button row below it (which the content knows nothing about)
  GUI::alignLabels({{mySpeed}});
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // Emulation speed
  const int speed = mapSpeed(settings.getFloat("speed"));
  mySpeed->setValue(speed);
  mySpeed->setValueLabel(formatSpeed(speed));

  // Use sync to vertical blank
  myUseVSync->setState(settings.getBool("vsync"));

  // Enable 'Turbo' mode
  myTurbo->setState(settings.getBool("turbo"));

  // Show UI messages
  myUIMessages->setState(settings.getBool("uimessages"));

  // Fast loading of Supercharger BIOS
  myFastSCBios->setState(settings.getBool("fastscbios"));

  // Multi-threaded rendering
  myUseThreads->setState(settings.getBool("threads"));

  // Automatically pause emulation when focus is lost
  myAutoPauseWidget->setState(settings.getBool("autopause"));

  // Confirm dialog when exiting emulation
  myConfirmExitWidget->setState(settings.getBool("confirmexit"));

  // Save on exit
  const string saveOnExit = settings.getString("saveonexit");
  mySaveOnExitGroup->setSelected(saveOnExit == "all" ? 2 : saveOnExit == "current" ? 1 : 0);
  // Automatically change save state slots
  myAutoSlotWidget->setState(settings.getBool("autoslot"));

  // State directory; resolves to the default location when unset
  myStatePath->setText(instance().configuredStateDir().getShortPath());
  myStateWithRom->setState(settings.getBool("statewithrom"));
  updateStatePathEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::updateStatePathEnabled()
{
  const bool enable = !myStateWithRom->getState();
  myStatePathButton->setEnabled(enable);
  myStatePath->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::saveConfig()
{
  Settings& settings = instance().settings();

  // Speed
  const int speedup = mySpeed->getValue();
  settings.setValue("speed", unmapSpeed(speedup));
  if(instance().hasConsole())
    instance().console().initializeAudio();

  // Use sync to vertical blank
  settings.setValue("vsync", myUseVSync->getState());

  // Enable 'Turbo' mode
  settings.setValue("turbo", myTurbo->getState());

  // Show UI messages
  settings.setValue("uimessages", myUIMessages->getState());

  // Fast loading of Supercharger BIOS
  settings.setValue("fastscbios", myFastSCBios->getState());

  // Multi-threaded rendering
  settings.setValue("threads", myUseThreads->getState());

  // Automatically pause emulation when focus is lost
  settings.setValue("autopause", myAutoPauseWidget->getState());

  // Confirm dialog when exiting emulation
  settings.setValue("confirmexit", myConfirmExitWidget->getState());

  // Save on exit
  const int saveOnExit = mySaveOnExitGroup->getSelected();
  settings.setValue("saveonexit",
                    saveOnExit == 0 ? "none" : saveOnExit == 1 ? "current" : "all");
  // Automatically change save state slots
  settings.setValue("autoslot", myAutoSlotWidget->getState());

  // State directory
  settings.setValue("statedir", myStatePath->getText());
  settings.setValue("statewithrom", myStateWithRom->getState());

  if(instance().hasConsole())
  {
    // update speed
    instance().console().initializeAudio();
    // update VSync
    instance().console().initializeVideo();
    instance().createFrameBuffer();

    instance().frameBuffer().tiaSurface().ntsc().enableThreading(myUseThreads->getState());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::setDefaults()
{
  // speed
  mySpeed->setValue(0);
  myUseVSync->setState(true);
  // misc
  myUIMessages->setState(true);
  myFastSCBios->setState(true);
  myUseThreads->setState(false);
  myAutoPauseWidget->setState(false);
  myConfirmExitWidget->setState(false);

  mySaveOnExitGroup->setSelected(0);
  myAutoSlotWidget->setState(false);

  // State directory; always reset to the default '<basedir>/state' location
  myStatePath->setText(instance().defaultStateDir().getShortPath());
  myStateWithRom->setState(false);
  updateStatePathEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EmulationDialog::handleCommand(CommandSender* sender, int cmd,
                                    int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kSpeedupChanged:
      mySpeed->setValueLabel(formatSpeed(mySpeed->getValue()));
      break;

    case kChooseStateDir:
      BrowserDialog::show(this, _font, "Select State Directory",
                          myStatePath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) myStatePath->setText(node.getShortPath());
                          });
      break;

    case kStateWithRom:
      updateStatePathEnabled();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
