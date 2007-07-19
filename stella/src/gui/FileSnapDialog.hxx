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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FileSnapDialog.hxx,v 1.3 2007-07-19 16:21:39 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef FILE_SNAP_DIALOG_HXX
#define FILE_SNAP_DIALOG_HXX

class OSystem;
class GuiObject;
class DialogContainer;
class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class StaticTextWidget;
class TabWidget;

#include "Dialog.hxx"
#include "Command.hxx"

class FileSnapDialog : public Dialog, public CommandSender
{
  public:
    FileSnapDialog(OSystem* osystem, DialogContainer* parent,
                   const GUI::Font& font, GuiObject* boss,
                   int x, int y, int w, int h);
    ~FileSnapDialog();

    virtual void loadConfig();
    virtual void saveConfig();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    void openBrowser(const string& title, const string& startpath, int cmd);

  private:
    enum {
      kChooseRomDirCmd      = 'LOrm', // rom select
      kChooseStateDirCmd    = 'LOsd', // state dir
      kChooseCheatFileCmd   = 'LOcf', // cheatfile (stella.cht)
      kChoosePaletteFileCmd = 'LOpf', // palette file (stella.pal)
      kChoosePropsFileCmd   = 'LOpr', // properties file (stella.pro)
      kChooseSnapDirCmd     = 'LOsn', // snap select
      kBrowseDirCmd         = 'LObd', // browse mode
      kStateDirChosenCmd    = 'LOsc', // state dir changed
      kCheatFileChosenCmd   = 'LOcc', // cheatfile changed
      kPaletteFileChosenCmd = 'LOpc', // palette file changed
      kPropsFileChosenCmd   = 'LOrc'  // properties file changed
    };


    BrowserDialog* myBrowser;
    TabWidget* myTab;

    // Rom path controls
    StaticTextWidget* myRomPath;
    CheckboxWidget*   myBrowseCheckbox;
    ButtonWidget*     myReloadButton;

    // Config paths
    StaticTextWidget* myStatePath;
    StaticTextWidget* myCheatFile;
    StaticTextWidget* myPaletteFile;
    StaticTextWidget* myPropsFile;
    StaticTextWidget* mySnapPath;
    CheckboxWidget*   mySnapSingleCheckbox;

    // Indicates if this dialog is used for global (vs. in-game) settings
    bool myIsGlobal;
};

#endif
