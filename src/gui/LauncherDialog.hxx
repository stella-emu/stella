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

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

#include "bspf.hxx"

class ButtonWidget;
class CommandSender;
class ContextMenu;
class DialogContainer;
class GameList;
class BrowserDialog;
class OptionsDialog;
class GlobalPropsDialog;
class LauncherFilterDialog;
class OSystem;
class Properties;
class EditTextWidget;
class RomInfoWidget;
class StaticTextWidget;
class StringListWidget;

#include "Dialog.hxx"
#include "FSNode.hxx"
#include "StringList.hxx"
#include "Stack.hxx"
#include "MessageBox.hxx"

class LauncherDialog : public Dialog
{
  public:
    // These must be accessible from dialogs created by this class
    enum {
      kLoadROMCmd       = 'STRT',  // load currently selected ROM
      kRomDirChosenCmd  = 'romc',  // rom chosen
      kReloadRomDirCmd  = 'rdrl',  // reload the current listing
      kReloadFiltersCmd = 'rlfl'   // reload filtering options and current listing
    };

  public:
    LauncherDialog(OSystem* osystem, DialogContainer* parent,
                   int x, int y, int w, int h);
    ~LauncherDialog();

    /**
      Get MD5sum for the currently selected file

      @return md5sum if a valid ROM file, else the empty string
    */
    const string& selectedRomMD5();

    /**
      Get node for the currently selected directory

      @return FilesystemNode currently active
    */
    const FilesystemNode& currentNode() const { return myCurrentNode; }

    /**
      Reload the current listing
    */
    void reload() { updateListing(); }

  protected:
    virtual void handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void updateListing(const string& nameToSelect = "");

  private:
    void enableButtons(bool enable);
    void loadDirListing();
    void loadRomInfo();
    void handleContextMenu();
    void setListFilters();
    bool matchPattern(const string& s, const string& pattern) const;

  private:
    ButtonWidget* myStartButton;
    ButtonWidget* myPrevDirButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myQuitButton;

    StringListWidget* myList;
    StaticTextWidget* myDirLabel;
    StaticTextWidget* myDir;
    StaticTextWidget* myRomCount;
    EditTextWidget*   myPattern;
    GameList*         myGameList;

    OptionsDialog* myOptions;
    RomInfoWidget* myRomInfoWidget;

    ContextMenu*          myMenu;
    GlobalPropsDialog*    myGlobalProps;
    LauncherFilterDialog* myFilters;

    GUI::MessageBox* myFirstRunMsg;
    BrowserDialog*   myRomDir;

    int mySelectedItem;
    FilesystemNode myCurrentNode;
    Common::FixedStack<string> myNodeNames;

    StringList myRomExts;

    enum {
      kPrevDirCmd = 'PRVD',
      kOptionsCmd = 'OPTI',
      kQuitCmd    = 'QUIT',

      kFirstRunMsgChosenCmd   = 'frmc',
      kStartupRomDirChosenCmd = 'rmsc'
    };
};

#endif
