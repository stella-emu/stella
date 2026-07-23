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

#include "bspf.hxx"
#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "Font.hxx"
#include "Layout.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "SnapshotDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnapshotDialog::SnapshotDialog(OSystem& osystem, DialogContainer& parent,
                               const GUI::Font& font)
  : Dialog(osystem, parent, font, "Snapshot settings")
{
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font, so the dialog reflows on font change.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Snapshot path (save files)
  mySnapSaveButton =
    new ButtonWidget(this, font, "Save path" + ELLIPSIS, kChooseSnapSaveDirCmd);
  wid.push_back(mySnapSaveButton);
  mySnapSavePath = new EditTextWidget(this, font, 1);
  wid.push_back(mySnapSavePath);

  // Snapshot interval (continuous mode)
  mySnapIntervalLabel = new StaticTextWidget(this, font, "Continuous snapshot interval");
  mySnapInterval = new SliderWidget(this, font, 0, kSnapshotInterval, 10);
  mySnapInterval->setMinValue(1);
  mySnapInterval->setMaxValue(10);
  mySnapInterval->setTickmarkIntervals(3);
  wid.push_back(mySnapInterval);

  // Header for the boolean save options
  myWhenLabel = new StaticTextWidget(this, font, "When saving snapshots:", TextAlign::Left);

  // Snapshot single or multiple saves
  mySnapName = new CheckboxWidget(this, font, "Use actual ROM name");
  wid.push_back(mySnapName);
  mySnapSingle = new CheckboxWidget(this, font, "Overwrite existing files");
  wid.push_back(mySnapSingle);

  // Snapshot in 1x mode (ignore scaling)
  mySnap1x = new CheckboxWidget(this, font,
      "Create pixel-exact image (no zoom/post-processing)");
  wid.push_back(mySnap1x);

  // Automatically crop black borders
  mySnapCrop = new CheckboxWidget(this, font, "Crop black borders");
  wid.push_back(mySnapCrop);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);

  setHelpAnchor("Snapshots");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap(),
            INDENT       = Dialog::indent();

  // The slider's label stands on its own, so give it a label column of its own
  GUI::alignLabels({{mySnapIntervalLabel}});

  // Save-path row: a button plus an edit field that fills the remaining width.
  // The row is the only one with several widgets, so it needs its own HBox; the
  // outer VBox already supplies the HBORDER inset (hence marginH 0 here).  The
  // edit field keeps its own (natural) height, vertically centered in the taller
  // button row, and how much of a path it must show is this dialog's one width
  // decision -- everything else is derived from it
  auto pathRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  pathRow->addAuto(anchoredItem(mySnapSaveButton));
  pathRow->addSpace(fontWidth);
  pathRow->addStretch(stretchedItem(mySnapSavePath,
                                    EditTextWidget::calcWidth(_font, 48)));

  // Assemble the vertical stack; the button group sits below it, positioned
  // separately by layoutButtonGroup().  The header/checkboxes keep their
  // natural size, so all but the interval row are anchored.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(std::move(pathRow));
  root->addSpace(VGAP * 4);
  root->addAuto(labeledRow(mySnapIntervalLabel, mySnapInterval));
  root->addSpace(VGAP * 3);
  root->addAuto(anchoredItem(myWhenLabel));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySnapName, INDENT));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySnapSingle, INDENT));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySnap1x, INDENT));
  root->addSpace(VGAP);
  root->addAuto(indentedItem(mySnapCrop, INDENT));

  // The dialog is as large as its content asks to be, and at least wide enough
  // for the button row below it (which the content knows nothing about)
  const Common::Size natural = root->naturalSize();

  _w = std::max(static_cast<int>(natural.w), Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(natural.h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::loadConfig()
{
  const Settings& settings = instance().settings();
  mySnapSavePath->setText(settings.getString("snapsavedir"));
  mySnapInterval->setValue(instance().settings().getInt("ssinterval"));
  mySnapName->setState(instance().settings().getString("snapname") == "rom");
  mySnapSingle->setState(settings.getBool("sssingle"));
  mySnap1x->setState(settings.getBool("ss1x"));
  mySnapCrop->setState(settings.getBool("sscrop"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::saveConfig()
{
  instance().settings().setValue("snapsavedir", mySnapSavePath->getText());
  instance().settings().setValue("ssinterval", mySnapInterval->getValue());
  instance().settings().setValue("snapname", mySnapName->getState() ? "rom" : "int");
  instance().settings().setValue("sssingle", mySnapSingle->getState());
  instance().settings().setValue("ss1x", mySnap1x->getState());
  instance().settings().setValue("sscrop", mySnapCrop->getState());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::setDefaults()
{
  mySnapSavePath->setText(instance().userDir().getShortPath());
  mySnapInterval->setValue(2);
  mySnapName->setState(false);
  mySnapSingle->setState(false);
  mySnap1x->setState(false);
  mySnapCrop->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::handleCommand(CommandSender* sender, int cmd,
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

    case kChooseSnapSaveDirCmd:
      BrowserDialog::show(this, _font, "Select Snapshot Save Directory",
                          mySnapSavePath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) mySnapSavePath->setText(node.getShortPath());
                          });
      break;

    case kSnapshotInterval:
      if(mySnapInterval->getValue() == 1)
        mySnapInterval->setValueUnit(" second");
      else
        mySnapInterval->setValueUnit(" seconds");
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
