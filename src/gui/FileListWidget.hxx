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

#ifndef FILE_LIST_WIDGET_HXX
#define FILE_LIST_WIDGET_HXX

#include "Command.hxx"
#include "FSNode.hxx"
#include "GameList.hxx"
#include "StringListWidget.hxx"

/** 
  Provides an encapsulation of a file listing, allowing to descend into
  directories, and send signals based on whether an item is selected or
  activated.

  When the signals ItemChanged and ItemActivated are emitted, the caller
  can query the selected() and/or currentDir() methods to determine the
  current state.

  Note that for the current implementation, the ItemActivated signal is
  not sent when activating a directory (instead the code descends into
  the directory).  This may be changed in a future revision.
*/
class FileListWidget : public StringListWidget
{
  public:
    enum {
      ItemChanged   = 'FLic',  // Entry in the list is changed (single-click, etc)
      ItemActivated = 'FLac'   // Entry in the list is activated (double-click, etc)
    };

  public:
    FileListWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int w, int h);
    virtual ~FileListWidget();

    /** Determines how to display files/folders */
    void setFileListMode(FilesystemNode::ListMode mode) { _fsmode = mode; }
    void setFileExtension(const string& ext) { _extension = ext; }

    /** Set current location (file or directory) */
    void setLocation(const FilesystemNode& node, string select = "");

    /** Select parent directory (if applicable) */
    void selectParent();

    /** Gets current node(s) */
    const FilesystemNode& selected() const   { return _selected;  }
    const FilesystemNode& currentDir() const { return _node;      }

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    FilesystemNode::ListMode _fsmode;
    FilesystemNode _node, _selected;
    string _extension;

    GameList _gameList;
};

#endif
