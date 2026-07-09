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

#ifndef PROMPT_WIDGET_HXX
#define PROMPT_WIDGET_HXX

class ContextMenu;
class UndoHandler;
class ScrollBarWidget;
class FSNode;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

class PromptWidget : public Widget, public CommandSender
{
  public:
    PromptWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h);
    ~PromptWidget() override = default;

    void loadConfig() override;

    void print(string_view str);
    void printPrompt();
    string saveBuffer(const FSNode& file);

    // Clear screen
    void clearScreen();
    // Erase all history
    void clearHistory();

    void addToHistory(string_view str);

    bool isLoaded() const { return !_firstTime; }

    // Account for the extra width of embedded scrollbar
    int getWidth() const override;

    using Widget::setPos;
    void setPos(const Common::Point& pos) override;
    void setWidth(int w) override;
    void setHeight(int h) override;
    void setArea(int x, int y, int w, int h) override;
    void refreshFontMetrics() override;

    bool wantsFocus() const override { return true; }

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;

  protected:
    int& buffer(int idx) { return _buffer[idx % kBufferSize]; }
    int  buffer(int idx) const { return _buffer[idx % kBufferSize]; }

    void drawWidget(bool hilite) override;
    void drawCaret();
    void putcharIntern(int c);
    void updateScrollBuffer();
    void scrollToCurrent();

    // Line editing
    void nextLine();
    void killChar(int direction);
    void killLine(int direction);
    void killWord(int direction);
    void moveWord(int direction, bool select);
    void markWord();
    void setLine(string_view text);

    // Clipboard
    string getLine() const;
    string selectedText() const;
    void textCut();
    void textCopy();
    void textPaste();

    // History
    bool historyScroll(int direction);

    bool execute();
    bool autoComplete(int direction);
    void resetFunctions();

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void lostFocusWidget() override
    {
      _selectSize = 0;
      if(_currentPos < _promptStartPos)
        _currentPos = _promptEndPos;
    }

  private:
    enum: uInt16 {
      kBufferSize = 32768,
      kLineBufferSize = 256,
      kHistorySize = 1000
    };

    std::array<int, kBufferSize> _buffer{};
    int  _linesInBuffer{0};

    int  _lineWidth{0};
    int  _linesPerPage{0};

    int  _currentPos{0};
    int  _scrollLine{0};
    int  _firstLineInBuffer{0};
    int  _scrollStopLine{INT_MAX};

    int  _promptStartPos{0};
    int  _promptEndPos{0};

    ScrollBarWidget* _scrollBar{nullptr};
    unique_ptr<ContextMenu>  myMouseMenu;
    unique_ptr<UndoHandler>  myUndoHandler;

    std::vector<string> _history;
    int _historyIndex{0};
    int _historyLine{0};
    int _tabCount{-1};
    char _inputStr[kLineBufferSize]{};

    int _kConsoleCharWidth{0}, _kConsoleCharHeight{0}, _kConsoleLineHeight{0};

    int  _selectSize{0};
    bool _isDragging{false};
    bool _inverse{false};
    bool _firstTime{true};
    bool _exitedEarly{false};

    // Set once the line width changes, meaning the buffer no longer matches the
    // geometry it was wrapped for; cleared by the next clearScreen()
    bool _bufferStale{false};

    int historyDir(int& index, int direction);
    void historyAdd(string_view entry);
    ContextMenu& mouseMenu();
    void recalcMetrics();

    int selectStartPos() const {
      return _selectSize < 0 ? _currentPos + _selectSize : _currentPos;
    }
    int selectEndPos() const {
      return _selectSize > 0 ? _currentPos + _selectSize : _currentPos;
    }
    void killSelectedText();

  private:
    // Following constructors and assignment operators not supported
    PromptWidget() = delete;
    PromptWidget(const PromptWidget&) = delete;
    PromptWidget(PromptWidget&&) = delete;
    PromptWidget& operator=(const PromptWidget&) = delete;
    PromptWidget& operator=(PromptWidget&&) = delete;
};

#endif  // PROMPT_WIDGET_HXX
