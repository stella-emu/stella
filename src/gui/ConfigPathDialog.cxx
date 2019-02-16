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

#include "bspf.hxx"
#include "BrowserDialog.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "Font.hxx"
#include "LauncherDialog.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "ConfigPathDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConfigPathDialog::ConfigPathDialog(
      OSystem& osystem, DialogContainer& parent,
      const GUI::Font& font, GuiObject* boss, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Configure paths"),
    CommandSender(boss),
    myFont(font),
    myBrowser(nullptr),
    myIsGlobal(boss != nullptr)
{
  const int VBORDER = 10 + _th;
  const int HBORDER = 10;
  const int V_GAP = 4;
  const int H_GAP = 8;
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            buttonWidth  = font.getStringWidth("ROM Path") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  WidgetArray wid;

  // Set real dimensions
  setSize(64 * fontWidth + HBORDER * 2, 9 * (lineHeight + V_GAP) + VBORDER, max_w, max_h);

  xpos = HBORDER;  ypos = VBORDER;

  // ROM path
  ButtonWidget* romButton =
    new ButtonWidget(this, font, xpos, ypos, buttonWidth, buttonHeight,
                     "ROM path" + ELLIPSIS, kChooseRomDirCmd);
  wid.push_back(romButton);
  xpos += buttonWidth + H_GAP;
  myRomPath = new EditTextWidget(this, font, xpos, ypos + 1,
                                 _w - xpos - HBORDER, lineHeight, "");
  wid.push_back(myRomPath);

  // Add Defaults, OK and Cancel buttons
  addDefaultsOKCancelBGroup(wid, font);

  addToFocusList(wid);

  // All ROM settings are disabled while in game mode
  if(!myIsGlobal)
  {
    romButton->clearFlags(WIDGET_ENABLED);
    myRomPath->setEditable(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConfigPathDialog::~ConfigPathDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConfigPathDialog::loadConfig()
{
  const Settings& settings = instance().settings();
  myRomPath->setText(settings.getString("romdir"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConfigPathDialog::saveConfig()
{
  instance().settings().setValue("romdir", myRomPath->getText());

  // Flush changes to disk and inform the OSystem
  instance().saveConfig();
  instance().setConfigPaths();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConfigPathDialog::setDefaults()
{
  FilesystemNode node("~");
  myRomPath->setText(node.getShortPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConfigPathDialog::handleCommand(CommandSender* sender, int cmd,
                                     int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      if(myIsGlobal)  // Let the boss know romdir has changed
        sendCommand(LauncherDialog::kRomDirChosenCmd, 0, 0);
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kChooseRomDirCmd:
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      createBrowser("Select ROM directory");
      myBrowser->show(myRomPath->getText(),
                      BrowserDialog::Directories, LauncherDialog::kRomDirChosenCmd);
      break;

    case LauncherDialog::kRomDirChosenCmd:
      myRomPath->setText(myBrowser->getResult().getShortPath());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConfigPathDialog::createBrowser(const string& title)
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
