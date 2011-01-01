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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DEBUGGER_DIALOG_HXX
#define DEBUGGER_DIALOG_HXX

class Debugger;
class OSystem;
class DialogContainer;
class ButtonWidget;
class CpuWidget;
class PromptWidget;
class RamWidget;
class RomWidget;
class TabWidget;
class EditTextWidget;
class TiaInfoWidget;
class TiaOutputWidget;
class TiaZoomWidget;

#include "Dialog.hxx"

class DebuggerDialog : public Dialog
{
  public:
    DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                          int x, int y, int w, int h);
    ~DebuggerDialog();

    PromptWidget* prompt()       { return myPrompt;       }
    TiaInfoWidget* tiaInfo()     { return myTiaInfo;      }
    TiaOutputWidget* tiaOutput() { return myTiaOutput;    }
    TiaZoomWidget* tiaZoom()     { return myTiaZoom;      }
    RomWidget* rom()             { return myRom;          }
    EditTextWidget* message()    { return myMessageBox;   }
    ButtonWidget* rewindButton() { return myRewindButton; }

    virtual void loadConfig();
    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    TabWidget* myTab;

    PromptWidget*    myPrompt;
    TiaInfoWidget*   myTiaInfo;
    TiaOutputWidget* myTiaOutput;
    TiaZoomWidget*   myTiaZoom;
    CpuWidget*       myCpu;
    RamWidget*       myRam;
    RomWidget*       myRom;
    EditTextWidget*  myMessageBox;
    ButtonWidget*    myRewindButton;

  private:
    void addTiaArea();
    void addTabArea();
    void addStatusArea();
    void addRomArea();

    void doStep();
    void doTrace();
    void doScanlineAdvance();
    void doAdvance();
    void doRewind();
    void doExit();
};

#endif
