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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FILE_LIST_WIDGET_HXX
#define FILE_LIST_WIDGET_HXX

class CommandSender;
class ProgressDialog;

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

  Widgets wishing to enforce their own filename filtering are able
  to use a 'NameFilter' as described below.
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
    ~FileListWidget() override = default;

    string getToolTip(const Common::Point& pos) const override;

    /** Determines how to display files/folders; either setDirectory or reload
        must be called after any of these are called. */
    void setListMode(FilesystemNode::ListMode mode) { _fsmode = mode; }
    void setNameFilter(const FilesystemNode::NameFilter& filter) {
      _filter = filter;
    }

    // When enabled, all subdirectories will be searched too.
    void setIncludeSubDirs(bool enable) { _includeSubDirs = enable; }

    /**
      Set initial directory, and optionally select the given item.

        @param node       The directory to display.  If this is a file, its parent
                          will instead be used, and the file will be selected
        @param select     An optional entry to select (if applicable)
    */
    void setDirectory(const FilesystemNode& node,
                      const string& select = EmptyString);

    /** Select parent directory (if applicable) */
    void selectParent();

    /** Reload current location (file or directory) */
    void reload();

    /** Gets current node(s) */
    const FilesystemNode& selected() {
      _selected = BSPF::clamp(_selected, 0U, uInt32(_fileList.size()-1));
      return _fileList[_selected];
    }
    const FilesystemNode& currentDir() const { return _node; }

    static void setQuickSelectDelay(uInt64 time) { _QUICK_SELECT_DELAY = time; }
    uInt64 getQuickSelectDelay() const { return _QUICK_SELECT_DELAY; }

    ProgressDialog& progress();
    void incProgress();

  protected:
    static unique_ptr<ProgressDialog> myProgressDialog;

  private:
    /** Very similar to setDirectory(), but also updates the history */
    void setLocation(const FilesystemNode& node, const string& select);

    /** Descend into currently selected directory */
    void selectDirectory();

    bool handleText(char text) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    FilesystemNode::ListMode _fsmode{FilesystemNode::ListMode::All};
    FilesystemNode::NameFilter _filter;
    FilesystemNode _node;
    FSList _fileList;
    bool _includeSubDirs{false};

    StringList _dirList;

    Common::FixedStack<string> _history;
    uInt32 _selected{0};
    string _selectedFile;

    string _quickSelectStr;
    uInt64 _quickSelectTime{0};
    static uInt64 _QUICK_SELECT_DELAY;

  private:
    // Following constructors and assignment operators not supported
    FileListWidget() = delete;
    FileListWidget(const FileListWidget&) = delete;
    FileListWidget(FileListWidget&&) = delete;
    FileListWidget& operator=(const FileListWidget&) = delete;
    FileListWidget& operator=(FileListWidget&&) = delete;
};

#endif
