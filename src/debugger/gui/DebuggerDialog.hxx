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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
class CartDebugWidget;

#include "Dialog.hxx"
#include "MessageBox.hxx"
#include "Rect.hxx"

class DebuggerDialog : public Dialog
{
  public:
    enum {
      kSmallFontMinW  = 1080, kSmallFontMinH  = 720,
      kMediumFontMinW = 1280, kMediumFontMinH = 860,
      kLargeFontMinW  = 1300, kLargeFontMinH  = 940
    };

    DebuggerDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~DebuggerDialog();

    const GUI::Font& lfont() const     { return *myLFont;        }
    const GUI::Font& nfont() const     { return *myNFont;        }
    PromptWidget& prompt() const       { return *myPrompt;       }
    TiaInfoWidget& tiaInfo() const     { return *myTiaInfo;      }
    TiaOutputWidget& tiaOutput() const { return *myTiaOutput;    }
    TiaZoomWidget& tiaZoom() const     { return *myTiaZoom;      }
    RomWidget& rom() const             { return *myRom;          }
    CartDebugWidget& cartDebug() const { return *myCartDebug;    }
    EditTextWidget& message() const    { return *myMessageBox;   }
    ButtonWidget& rewindButton() const { return *myRewindButton; }

    void showFatalMessage(const string& msg);

  private:
    void loadConfig();
    void handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void doStep();
    void doTrace();
    void doScanlineAdvance();
    void doAdvance();
    void doRewind();
    void doExitDebugger();
    void doExitRom();

    void createFont();
    void addTiaArea();
    void addTabArea();
    void addStatusArea();
    void addRomArea();

    GUI::Rect getTiaBounds() const;
    GUI::Rect getRomBounds() const;
    GUI::Rect getStatusBounds() const;
    GUI::Rect getTabBounds() const;

  private:
    enum {
      kDDStepCmd      = 'DDst',
      kDDTraceCmd     = 'DDtr',
      kDDAdvCmd       = 'DDav',
      kDDSAdvCmd      = 'DDsv',
      kDDRewindCmd    = 'DDrw',
      kDDExitCmd      = 'DDex',
      kDDExitFatalCmd = 'DDer'
    };

    TabWidget *myTab, *myRomTab;

    GUI::Font*       myLFont;    // used for labels
    GUI::Font*       myNFont;    // used for normal text
    PromptWidget*    myPrompt;
    TiaInfoWidget*   myTiaInfo;
    TiaOutputWidget* myTiaOutput;
    TiaZoomWidget*   myTiaZoom;
    CpuWidget*       myCpu;
    RamWidget*       myRam;
    RomWidget*       myRom;
    CartDebugWidget* myCartDebug;
    EditTextWidget*  myMessageBox;
    ButtonWidget*    myRewindButton;
    GUI::MessageBox* myFatalError;
};

#endif
