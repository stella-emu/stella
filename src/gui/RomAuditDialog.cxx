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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "Launcher.hxx"
#include "LauncherFilterDialog.hxx"
#include "BrowserDialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "ProgressDialog.hxx"
#include "FSNode.hxx"
#include "Font.hxx"
#include "MessageBox.hxx"
#include "FrameBuffer.hxx"
#include "MD5.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "Settings.hxx"
#include "RomAuditDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::RomAuditDialog(OSystem& osystem, DialogContainer& parent,
                               const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Audit ROMs"),
    myConfirmMsg(nullptr),
    myMaxWidth(max_w),
    myMaxHeight(max_h)
{
  const int VBORDER = 10 + _th;
  const int HBORDER = 10;

  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Audit path" + ELLIPSIS) + 20,
            buttonHeight = font.getLineHeight() + 4,
            lwidth = font.getStringWidth("ROMs without properties (skipped) ");
  int xpos, ypos = VBORDER;
  WidgetArray wid;

  // Set real dimensions
  _w = 64 * fontWidth + HBORDER * 2;
  _h = 7 * (lineHeight + 4) + VBORDER;

  // Audit path
  ButtonWidget* romButton =
    new ButtonWidget(this, font, HBORDER, ypos, buttonWidth, buttonHeight,
                     "Audit path" + ELLIPSIS, kChooseAuditDirCmd);
  wid.push_back(romButton);
  xpos = HBORDER + buttonWidth + 8;
  myRomPath = new EditTextWidget(this, font, xpos, ypos + 1,
                                 _w - xpos - HBORDER, lineHeight, "");
  wid.push_back(myRomPath);

  // Show results of ROM audit
  ypos += buttonHeight + 16;
  new StaticTextWidget(this, font, HBORDER, ypos, lwidth, fontHeight,
                       "ROMs with properties (renamed) ", TextAlign::Left);
  myResults1 = new EditTextWidget(this, font, HBORDER + lwidth, ypos - 2,
                                  fontWidth * 6, lineHeight, "");
  myResults1->setEditable(false, true);
  ypos += buttonHeight;
  new StaticTextWidget(this, font, HBORDER, ypos, lwidth, fontHeight,
                       "ROMs without properties (skipped) ", TextAlign::Left);
  myResults2 = new EditTextWidget(this, font, HBORDER + lwidth, ypos - 2,
                                  fontWidth * 6, lineHeight, "");
  myResults2->setEditable(false, true);

  ypos += buttonHeight + 8;
  new StaticTextWidget(this, font, HBORDER, ypos, _w - 20, fontHeight,
                       "(*) WARNING: Operation cannot be undone!",
                       TextAlign::Left);

  // Add OK and Cancel buttons
  addOKCancelBGroup(wid, font, "Audit", "Close");
  addBGroupToFocusList(wid);

  // Create file browser dialog
  myBrowser = make_unique<BrowserDialog>(this, font, myMaxWidth, myMaxHeight, "Select ROM directory to audit");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::~RomAuditDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::loadConfig()
{
  const string& currentdir =
    instance().launcher().currentNode().getShortPath();
  const string& path = currentdir == "" ?
    instance().settings().getString("romdir") : currentdir;

  myRomPath->setText(path);
  myResults1->setText("");
  myResults2->setText("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::auditRoms()
{
  const string& auditPath = myRomPath->getText();
  myResults1->setText("");
  myResults2->setText("");

  FilesystemNode node(auditPath);
  FSList files;
  files.reserve(2048);
  node.getChildren(files, FilesystemNode::kListFilesOnly);

  // Create a progress dialog box to show the progress of processing
  // the ROMs, since this is usually a time-consuming operation
  ProgressDialog progress(this, instance().frameBuffer().font(),
                          "Auditing ROM files ...");
  progress.setRange(0, int(files.size()) - 1, 5);

  // Create a entry for the GameList for each file
  Properties props;
  int renamed = 0, notfound = 0;
  for(uInt32 idx = 0; idx < files.size(); idx++)
  {
    bool renameSucceeded = false;
    string extension;
    if(files[idx].isFile() &&
       LauncherFilterDialog::isValidRomName(files[idx], extension))
    {
      // Calculate the MD5 so we can get the rest of the info
      // from the PropertiesSet (stella.pro)
      const string& md5 = MD5::hash(files[idx]);
      if(instance().propSet().getMD5(md5, props))
      {
        const string& name = props.get(Cartridge_Name);

        // Only rename the file if we found a valid properties entry
        if(name != "" && name != files[idx].getName())
        {
          const string& newfile = node.getPath() + name + "." + extension;
          if(files[idx].getPath() != newfile && files[idx].rename(newfile))
            renameSucceeded = true;
        }
      }
      if(renameSucceeded)
        ++renamed;
      else
        ++notfound;
    }

    // Update the progress bar, indicating one more ROM has been processed
    progress.setProgress(idx);
  }
  progress.close();

  myResults1->setText(Variant(renamed).toString());
  myResults2->setText(Variant(notfound).toString());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      if(!myConfirmMsg)
      {
        StringList msg;
        msg.push_back("This operation cannot be undone.  Your ROMs");
        msg.push_back("will be modified, and as such there is a chance");
        msg.push_back("that files may be lost.  You are recommended");
        msg.push_back("to back up your files before proceeding.");
        msg.push_back("");
        msg.push_back("If you're sure you want to proceed with the");
        msg.push_back("audit, click 'OK', otherwise click 'Cancel'.");
        myConfirmMsg = make_unique<GUI::MessageBox>
                          (this, instance().frameBuffer().font(), msg,
                          myMaxWidth, myMaxHeight, kConfirmAuditCmd,
                           "OK", "Cancel", "ROM Audit", false);
      }
      myConfirmMsg->show();
      break;

    case kConfirmAuditCmd:
      auditRoms();
      instance().launcher().reload();
      break;

    case kChooseAuditDirCmd:
      myBrowser->show(myRomPath->getText(),
                      BrowserDialog::Directories, kAuditDirChosenCmd);
      break;

    case kAuditDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setText(dir.getShortPath());
      myResults1->setText("");
      myResults2->setText("");
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
