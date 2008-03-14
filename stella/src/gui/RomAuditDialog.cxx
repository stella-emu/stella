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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: RomAuditDialog.cxx,v 1.1 2008-03-14 15:23:24 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"

#include "BrowserDialog.hxx"
#include "DialogContainer.hxx"
#include "EditTextWidget.hxx"
#include "FSNode.hxx"
#include "Settings.hxx"

#include "RomAuditDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::RomAuditDialog(OSystem* osystem, DialogContainer* parent,
                               const GUI::Font& font,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myBrowser(NULL)
{
  const int vBorder = 8;
  const int bwidth  = font.getStringWidth("Audit path:") + 20,
            bheight = font.getLineHeight() + 4,
            fontHeight = font.getLineHeight();
  int xpos = vBorder, ypos = vBorder;
  WidgetArray wid;
  ButtonWidget* b;

  // Audit path
  ButtonWidget* romButton = 
    new ButtonWidget(this, font, xpos, ypos, bwidth, bheight, "Audit path:",
                     kChooseAuditDirCmd);
  wid.push_back(romButton);
  xpos += bwidth + 10;
  myRomPath = new EditTextWidget(this, font, xpos, ypos + 2,
                                 _w - xpos - 10, font.getLineHeight(), "");
  wid.push_back(myRomPath);

  // Show results of ROM audit
  xpos = 10;  ypos += bheight;
  myResults1 = new StaticTextWidget(this, font, xpos, ypos, _w - 20, fontHeight,
                                    "", kTextAlignLeft);
  myResults1->setFlags(WIDGET_CLEARBG);
  ypos += bheight;
  myResults2 = new StaticTextWidget(this, font, xpos, ypos, _w - 20, fontHeight,
                                    "", kTextAlignLeft);
  myResults2->setFlags(WIDGET_CLEARBG);


  // Add OK & Cancel buttons
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Audit", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Audit", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif

  addToFocusList(wid);

  // Create file browser dialog
  myBrowser = new BrowserDialog(this, font, 60, 20, 200, 200);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomAuditDialog::~RomAuditDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::loadConfig()
{
  myRomPath->setEditString(instance()->settings().getString("romdir"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::auditRoms()
{
  cerr << "do rom audit\n";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::openBrowser(const string& title, const string& startpath,
                                 FilesystemNode::ListMode mode, int cmd)
{
  parent()->addDialog(myBrowser);

  myBrowser->setTitle(title);
  myBrowser->setEmitSignal(cmd);
  myBrowser->setStartPath(startpath, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomAuditDialog::handleCommand(CommandSender* sender, int cmd,
                                   int data, int id)
{
  switch (cmd)
  {
    case kOKCmd:
      auditRoms();
      break;

    case kChooseAuditDirCmd:
      openBrowser("Select ROM directory to audit:", myRomPath->getEditString(),
                  FilesystemNode::kListDirectoriesOnly, kAuditDirChosenCmd);
      break;

    case kAuditDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setEditString(dir.path());
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
