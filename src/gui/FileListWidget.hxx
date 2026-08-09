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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FILE_LIST_WIDGET_HXX
#define FILE_LIST_WIDGET_HXX

class CommandSender;
class ProgressDialog;

#include <unordered_map>

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
    struct Cmd {
      static constexpr GuiCmd::Code
        ItemChanged   = GuiCmd::of("FileListWidget.ItemChanged"),
        ItemActivated = GuiCmd::of("FileListWidget.ItemActivated"),
        HomeDir       = GuiCmd::of("FileListWidget.HomeDir"),
        PrevDir       = GuiCmd::of("FileListWidget.PrevDir"),
        NextDir       = GuiCmd::of("FileListWidget.NextDir");
    };
  public:
    FileListWidget(GuiObject* boss, const GUI::Font& font);
    ~FileListWidget() override = default;

    // Alt+Home/Left/Right for history navigation, plus quick-select by typing
    // consecutive letters (see _quickSelectStr)
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    bool handleText(char text) override;

    // The full path when fullPathToolTip(), else the base tooltip (see StringListWidget)
    string getToolTip(const Common::Point& pos) const override;

    /** Determines how to display files/folders; either setDirectory or reload
        must be called after any of these are called. */
    void setListMode(FSNode::ListMode mode) { _fsmode = mode; }
    void setNameFilter(FSNode::NameFilter filter) {
      _filter = std::move(filter);
    }

    // When enabled, all subdirectories will be searched too.
    void setIncludeSubDirs(bool enable) { _includeSubDirs = enable; }

    // When enabled, file extensions will be displayed too.
    void setShowFileExtensions(bool enable) { _showFileExtensions = enable; }

    /**
      Set initial directory, and optionally select the given item.

        @param node       The directory to display.  If this is a file, its parent
                          will instead be used, and the file will be selected
        @param select     An optional entry to select (if applicable)
    */
    void setInitialDirectory(const FSNode& node, string_view select = {});

    /** Descend into currently selected directory */
    void selectDirectory();
    /** Go to directory */
    void selectDirectory(const FSNode& node);
    /** Select parent directory (if applicable) */
    void selectParent();
    /** Check if there is a previous directory in history */
    bool hasPrevHistory() const;
    /** Check if there is a next directory in history */
    bool hasNextHistory() const;

    /** Reload current location (file or directory) */
    void reload();

    /** Gets current node(s) */
    const FSNode& selected();
    const FSNode& currentDir() const { return _node; }

    // How long consecutive keypresses are treated as one quick-select search
    static void setQuickSelectDelay(uInt64 time) { S_QUICK_SELECT_DELAY = time; }
    static uInt64 getQuickSelectDelay() { return S_QUICK_SELECT_DELAY; }

    // The progress dialog shown while scanning a large directory, created on first use
    ProgressDialog& progress();
    void incProgress();

    virtual bool isDirectory(const FSNode& node) const;

  protected:
    // Which icon a row gets; numTypes onward are launcher-only virtual-folder
    // icons (favorites/recent/popular), kept out of the plain file-list range
    enum class IconType: uInt8 {
      unknown,
      rom,
      directory,
      zip,
      cassette,
      updir,
      numTypes,
      favrom = numTypes,
      favdir,
      favzip,
      userdir,
      recentdir,
      popdir,
      numLauncherTypes = popdir - numTypes + 1
    };
    using IconTypeList = std::vector<IconType>;

    // The currently shown directory (or selected file's parent)
    FSNode _node;
    // Its contents, in the same order as the displayed rows
    FSList _fileList;
    FSNode::NameFilter _filter;
    // Display strings for _fileList; StringListWidget::_list is set from this
    StringList _dirList;
    // Icon type for each row, parallel to _fileList/_dirList
    IconTypeList _iconTypeList;

  protected:
    // Populates _fileList/_dirList/_iconTypeList from _node
    virtual void getChildren(const FSNode::CancelCheck& isCancelled);
    // Hook for a subclass to inject extra (virtual) entries; default adds none
    virtual void extendLists(StringList& list) { }
    virtual IconType getIconType(const FSNode& node) const;
    virtual const GUI::Icon* getIcon(int i) const;
    // Whether the tooltip shows the full path rather than just the name
    virtual bool fullPathToolTip() const { return false; }

    // Icon size in pixels, stepped up once the row is tall enough for it
    int iconWidth() const { return (_lineHeight < 26) ? 16 + 4: 24 + 6; }
    int drawIcon(int i, int x, int y, ColorId color) override;

    // Reacts to the Home/Prev/Next directory-navigation commands
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    /** Very similar to setInitialDirectory(), but also updates selection history */
    void setLocation(const FSNode& node, string_view select = {});
    /** Select to home directory */
    void selectHomeDir();
    /** Select previous directory in history (if applicable) */
    void selectPrevHistory();
    /** Select next directory in history (if applicable) */
    void selectNextHistory();

    // Records 'node' as the new current entry in the Prev/Next history
    void addHistory(const FSNode& node);

  private:
    // Directories visited, for Prev/Next navigation
    std::vector<FSNode> _history;
    // Current position within _history
    size_t _currentHistoryIdx{0};
    size_t _historyHome{0}; // offset into initially created history
    // Remembers the selected child when returning to a directory, keyed by path
    std::unordered_map<string, string> _selectionHistory;

    // Set via setListMode()/setNameFilter()/setShowFileExtensions()
    FSNode::ListMode _fsmode{FSNode::ListMode::All};
    bool _includeSubDirs{false};
    bool _showFileExtensions{true};

    // Index into _fileList of the selected entry (mirrors ListWidget::_selectedItem)
    uInt32 _selected{0};

    // Allow quick select for "uppercase", non-letter input
    StellaKey _lastKey{StellaKey::UNKNOWN};
    StellaMod _lastMod{StellaMod::NONE};
    StellaMod _firstMod{StellaMod::NONE};
    string _quickSelectStr;
    uInt64 _quickSelectTime{0};
    static inline uInt64 S_QUICK_SELECT_DELAY{300};

    // Backing storage for progress(); null until first shown
    unique_ptr<ProgressDialog> myProgressDialog;

    // Fallback returned by selected() when _fileList is empty (should not happen)
    static const FSNode& defaultNode() {
      static const FSNode node{"~"};
      return node;
    }

  private:
    // Following constructors and assignment operators not supported
    FileListWidget() = delete;
    FileListWidget(const FileListWidget&) = delete;
    FileListWidget(FileListWidget&&) = delete;
    FileListWidget& operator=(const FileListWidget&) = delete;
    FileListWidget& operator=(FileListWidget&&) = delete;
};

#endif  // FILE_LIST_WIDGET_HXX
