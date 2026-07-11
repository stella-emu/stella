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
#include "Launcher.hxx"
#include "Bankswitch.hxx"
#include "BrowserDialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "ProgressDialog.hxx"
#include "FSNode.hxx"
#include "Font.hxx"
#include "Layout.hxx"
#include "MessageBox.hxx"
#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Props.hxx"
#include "PropsSet.hxx"
#include "Settings.hxx"
#include "RomAuditDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::RomAuditDialog(OSystem& osystem, DialogContainer& parent,
                               const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Audit ROMs"),
    myMaxWidth{max_w},
    myMaxHeight{max_h}
{
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Audit path" + ELLIPSIS);
  WidgetArray wid;

  // Widgets are only created here (at placeholder geometry); layout() assigns
  // all geometry from the current font.

  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  // Audit path
  myRomButton = new ButtonWidget(this, font, 0, 0,
      buttonWidth, buttonHeight, "Audit path" + ELLIPSIS, kChooseAuditDirCmd);
  wid.push_back(myRomButton);
  myRomPath = new EditTextWidget(this, font, 0, 0, lineHeight, lineHeight);
  wid.push_back(myRomPath);

  // Show results of ROM audit
  myRenamedLabel = new StaticTextWidget(this, font, 0, 0,
                                        "ROMs with properties (renamed) ");
  myResults1 = new EditTextWidget(this, font, 0, 0, fontWidth * 6, lineHeight);
  myResults1->setEditable(false, true);
  mySkippedLabel = new StaticTextWidget(this, font, 0, 0,
                                        "ROMs without properties (skipped) ");
  myResults2 = new EditTextWidget(this, font, 0, 0, fontWidth * 6, lineHeight);
  myResults2->setEditable(false, true);

  myWarningLabel = new StaticTextWidget(this, font, 0, 0,
                                        "(*) WARNING: Operation cannot be undone!");

  // Add OK and Cancel buttons
  addOKCancelBGroup(wid, font, "Audit", "Close");
  addBGroupToFocusList(wid);

  setHelpAnchor("ROMAudit");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::~RomAuditDialog() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            buttonWidth  = Dialog::buttonWidth("Audit path" + ELLIPSIS),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  const int lwidth = _font.getStringWidth("ROMs without properties (skipped) ");

  // Size the (fixed) dialog from the current font so it reflows on font change
  _w = 64 * fontWidth + HBORDER * 2;
  _h = _th + VBORDER * 2 + buttonHeight * 2 + lineHeight * 3 + VGAP * 10;

  // Audit-path row: a button plus an edit filling the remaining width (the edit
  // keeps its natural height, vertically centered in the taller button row)
  auto pathRow = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  pathRow->addFixed(widgetItem(myRomButton), buttonWidth);
  pathRow->addSpace(fontWidth);
  pathRow->addStretch(alignedItem(myRomPath, HAlign::Fill, VAlign::Center));

  // Two result rows: a label plus a small value field, the field aligned at a
  // shared column (lwidth) across both rows
  auto resultRow1 = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  resultRow1->addFixed(anchoredItem(myRenamedLabel), lwidth);
  resultRow1->addFixed(anchoredItem(myResults1), fontWidth * 6);

  auto resultRow2 = std::make_unique<BoxLayout>(Dir::Horizontal, 0, 0, 0);
  resultRow2->addFixed(anchoredItem(mySkippedLabel), lwidth);
  resultRow2->addFixed(anchoredItem(myResults2), fontWidth * 6);

  // Vertical stack; the OK/Cancel group sits below, positioned by
  // layoutButtonGroup().  Result rows use a buttonHeight pitch (matching the
  // original), the label/field anchored at the top.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addFixed(std::move(pathRow), buttonHeight);
  root->addSpace(VGAP * 4);
  root->addFixed(std::move(resultRow1), buttonHeight);
  root->addFixed(std::move(resultRow2), buttonHeight);
  root->addSpace(VGAP * 2);
  root->addFixed(anchoredItem(myWarningLabel), lineHeight);

  root->doLayout(0, _th, _w, _h - _th);

  // OK ("Audit") / Cancel ("Close") along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::loadConfig()
{
  const string& currentdir =
    instance().launcher().currentDir().getShortPath();
  const string& path = currentdir.empty() ?
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

  const FSNode node(auditPath);
  FSList files;
  files.reserve(2048);
  node.getChildren(files, FSNode::ListMode::FilesOnly);

  // Create a progress dialog box to show the progress of processing
  // the ROMs, since this is usually a time-consuming operation
  ProgressDialog progress(this, instance().frameBuffer().font());
  progress.setMessage("Auditing ROM files" + ELLIPSIS);
  progress.setRange(0, static_cast<int>(files.size()) - 1, 5);
  progress.open();

  Properties props;
  uInt32 renamed = 0, notfound = 0;
  for(uInt32 idx = 0; idx < files.size() && !progress.isCancelled(); ++idx)
  {
    string extension;
    if(files[idx].isFile() &&
       Bankswitch::isValidRomName(files[idx], extension))
    {
      bool renameSucceeded = false;

      // Calculate the MD5 so we can get the rest of the info
      // from the PropertiesSet (stella.pro)
      const string& md5 = OSystem::getROMMD5(files[idx]);
      if(instance().propSet().getMD5(md5, props))
      {
        string_view name = props.get(PropType::Cart_Name);

        // Only rename the file if we found a valid properties entry
        if(!name.empty() && name != files[idx].getName())
        {
          string newfile = node.getPath();
          newfile.append(name).append(".").append(extension);
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
    progress.incProgress();
  }

  progress.close();

  myResults1->setText(std::to_string(renamed));
  myResults2->setText(std::to_string(notfound));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      if(!myConfirmMsg)
      {
        StringList msg;
        msg.emplace_back("This operation cannot be undone.  Your ROMs");
        msg.emplace_back("will be modified, and as such there is a chance");
        msg.emplace_back("that files may be lost.  You are recommended");
        msg.emplace_back("to back up your files before proceeding.");
        msg.emplace_back("");
        msg.emplace_back("If you're sure you want to proceed with the");
        msg.emplace_back("audit, click 'OK', otherwise click 'Cancel'.");
        myConfirmMsg = std::make_unique<GUI::MessageBox>
          (this, _font, msg, myMaxWidth, myMaxHeight, kConfirmAuditCmd,
          "OK", "Cancel", "ROM Audit", false);
      }
      myConfirmMsg->show();
      break;

    case kConfirmAuditCmd:
      auditRoms();
      instance().launcher().reload();
      break;

    case kChooseAuditDirCmd:
      BrowserDialog::show(this, _font, "Select ROM Directory to Audit",
                          myRomPath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) {
                              myRomPath->setText(node.getShortPath());
                              myResults1->setText("");
                              myResults2->setText("");
                            }
                          });
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
