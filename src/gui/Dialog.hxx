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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DIALOG_HXX
#define DIALOG_HXX

class FBSurface;
class OSystem;
class DialogContainer;
class TabWidget;
class CommandSender;
class ToolTip;

namespace GUI {
  class Layout;
}  // namespace GUI

#include <functional>

#include "Widget.hxx"
#include "GuiObject.hxx"
#include "StellaKeys.hxx"
#include "EventHandlerConstants.hxx"
#include "bspf.hxx"

/**
  This is the base class for all dialog boxes.

  @author  Stephen Anthony
*/
class Dialog : public GuiObject
{
  friend class DialogContainer;

  public:
    // Current Stella mode
    enum class AppMode: uInt8 { launcher, emulator, debugger };

    using RenderCallback = std::function<void()>;

    Dialog(OSystem& instance, DialogContainer& parent,
           int w = 0, int h = 0);
    Dialog(OSystem& instance, DialogContainer& parent, const GUI::Font& font,
           string_view title = "", int w = 0, int h = 0);
    ~Dialog() override;

    /** Resets widget/focus/button-group state, ready to be rebuilt */
    void clear();
    /** Lays out and shows the dialog: sizes/allocates its surface, builds the
        focus list, calls loadConfig(), then pushes it onto the container's stack */
    void open();
    /** Releases focus and pops this dialog off the container's stack */
    void close();

    bool isVisible() const override { return _visible; }

    /** Positions the dialog's surface on screen, per the 'dialogpos' setting */
    virtual void setPosition();
    /** Draws the dialog's own chrome (background/title/border), then its children */
    virtual void drawDialog();
    /** Hooks a dialog overrides to read/write/reset its widgets from settings;
        called by open()/handleCommand() respectively, defaults do nothing */
    virtual void loadConfig()  { }
    virtual void saveConfig()  { }
    virtual void setDefaults() { }

    // A Dialog draws via drawDialog()/drawChain() (see redraw()), not draw();
    // this override keeps it out of the normal Widget draw chain
    void draw() override { }
    // Unlike Widget's, these don't propagate to a boss -- a Dialog IS the top
    void setDirty() override;
    void setDirtyChain() override;
    /** Redraws this dialog if visible, optionally forcing a full repaint */
    void redraw(bool force = false);
    /** Presents this dialog's surface (and any shading below a modal one) */
    void render();

    /**
      Re-run layout() for the current window size, resize the backing surface
      and repaint.  Safe to call repeatedly (e.g. on a window-resize event).
    */
    void relayout();

    /**
      Refresh all font-derived state after the dialog's font has been changed
      in place (see OSystem::refreshFonts), then re-run layout() so the whole
      dialog re-fonts live without being recreated.  Reached by our container's
      broadcast, which every dialog registers for as it is constructed -- so an
      owner never has to forward this to a dialog it holds.  Virtual for the
      subclasses carrying font-derived state of their own, outside the widget
      tree the base walks (ContextMenu's row height, DebuggerDialog's tooltip).
    */
    void refreshFont() override;

    /**
      Answers whether this dialog (at its current size and hidpi scaling) is
      larger than the screen it would be drawn into.  Used to detect when a
      font change would make a dialog too big for the current window.
    */
    bool exceedsScreen() const;

    void tick() override;

    // Register widgets so Tab/arrow navigation can reach them.  The tabId
    // overload files a list under one tab of 'w', so it is only scanned while
    // that tab is active; addBGroupToFocusList always goes last (see
    // buildCurrentFocusList), so the standard buttons are reached after tab content
    void addFocusWidget(Widget* w) override;
    int addToFocusList(const WidgetArray& list) override;
    int addToFocusList(const WidgetArray& list, const TabWidget* w, int tabId);
    void addBGroupToFocusList(const WidgetArray& list) { _buttonGroup = list; }
    /** Registers a top-level tab widget so its per-tab focus lists are tracked */
    void addTabWidget(TabWidget* w);
    // Remember which button plays which standard role, for layoutButtonGroup()
    // and for UIOK/UICancel navigation (see handleNavEvent)
    void addDefaultWidget(ButtonWidget* w) { _defaultWidget = w; }
    void addExtraWidget(ButtonWidget* w)   { _extraWidget = w;   }
    void addOKWidget(ButtonWidget* w)      { _okWidget = w;      }
    void addCancelWidget(ButtonWidget* w)  { _cancelWidget = w;  }
    /** Moves the input focus to 'w', if it isn't already there and wants focus */
    void setFocus(const Widget* w);

    /**
      If 'w' currently holds the dialog's focus, move focus to the next
      enabled widget instead (the same forward cycle Tab navigation uses).
      Called when a widget becomes hidden out from under the focus it
      holds, so the focused-widget highlight isn't left drawn over it.
    */
    void releaseFocus(const Widget* w);

    /** Returns the base surface associated with this dialog. */
    FBSurface& surface() const { return *_surface; }

    /**
      This method is called each time the main Dialog::render is called.
      It is called *after* the dialog has been rendered, so it can be
      used to render another surface on top of it, among other things.
    */
    void addRenderCallback(const RenderCallback& callback);

    /** Sets the title text and re-derives the title-bar height (_th) from it */
    void setTitle(string_view title);
    bool hasTitle() { return !_title.empty(); }

    /** Creates the title-bar '?' help button on first use, and shows/hides it
        depending on whether this dialog currently has any help to offer */
    void initHelp();
    /** Registers this dialog's own context-help target (see Widget::setHelpAnchor) */
    void setHelpAnchor(string_view helpAnchor, bool debugger = false);
    void setHelpURL(string_view helpURL);

    // Whether a dialog below this one on the stack is darkened while this one
    // is open (see render()); a non-modal-looking overlay (ContextMenu) says no
    virtual bool isShading() const { return true; }

    /**
      Determine the maximum width/height of a dialog based on the minimum
      allowable bounds, also taking into account the current window size.
      Currently scales the width/height to 95% of allowable area when possible.

      NOTE: This method is meant to be used for dynamic, resizeable dialogs.
            That is, those that can change size during a program run, and
            *have* to take the current window size into account.

      @param w  The resulting width to use for the dialog
      @param h  The resulting height to use for the dialog

      @return  True if the dialog fits in the current window (scaled to 90%)
               False if the dialog is smaller than the current window, and
               has to be scaled down
    */
    bool getDynamicBounds(uInt32& w, uInt32& h) const;

    /**
      Checks if the dialogs fits into the actual sizes.

      @param w  The resulting width to use for the dialog
      @param h  The resulting height to use for the dialog

      @return  True if the dialog should be resized
    */
    bool shouldResize(uInt32& w, uInt32& h) const;

    /** This dialog's tooltip popup */
    ToolTip& tooltip() { return *_toolTip; }

    // Font metrics, and the standard spacing/sizing constants layout() methods
    // derive from them -- so every dialog measures itself the same way
    int lineHeight() const { return _font.getLineHeight(); }
    int fontHeight() const { return _font.getFontHeight(); }
    int fontWidth() const { return _font.getMaxCharWidth(); }
    // How tall a button is — a unit other things are measured in (a navigation
    // bar is one button tall).  There is deliberately no buttonWidth(): a button
    // sizes itself to its label, and a GROUP of them is equalized by the layout
    // (GUI::alignButtons), so no dialog ever names a button's width
    int buttonHeight() const { return ButtonWidget::calcHeight(_font); }
    // The standard button group (Defaults / OK / Cancel) is the same size in
    // every dialog rather than shrink-wrapping to whatever its labels happen to
    // be, so it does not change shape from one dialog to the next.  Eight
    // characters is what the longest of them ("Defaults") needs
    int standardButtonWidth() const { return ButtonWidget::calcWidth(_font, 8); }
    int buttonGap() const { return fontWidth(); }
    int hBorder() const { return fontWidth() * 1.25; }
    int vBorder() const { return fontHeight() / 2; }
    int vGap() const { return fontHeight() / 4; }
    int indent() const { return fontWidth() * 2; }

  protected:
    struct Cmd {
      static constexpr GuiCmd::Code
        Help = GuiCmd::of("Dialog.Help");
    };

    // Remembers the focused widget's place in its tab(s) before it's released
    void releaseFocus() override;

    // Low-level input, dispatched here from DialogContainer for the topmost
    // dialog; the base routes to the focused widget (falling back to
    // handleNavEvent()/findWidget() as appropriate), a subclass overrides to add
    // dialog-specific behavior
    virtual void handleText(char text);
    virtual void handleKeyDown(StellaKey key, StellaMod modifiers, bool repeated = false);
    virtual void handleKeyUp(StellaKey key, StellaMod modifiers);
    virtual void handleMouseDown(int x, int y, MouseButton b, int clickCount);
    virtual void handleMouseUp(int x, int y, MouseButton b, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual void handleMouseMoved(int x, int y);
    virtual bool handleMouseClicks(int x, int y, MouseButton b);
    virtual void handleJoyDown(int stick, int button, bool longPress = false);
    virtual void handleJoyUp(int stick, int button);
    virtual void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button = JOY_CTRL_NONE);
    virtual bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button = JOY_CTRL_NONE);
    virtual void handleEvent(Event::Type event) {}
    // Reacts to a tab change (remembering it) and to the title-bar help/close buttons
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    virtual Event::Type getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button);

    Widget* findWidget(int x, int y) const; // Find the widget at pos x,y if any

    void drawChain() override;
    // Checked in order: the focused widget, the active tab (and its tab group),
    // then this dialog's own anchor/URL; empty if none of those has help
    string getHelpURL() const override;
    bool hasHelp() const override { return !getHelpURL().empty(); }

    // The standard button groups.  Each creates self-sizing buttons and lets
    // layoutButtonGroup() give them one shared width, so no caller passes one
    void addOKBGroup(WidgetArray& wid, const GUI::Font& font,
                     string_view okText = "OK");

    void addOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                           string_view okText = "OK",
                           string_view cancelText = "Cancel",
                           bool focusOKButton = true);

    void addDefaultsOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                                   string_view okText = "OK",
                                   string_view cancelText = "Cancel",
                                   string_view defaultsText = "Defaults",
                                   bool focusOKButton = true);

    // NOTE: This method, and the three above it, are due to be refactored at some
    //       point, since the parameter list is kind of getting ridiculous
    void addDefaultsExtraOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                                        string_view extraText, GuiCmd::Code extraCmd,
                                        string_view okText = "OK",
                                        string_view cancelText = "Cancel",
                                        string_view defaultsText = "Defaults",
                                        bool focusOKButton = true);

    // (Re)position the standard bottom button group — Defaults (and optional
    // Extra) at the left, OK/Cancel at the right (platform order) — along the
    // dialog's bottom edge for the current _w/_h.  Only the buttons that exist
    // are moved.  Used both when the group is first created and by a resizeable
    // dialog's layout() so the group follows font/size changes.
    void layoutButtonGroup();

    // The same, then lay 'bandContent' out in the part of the button band the
    // standard group leaves free: from the left group's end (or the border) to
    // the right group's start.  This is how a dialog puts its OWN content on the
    // button row — navigation buttons (About/Help/Browser) — without re-deriving
    // where the band is.
    void layoutButtonGroup(unique_ptr<GUI::Layout> bandContent);

    // Lay 'content' out across the WHOLE button band, with no standard group
    // placed at all.  For a dialog whose bottom row is entirely its own
    // (JoystickDialog: a status readout and buttons that deliberately keep
    // their natural widths rather than the standard group's).
    void layoutButtonBand(unique_ptr<GUI::Layout> content);

    /**
      The width the button group needs: Defaults (and any Extra) on the left,
      OK/Cancel on the right, with clearance between the two sides.  A dialog
      that sizes itself from its content (Layout::naturalSize) must still be wide
      enough for its buttons, which the content knows nothing about.
    */
    int buttonGroupWidth() const;

    // Position the title-bar help ('?') button at the top-right for the current
    // _w.  Called automatically after layout(), so dialogs that compute _w in
    // layout() get it placed correctly.
    void layoutHelp();

    // Lets UICancel close a dialog that has no cancel button of its own
    void processCancelWithoutWidget(bool state = true) { _processCancel = state; }
    virtual void processCancel() { close(); }

    /** Define the size (allowed) for the dialog. */
    void setSize(uInt32 w, uInt32 h, uInt32 max_w, uInt32 max_h);
    // Sets the surface's on-screen position from a 'dialogpos' setting value
    // (corner/center), staggering it by _layer so stacked dialogs don't overlap exactly
    void positionAt(uInt32 pos);

    /**
      (Re)compute this dialog's size and the positions/sizes of its child
      widgets for the current available area.  Unlike the constructor (which
      *creates* the widgets), layout() only sizes/positions them, so it may
      be called repeatedly — e.g. on a window-resize event.

      The base implementation does nothing; legacy dialogs still size
      themselves in their constructor.  Resizeable dialogs override this.
    */
    virtual void layout() { }

    // Whether a held joystick button/axis/hat repeats in this dialog; a
    // dialog that consumes long-press itself (e.g. for its own repeat scheme) says no
    virtual bool repeatEnabled() { return true; }

  private:
    // Rebuilds _focusList from _myFocus plus whichever tab's list is active
    // (switching tabs if 'tabID' names one), then _buttonGroup last
    void buildCurrentFocusList(int tabID = -1);
    // Tab/arrow/OK/Cancel/Help navigation, tried before a raw key/button reaches
    // the focused widget; returns whether it handled the event
    bool handleNavEvent(Event::Type e, bool repeated = false);
    // Updates _tabID to whichever top-level tab owns 'w', if any
    void getTabIdForWidget(const Widget* w);
    // Switches the active top-level tab by 'direction' (-1/+1); false if none is active
    bool cycleTab(int direction);
    // Opens getHelpURL() in the system browser
    void openHelp();

    /**
      The settings key under which a tab widget's selected tab is remembered,
      or the empty string for a dialog that cannot be identified (one built
      without a title).  The value is deliberately never registered as a
      permanent setting, so it lives only for this run.
    */
    string tabStateKey(const TabWidget* tab) const;

    // Remember / re-select a tab widget's chosen tab across reopens of the
    // dialog.  Owned by Dialog (rather than by each dialog that has tabs)
    // because the tab widgets are already registered here, via addTabWidget()
    void saveActiveTab(int tabID, int id);
    void restoreActiveTab(TabWidget* tab);

  protected:
    // The font this dialog (and its widgets, by default) draws with
    const GUI::Font& _font;

    // The widget currently under the mouse (receives entered/left/moved)
    Widget* _mouseWidget{nullptr};
    // The widget currently holding the input focus
    Widget* _focusedWidget{nullptr};
    // The widget a mouse-down originated in, while the button stays held
    Widget* _dragWidget{nullptr};
    // The standard button-group members this dialog has, if any (see addOK*BGroup)
    ButtonWidget* _defaultWidget{nullptr};
    ButtonWidget* _extraWidget{nullptr};
    ButtonWidget* _okWidget{nullptr};
    ButtonWidget* _cancelWidget{nullptr};

    // True while this dialog is open and on the container's stack
    bool    _visible{false};
    // Whether UICancel closes this dialog even with no cancel button (see
    // processCancelWithoutWidget)
    bool    _processCancel{false};
    // Current title text; may be rewritten live by setTitle(), unlike _builtTitle
    string  _title;
    // Title-bar height in pixels; 0 when untitled
    int     _th{0};
    // This dialog's position in the container's stack, used to stagger stacked dialogs
    int     _layer{0};
    // This dialog's tooltip popup
    unique_ptr<ToolTip> _toolTip;
    // Context-help target: an anchor into the manual, or a full URL; _debuggerHelp
    // picks the debugger manual over the main one
    string  _helpAnchor;
    string  _helpURL;
    bool    _debuggerHelp{false};
    // The title-bar '?' help button, created lazily by initHelp()
    ButtonWidget* _helpWidget{nullptr};

  private:
    // One focus list, and which of its widgets currently holds it
    struct Focus {
      Widget* widget{nullptr};
      WidgetArray list;

      explicit Focus(Widget* w = nullptr) : widget{w} { }
    };
    using FocusList = vector<Focus>;

    // A top-level tab widget, and a separate Focus per child tab so each
    // remembers its own focused widget across tab switches
    struct TabFocus {
      TabWidget* widget{nullptr};
      FocusList focus;
      // Which child tab 'focus' was last read for (see getNewFocus())
      uInt32 currentTab{0};

      explicit TabFocus(TabWidget* w = nullptr) : widget{w} { }

      // Appends the active tab's focus list to 'lst' (see buildCurrentFocusList)
      void appendFocusList(WidgetArray& lst);
      // Remembers 'w' as the active tab's focused widget, if 'w' belongs to it
      void saveCurrentFocus(Widget* w);
      // Switches currentTab to the now-active tab and returns its remembered widget
      Widget* getNewFocus();
    };
    using TabFocusList = vector<TabFocus>;

    Focus        _myFocus;    // focus for base dialog
    TabFocusList _myTabList;  // focus for each tab (if any)

    // The title this dialog was *built* with, which is what identifies it to
    // the tab memory.  Not _title, which setTitle() may rewrite while the
    // dialog is open (GameInfoDialog retitles itself for the current ROM),
    // leaving the saving and restoring halves looking at different keys
    const string _builtTitle;

    // The standard OK/Cancel/Defaults/Extra buttons, appended to the focus
    // list last (see buildCurrentFocusList)
    WidgetArray _buttonGroup;
    // This dialog's backing surface, and (while it is not the topmost dialog)
    // the overlay that darkens it (see render())
    shared_ptr<FBSurface> _surface;
    shared_ptr<FBSurface> _shadeSurface;

    // Index into _myTabList of the currently active top-level tab widget
    int _tabID{0};
    uInt32 _max_w{0}; // maximum wanted width
    uInt32 _max_h{0}; // maximum wanted height

    // Extra draw hook run after this dialog renders (see addRenderCallback)
    RenderCallback _renderCallback;

  private:
    // Following constructors and assignment operators not supported
    Dialog() = delete;
    Dialog(const Dialog&) = delete;
    Dialog(Dialog&&) = delete;
    Dialog& operator=(const Dialog&) = delete;
    Dialog& operator=(Dialog&&) = delete;
};

#endif  // DIALOG_HXX
