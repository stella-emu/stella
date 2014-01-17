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

#ifndef CONFIG_PATH_DIALOG_HXX
#define CONFIG_PATH_DIALOG_HXX

class OSystem;
class GuiObject;
class DialogContainer;
class BrowserDialog;
class CheckboxWidget;
class PopUpWidget;
class EditTextWidget;
class SliderWidget;
class StaticTextWidget;

#include "Dialog.hxx"
#include "Command.hxx"

class ConfigPathDialog : public Dialog, public CommandSender
{
  public:
    ConfigPathDialog(OSystem* osystem, DialogContainer* parent,
                     const GUI::Font& font, GuiObject* boss,
                     int max_w, int max_h);
    ~ConfigPathDialog();

    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

  private:
    enum {
      kChooseRomDirCmd      = 'LOrm', // rom select
      kChooseStateDirCmd    = 'LOsd', // state dir
      kChooseCheatFileCmd   = 'LOcf', // cheatfile (stella.cht)
      kChoosePaletteFileCmd = 'LOpf', // palette file (stella.pal)
      kChoosePropsFileCmd   = 'LOpr', // properties file (stella.pro)
      kChooseNVRamDirCmd   =  'LOnv', // nvram (flash/eeprom) dir
      kStateDirChosenCmd    = 'LOsc', // state dir changed
      kCheatFileChosenCmd   = 'LOcc', // cheatfile changed
      kPaletteFileChosenCmd = 'LOpc', // palette file changed
      kPropsFileChosenCmd   = 'LOrc', // properties file changed
      kNVRamDirChosenCmd    = 'LOnc'  // nvram (flash/eeprom) dir changed
    };

    BrowserDialog* myBrowser;

    // Config paths
    EditTextWidget* myRomPath;
    EditTextWidget* myStatePath;
    EditTextWidget* myNVRamPath;
    EditTextWidget* myCheatFile;
    EditTextWidget* myPaletteFile;
    EditTextWidget* myPropsFile;

    // Indicates if this dialog is used for global (vs. in-game) settings
    bool myIsGlobal;
};

#endif
