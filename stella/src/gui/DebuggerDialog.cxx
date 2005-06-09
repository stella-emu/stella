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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DebuggerDialog.cxx,v 1.5 2005-06-09 19:04:59 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "DialogContainer.hxx"
#include "PromptDialog.hxx"
#include "DebuggerDialog.hxx"

enum {
  kPromptCmd = 'PRMT',
  kCpuCmd    = 'CPU ',
  kRamCmd    = 'RAM ',
  kRomCmd    = 'ROM ',
  kTiaCmd    = 'TIA ',
  kCodeCmd   = 'CODE'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                               int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myPromptDialog(NULL),
    myCpuDialog(NULL),
    myRamDialog(NULL),
    myRomDialog(NULL),
    myTiaDialog(NULL),
    myCodeDialog(NULL)
{
  const int border = 5;
  const int space  = 5;
  const int width  = 40;
  int xpos = border;
/*
  // Add a row of buttons for the various debugger operations
  new ButtonWidget(this, xpos, border, width, 16, "Prompt", kPromptCmd, 0);
    xpos += space + width;
  new ButtonWidget(this, xpos, border, width, 16, "CPU", kCpuCmd, 0);
    xpos += space + width;
  new ButtonWidget(this, xpos, border, width, 16, "RAM", kRamCmd, 0);
    xpos += space + width;
  new ButtonWidget(this, xpos, border, width, 16, "ROM", kRomCmd, 0);
    xpos += space + width;
  new ButtonWidget(this, xpos, border, width, 16, "TIA", kTiaCmd, 0);
    xpos += space + width;
  new ButtonWidget(this, xpos, border, width, 16, "Code", kCodeCmd, 0);
    xpos += space + width;
*/
  const int xoff = border; 
  const int yoff = border + 16 + 2;

  // Create the debugger dialog boxes
//  myPromptDialog = new PromptDialog(this, osystem, parent, x, y, w, h);

  myPromptDialog = new PromptDialog(this, osystem, parent, x + xoff, y + yoff,
                                    w - xoff - 2, h - yoff - 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerDialog::~DebuggerDialog()
{
  delete myPromptDialog;
/*
  delete myCpuDialog;
  delete myRamDialog;
  delete myRomDialog;
  delete myTiaDialog;
  delete myCodeDialog;
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::addButtons(GuiObject* boss)
{
  // This is a terrible hack, but it does seem to work ...
  const int border = 5;
  const int space  = 5;
  const int width  = 40;
  int xpos = border;

  // Add a row of buttons for the various debugger operations
  new ButtonWidget(boss, xpos, border, width, 16, "Prompt", kPromptCmd, 0);
    xpos += space + width;
  new ButtonWidget(boss, xpos, border, width, 16, "CPU", kCpuCmd, 0);
    xpos += space + width;
  new ButtonWidget(boss, xpos, border, width, 16, "RAM", kRamCmd, 0);
    xpos += space + width;
  new ButtonWidget(boss, xpos, border, width, 16, "ROM", kRomCmd, 0);
    xpos += space + width;
  new ButtonWidget(boss, xpos, border, width, 16, "TIA", kTiaCmd, 0);
    xpos += space + width;
  new ButtonWidget(boss, xpos, border, width, 16, "Code", kCodeCmd, 0);
    xpos += space + width;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::loadConfig()
{
  // Auto-select the command prompt
  parent()->addDialog(myPromptDialog);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DebuggerDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kPromptCmd:
      parent()->reStack();
      parent()->addDialog(myPromptDialog);
      break;

    case kCpuCmd:
      parent()->reStack();
//      parent()->addDialog(myCpuDialog);
      cerr << "kCpuCmd\n";
      break;

    case kRamCmd:
      parent()->reStack();
//      parent()->addDialog(myRamDialog);
      cerr << "kRamCmd\n";
      break;

    case kRomCmd:
      parent()->reStack();
//      parent()->addDialog(myRomDialog);
      cerr << "kRomCmd\n";
      break;

    case kTiaCmd:
      parent()->reStack();
//      parent()->addDialog(myTiaDialog);
      cerr << "kTiaCmd\n";
      break;

    case kCodeCmd:
      parent()->reStack();
//      parent()->addDialog(myCodeDialog);
      cerr << "kCodeCmd\n";
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
      break;
  }
}
