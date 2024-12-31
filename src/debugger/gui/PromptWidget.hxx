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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PROMPT_WIDGET_HXX
#define PROMPT_WIDGET_HXX

#include <cstdarg>

class ScrollBarWidget;
class FSNode;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

// TODO - remove this once we clean up the printf stuff
#if defined(BSPF_UNIX) || defined(BSPF_MACOS)
  #define ATTRIBUTE_FMT_PRINTF __attribute__((__format__ (__printf__, 2, 0)))
#else
  #define ATTRIBUTE_FMT_PRINTF
#endif

class PromptWidget : public Widget, public CommandSender
{
  public:
    PromptWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h);
    ~PromptWidget() override = default;

  public:
    void print(string_view str);
    void printPrompt();
    string saveBuffer(const FSNode& file);

    // Clear screen
    void clearScreen();
    // Erase all history
    void clearHistory();

    void addToHistory(const char *str);

    bool isLoaded() const { return !_firstTime; }

  protected:
    ATTRIBUTE_FMT_PRINTF int printf(const char* format, ...);
    ATTRIBUTE_FMT_PRINTF int vprintf(const char* format, va_list argptr);
    int& buffer(int idx) { return _buffer[idx % kBufferSize]; }

    void drawWidget(bool hilite) override;
    void drawCaret();
    void putcharIntern(int c);
//    void insertIntoPrompt(const char *str);
    void updateScrollBuffer();
    void scrollToCurrent();

    // Line editing
    void nextLine();
    void killChar(int direction);
    void killLine(int direction);
    void killWord();

    // Clipboard
    string getLine();
    void textCut();
    void textCopy();
    void textPaste();

    // History
    bool historyScroll(int direction);

    bool execute();
    bool autoComplete(int direction);

    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Account for the extra width of embedded scrollbar
    int getWidth() const override;

    bool wantsFocus() const override { return true; }
    void loadConfig() override;

    void resetFunctions();

  private:
    enum: uInt16 {
      kBufferSize = 32768,
      kLineBufferSize = 256,
      kHistorySize = 1000
    };

    int  _buffer[kBufferSize]{};  // NOLINT  (will be rewritten soon)
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

    std::vector<string> _history;
    int _historyIndex{0};
    int _historyLine{0};
    int _tabCount{-1};
    char _inputStr[kLineBufferSize]{};

    int _kConsoleCharWidth{0}, _kConsoleCharHeight{0}, _kConsoleLineHeight{0};

    bool _inverse{false};
    bool _firstTime{true};
    bool _exitedEarly{false};

    int historyDir(int& index, int direction);
    void historyAdd(string_view entry);

  private:
    // Following constructors and assignment operators not supported
    PromptWidget() = delete;
    PromptWidget(const PromptWidget&) = delete;
    PromptWidget(PromptWidget&&) = delete;
    PromptWidget& operator=(const PromptWidget&) = delete;
    PromptWidget& operator=(PromptWidget&&) = delete;
};

#endif
