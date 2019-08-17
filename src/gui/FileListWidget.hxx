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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FILE_LIST_WIDGET_HXX
#define FILE_LIST_WIDGET_HXX

class CommandSender;

#include "FSNode.hxx"
#include "Stack.hxx"
#include "StringListWidget.hxx"

/**
  Provides an encapsulation of a file listing, allowing to descend into
  directories, and send signals based on whether an item is selected or
  activated.

  When the signals ItemChanged and ItemActivated are emitted, the caller
  can query the selected() and/or currentDir() methods to determine the
  current state.

  Note that the ItemActivated signal is not sent when activating a
  directory; instead the selection descends into the directory.
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
    virtual ~FileListWidget() = default;

    /** Determines how to display files/folders */
    void setListMode(FilesystemNode::ListMode mode) { _fsmode = mode; }
    void setFileExtension(const string& ext) { _extension = ext; }  // TODO - re-implement this

    /**
      Set initial directory, and optionally select the given item.

        @param node  The directory to display.  If this is a file, its parent
                     will instead be used, and the file will be selected
        @param select  An optional entry to select (if applicable)
    */
    void setDirectory(const FilesystemNode& node, string select = "");

    /** Select parent directory (if applicable) */
    void selectParent();

    /** Reload current location (file or directory) */
    void reload();

    /** Gets current node(s) */
    const FilesystemNode& selected() const   { return _fileList[_selected]; }
    const FilesystemNode& currentDir() const { return _node; }

  private:
    /** Very similar to setDirectory(), but also updates the history */
    void setLocation(const FilesystemNode& node, const string& select = EmptyString);

    /** Descend into currently selected directory */
    void selectDirectory();

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    FilesystemNode::ListMode _fsmode;
    FilesystemNode _node;
    FSList _fileList;

    Common::FixedStack<string> _history;

    string _extension;
    uInt32 _selected;

  private:
    // Following constructors and assignment operators not supported
    FileListWidget() = delete;
    FileListWidget(const FileListWidget&) = delete;
    FileListWidget(FileListWidget&&) = delete;
    FileListWidget& operator=(const FileListWidget&) = delete;
    FileListWidget& operator=(FileListWidget&&) = delete;
};

#endif
