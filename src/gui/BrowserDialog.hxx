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

#ifndef BROWSER_DIALOG_HXX
#define BROWSER_DIALOG_HXX

class GuiObject;
class ButtonWidget;
class EditTextWidget;
class FileListWidget;
class StaticTextWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"

class BrowserDialog : public Dialog, public CommandSender
{
  public:
    enum ListMode {
      FileLoad,   // File selector, no input from user
      FileSave,   // File selector, filename changable by user
      Directories // Directories only, no input from user
    };

  public:
    BrowserDialog(GuiObject* boss, const GUI::Font& font, int max_w, int max_h);
    virtual ~BrowserDialog();

    /** Place the browser window onscreen, using the given attributes */
    void show(const string& title, const string& startpath,
              BrowserDialog::ListMode mode, int cmd, const string& ext = "");

    /** Get resulting file node (called after receiving kChooseCmd) */
    const FilesystemNode& getResult() const;

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void updateUI();

  private:
    enum {
      kChooseCmd  = 'CHOS',
      kGoUpCmd    = 'GOUP',
      kBaseDirCmd = 'BADR'
    };

    int	_cmd;

    FileListWidget*   _fileList;
    StaticTextWidget* _currentPath;
    StaticTextWidget* _title;
    StaticTextWidget* _type;
    EditTextWidget*   _selected;
    ButtonWidget*     _goUpButton;
    ButtonWidget*     _basedirButton;

    BrowserDialog::ListMode _mode;
};

#endif
