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

#ifndef OPTIONS_DIALOG_HXX
#define OPTIONS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class GuiObject;
class VideoDialog;
class AudioDialog;
class InputDialog;
class UIDialog;
class SnapshotDialog;
class ConfigPathDialog;
class RomAuditDialog;
class GameInfoDialog;
class CheatCodeDialog;
class HelpDialog;
class AboutDialog;
class LoggerDialog;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

class OptionsDialog : public Dialog
{
  public:
    OptionsDialog(OSystem* osystem, DialogContainer* parent, GuiObject* boss,
                  int max_w, int max_h, bool global);
    virtual ~OptionsDialog();

  private:
    void loadConfig();
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    VideoDialog*      myVideoDialog;
    AudioDialog*      myAudioDialog;
    InputDialog*      myInputDialog;
    UIDialog*         myUIDialog;
    SnapshotDialog*   mySnapshotDialog;
    ConfigPathDialog* myConfigPathDialog;
    RomAuditDialog*   myRomAuditDialog;
    GameInfoDialog*   myGameInfoDialog;
    CheatCodeDialog*  myCheatCodeDialog;
    LoggerDialog*     myLoggerDialog;
    HelpDialog*       myHelpDialog;
    AboutDialog*      myAboutDialog;

    ButtonWidget* myRomAuditButton;
    ButtonWidget* myGameInfoButton;
    ButtonWidget* myCheatCodeButton;

    // Indicates if this dialog is used for global (vs. in-game) settings
    bool myIsGlobal;

    enum {
      kVidCmd      = 'VIDO',
      kAudCmd      = 'AUDO',
      kInptCmd     = 'INPT',
      kUsrIfaceCmd = 'URIF',
      kSnapCmd     = 'SNAP',
      kCfgPathsCmd = 'CFGP',
      kAuditCmd    = 'RAUD',
      kInfoCmd     = 'INFO',
      kCheatCmd    = 'CHET',
      kLoggerCmd   = 'LOGG',
      kHelpCmd     = 'HELP',
      kAboutCmd    = 'ABOU',
      kExitCmd     = 'EXIM'
    };
};

#endif
