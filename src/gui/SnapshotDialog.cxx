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

#include "bspf.hxx"
#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "Font.hxx"
#include "LauncherDialog.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "SnapshotDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnapshotDialog::SnapshotDialog(OSystem& osystem, DialogContainer& parent,
                               const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Snapshot settings"),
    myFont(font)
{
  const int lineHeight   = font.getLineHeight(),
            fontHeight   = _font.getFontHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonWidth  = font.getStringWidth("Save path" + ELLIPSIS) + fontWidth * 2.5,
            buttonHeight = font.getLineHeight() * 1.25;
  const int HBORDER = fontWidth * 1.25;
  const int VBORDER = fontHeight / 4;
  const int INDENT = fontWidth * 2;
  const int VGAP = fontHeight / 4;

  int xpos, ypos, fwidth;
  WidgetArray wid;
  ButtonWidget* b;

  // Set real dimensions
  setSize(64 * fontWidth + HBORDER * 2, 9 * (lineHeight + VGAP) + VBORDER + _th, max_w, max_h);

  xpos = HBORDER;  ypos = VBORDER + _th;

  // Snapshot path (save files)
  b = new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                       "Save path" + ELLIPSIS, kChooseSnapSaveDirCmd);
  wid.push_back(b);
  xpos += buttonWidth + fontWidth;
  mySnapSavePath = new EditTextWidget(this, font, xpos, ypos + (buttonHeight - lineHeight) / 2 - 1,
                                  _w - xpos - HBORDER, lineHeight, "");
  wid.push_back(mySnapSavePath);

  // Snapshot naming
  xpos = HBORDER;  ypos += buttonHeight + VGAP * 4;

  // Snapshot interval (continuous mode)
  mySnapInterval = new SliderWidget(this, font, xpos, ypos,
                                    "Continuous snapshot interval ", 0, kSnapshotInterval,
                                    font.getStringWidth("10 seconds"));
  mySnapInterval->setMinValue(1);
  mySnapInterval->setMaxValue(10);
  mySnapInterval->setTickmarkIntervals(3);
  wid.push_back(mySnapInterval);

  // Booleans for saving snapshots
  fwidth = font.getStringWidth("When saving snapshots:");
  xpos = HBORDER;  ypos += lineHeight + VGAP * 3;
  new StaticTextWidget(this, font, xpos, ypos, fwidth, lineHeight,
                       "When saving snapshots:", TextAlign::Left);

  // Snapshot single or multiple saves
  xpos += INDENT;  ypos += lineHeight + VGAP;
  mySnapName = new CheckboxWidget(this, font, xpos, ypos, "Use actual ROM name");
  wid.push_back(mySnapName);
  ypos += lineHeight + VGAP;

  mySnapSingle = new CheckboxWidget(this, font, xpos, ypos, "Overwrite existing files");
  wid.push_back(mySnapSingle);

  // Snapshot in 1x mode (ignore scaling)
  ypos += lineHeight + VGAP;
  mySnap1x = new CheckboxWidget(this, font, xpos, ypos,
      "Create pixel-exact image (no zoom/post-processing)");
  wid.push_back(mySnap1x);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SnapshotDialog::~SnapshotDialog()
{
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::saveConfig()
{
  instance().settings().setValue("snapsavedir", mySnapSavePath->getText());
  instance().settings().setValue("ssinterval", mySnapInterval->getValue());
  instance().settings().setValue("snapname", mySnapName->getState() ? "rom" : "int");
  instance().settings().setValue("sssingle", mySnapSingle->getState());
  instance().settings().setValue("ss1x", mySnap1x->getState());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::setDefaults()
{
  mySnapSavePath->setText(instance().defaultSaveDir());
  mySnapInterval->setValue(2);
  mySnapName->setState(false);
  mySnapSingle->setState(false);
  mySnap1x->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::handleCommand(CommandSender* sender, int cmd,
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

    case kChooseSnapSaveDirCmd:
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      createBrowser("Select snapshot save directory");
      myBrowser->show(mySnapSavePath->getText(),
                      BrowserDialog::Directories, kSnapSaveDirChosenCmd);
      break;

    case kSnapSaveDirChosenCmd:
      mySnapSavePath->setText(myBrowser->getResult().getShortPath());
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SnapshotDialog::createBrowser(const string& title)
{
  uInt32 w = 0, h = 0;
  getDynamicBounds(w, h);

  // Create file browser dialog
  if(!myBrowser || uInt32(myBrowser->getWidth()) != w ||
     uInt32(myBrowser->getHeight()) != h)
    myBrowser = make_unique<BrowserDialog>(this, myFont, w, h, title);
  else
    myBrowser->setTitle(title);
}
