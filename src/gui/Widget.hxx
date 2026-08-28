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

#ifndef WIDGET_HXX
#define WIDGET_HXX

class Dialog;

#include <cassert>
#include <climits>

#include "bspf.hxx"
#include "Rect.hxx"
#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "FrameBufferConstants.hxx"
#include "StellaKeys.hxx"
#include "GuiObject.hxx"
#include "Font.hxx"
#include "Icon.hxx"

/**
  This is the base class for all widgets.

  @author  Stephen Anthony
*/
class Widget : public GuiObject
{
  friend class Dialog;

  public:
    /**
      A widget starts out with no size at all: every widget derives its own (from
      its font, its text, its item list, its row count), and the layout it sits in
      assigns the rest.  A subclass whose size IS its own business sets _w/_h in
      its body -- there is nothing the base could do with a size but store it.
    */
    Widget(GuiObject* boss, const GUI::Font& font);
    ~Widget() override = default;

    /** Screen position: this widget's local position plus the boss's own child offset */
    int getAbsX() const override { return _x + _boss->getChildX(); }
    int getAbsY() const override { return _y + _boss->getChildY(); }
    /** Edges, in the boss's local coordinate space */
    virtual int getLeft() const { return _x; }
    virtual int getTop() const  { return _y; }
    virtual int getRight() const  { return _x + getWidth();  }
    virtual int getBottom() const { return _y + getHeight(); }
    /** Moves the widget, marking the dialog dirty if the position actually changed */
    virtual void setPosX(int x);
    virtual void setPosY(int y);
    virtual void setPos(int x, int y);
    virtual void setPos(const Common::Point& pos);
    /** Resizes the widget, marking the dialog dirty if the size actually changed */
    void setWidth(int w) override;
    void setHeight(int h) override;
    virtual void setSize(int w, int h);
    virtual void setSize(const Common::Point& pos);
    // Set position and size together.  The base just forwards to setPos() and
    // the virtual setWidth()/setHeight(); widgets that must react to a geometry
    // change as a whole (e.g. RomImageWidget rescales its image) override it.
    // This is the single entry point the layout manager uses to place a widget.
    virtual void setArea(int x, int y, int w, int h);

    /** Input event hooks; override to react, default is "not handled" / no-op */
    virtual bool handleText(char text)                        { return false; }
    virtual bool handleKeyDown(StellaKey key, StellaMod mod)  { return false; }
    virtual bool handleKeyUp(StellaKey key, StellaMod mod)    { return false; }
    virtual void handleMouseDown(int x, int y, MouseButton b, int clickCount) { }
    virtual void handleMouseUp(int x, int y, MouseButton b, int clickCount) { }
    virtual void handleMouseEntered();
    virtual void handleMouseLeft();
    virtual void handleMouseMoved(int x, int y) { }
    virtual void handleMouseWheel(int x, int y, int direction) { }
    virtual bool handleMouseClicks(int x, int y, MouseButton b) { return false; }
    virtual void handleJoyDown(int stick, int button, bool longPress = false) { }
    virtual void handleJoyUp(int stick, int button) { }
    virtual void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button = JOY_CTRL_NONE) { }
    virtual bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button = JOY_CTRL_NONE) { return false; }
    virtual bool handleEvent(Event::Type event) { return false; }

    void tick() override;

    /** Draws this widget's content, then recurses into its children (see drawChain) */
    void draw() override;
    /** Marks this widget (or its children) dirty, and propagates dirtiness up to the boss */
    void setDirty() override;
    void setDirtyChain() override;
    /** Called by the focus manager when this widget gains/loses the input focus */
    void receivedFocus();
    void lostFocus();
    void addFocusWidget(Widget* w) override { _focusList.push_back(w); }
    int addToFocusList(const WidgetArray& list) override {
      Vec::append(_focusList, list);
      return static_cast<int>(_focusList.size());
    }

    /** Set/clear Flag::Enabled */
    virtual void setEnabled(bool e);

    // Show or hide this widget.  Prefer this to setting Flag::Invisible by hand:
    // a widget that goes invisible stops both drawing AND taking mouse events
    // (see findWidgetInList), and the area it was covering has to be repainted
    // by its boss, which only this knows to ask for
    virtual void setVisible(bool visible);

    /** Individual Flag queries */
    bool isEnabled() const          { return hasFlag(Flag::Enabled);      }
    bool isVisible() const override { return !hasFlag(Flag::Invisible);   }
    bool isHighlighted() const      { return hasFlag(Flag::Hilited);      }
    bool hasMouseFocus() const      { return hasFlag(Flag::MouseFocus);   }
    virtual bool wantsFocus() const { return hasFlag(Flag::RetainFocus);  }
    bool wantsTab() const           { return hasFlag(Flag::WantsTab);     }
    bool wantsRaw() const           { return hasFlag(Flag::WantsRawData); }

    /** A caller-assigned identifier used to distinguish widgets (not the command id) */
    virtual void setID(uInt32 id) { _id = id;   }
    uInt32 getID() const          { return _id; }

    /** The font this widget draws with */
    virtual const GUI::Font& font() const { return _font; }

    /**
      The size the widget would like to be (Qt calls this the size hint): what a
      layout gives it when it is neither filled nor stretched, and what
      GUI::BoxLayout::addAuto() sizes a cell from — so a row is as tall as its
      tallest widget without anyone hard-coding a height.  The default reports
      the current size, which for most widgets their constructor derives from the
      font; one whose constructor cannot know it overrides this.

      Deliberately distinct from GUI::Layout::minSize(), which is how far the
      content may be squeezed (a resizable dialog derives its window minimum from
      that).  Conflating the two would stop the launcher shrinking.
    */
    virtual Common::Size naturalSize() const {
      return Common::Size(std::max(_w, 0), std::max(_h, 0));
    }

    /**
      What GUI::alignLabels() uses to give a group of labels ONE column, sized to
      the longest of them, so that the controls they name line up down the group.

      Only LabelWidget answers meaningfully — it IS a label, so what it needs and
      what it has been given are simply its text width and its own width (see its
      overrides).  Everything else names nothing and reports 0, which is why a
      control must never be handed to alignLabels() in place of its label: that
      compiles and silently contributes nothing.

      naturalLabelWidth() is what my text needs; labelWidth() is what I have been
      given; setLabelWidth() resizes me to a column the layout has chosen.

      Nothing self-labels any more: SliderWidget and PopUpWidget used to draw
      their own label beside their track/value box, which no layout could line up
      from outside; they were split into a plain control plus an ordinary sibling
      LabelWidget, paired by GUI::labeledRow().
    */
    virtual int naturalLabelWidth() const { return 0; }
    virtual int labelWidth() const { return 0; }
    virtual void setLabelWidth(int w) { }

    /**
      The vertical offset, from the widget's top edge, at which the widget draws
      its first line of text.  A widget centers its text within its own height,
      so the default serves all the single-line controls, and two of them sharing
      a row line up once the layout centers each within the row.

      A widget that can show SEVERAL lines must override it, because centering
      would float its first line in the middle of a box whose whole purpose is to
      hold the lines below it: an EditTextWidget built two lines tall, or a data
      grid / toggle list, which is several rows of text in one box.  Such a widget
      reports where its first line starts, which is also the line a label beside
      it must sit on — GUI::VAlign::Baseline consumes exactly this.
    */
    virtual int firstTextY() const {
      return (_h - _font.getFontHeight()) / 2;
    }

    /**
      The referenced font's metrics may have changed at runtime (e.g. the user
      picked a different launcher font).  Re-read any cached font-derived state.
      The base refreshes the common metrics; widgets that cache additional
      font-derived values override and extend this.  Geometry is not touched
      here — the owning dialog's layout() repositions/resizes widgets after the
      refresh.
    */
    void refreshFont() override;

    /** Override the default/highlighted text, background, and shadow colors */
    void setTextColor(ColorId color)   { _textcolor = color;   setDirty(); }
    void setTextColorHi(ColorId color) { _textcolorhi = color; setDirty(); }
    void setBGColor(ColorId color)     { _bgcolor = color;     setDirty(); }
    void setBGColorHi(ColorId color)   { _bgcolorhi = color;   setDirty(); }
    void setShadowColor(ColorId color) { _shadowcolor = color; setDirty(); }

    /** Sets the tooltip text and/or the hotkey event(s) shown alongside it */
    void setToolTip(string_view text,
      Event::Type event1 = Event::Type::NoType, EventMode = EventMode::kEmulationMode);
    void setToolTip(string_view text,
      Event::Type event1, Event::Type event2, EventMode = EventMode::kEmulationMode);
    void setToolTip(Event::Type event1, EventMode mode = EventMode::kEmulationMode);
    void setToolTip(Event::Type event1, Event::Type event2,
      EventMode mode = EventMode::kEmulationMode);
    /** The tooltip text to show at 'pos', with any mapped hotkey(s) appended */
    virtual string getToolTip(const Common::Point& pos) const;
    /** Whether the tooltip must be recomputed as the mouse moves within this widget
        (e.g. a widget with several hover zones); the default never changes it */
    virtual bool changedToolTip(const Common::Point& oldPos,
                                const Common::Point& newPos) const { return false; }

    /** Registers this widget's context-help target: an anchor in the manual, or a full URL */
    void setHelpAnchor(string_view helpAnchor, bool debugger = false);
    void setHelpURL(string_view helpURL);

    /** Re-reads this widget's state from the underlying settings; the default does nothing */
    virtual void loadConfig() { }

  protected:
    /** Draws every child that needs it, then clears this widget's own dirty-chain flag */
    void drawChain() override;

    /** The actual per-widget drawing; draw() calls this after handling background/border */
    virtual void drawWidget(bool hilite) { }

    /** Hooks for a subclass to react to gaining/losing focus */
    virtual void receivedFocusWidget() { }
    virtual void lostFocusWidget() { }

    /** Hit-tests (x,y), in this widget's own coordinates; a composite widget looks inside itself */
    virtual Widget* findWidget(int x, int y) { return this; }

    void releaseFocus() override { assert(_boss); _boss->releaseFocus(); }

    /** Whether a tooltip should currently be shown / whether one is configured at all */
    virtual bool wantsToolTip() const { return hasMouseFocus() && hasToolTip(); }
    virtual bool hasToolTip() const;

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override
         { assert(_boss); _boss->handleCommand(sender, cmd, data, id); }

    /** The help URL, derived from _helpURL or _helpAnchor (see setHelpAnchor/setHelpURL) */
    string getHelpURL() const override;
    bool hasHelp() const override { return !getHelpURL().empty(); }

  protected:
    // The GuiObject that owns and positions this widget
    GuiObject*  _boss{nullptr};
    // The font this widget draws with
    const GUI::Font& _font;
    // Caller-assigned identifier (see setID/getID)
    uInt32      _id{0};
    // True while this widget holds the input focus
    bool        _hasFocus{false};
    // Cached font metrics, refreshed by refreshFont()
    int         _fontWidth{0};
    int         _fontHeight{0};
    int         _lineHeight{0};
    // Background/text/shadow colors in their normal, highlighted, and (where used) low states
    ColorId     _bgcolor{kWidColor};
    ColorId     _bgcolorhi{kWidColor};
    ColorId     _bgcolorlo{kBGColorLo};
    ColorId     _textcolor{kTextColor};
    ColorId     _textcolorhi{kTextColorHi};
    ColorId     _textcolorlo{kBGColorLo};
    ColorId     _shadowcolor{kShadowColor};
    // Tooltip text and the hotkey event(s)/mode shown alongside it (see setToolTip)
    string      _toolTipText;
    Event::Type _toolTipEvent1{Event::NoType};
    Event::Type _toolTipEvent2{Event::NoType};
    EventMode   _toolTipMode{EventMode::kEmulationMode};
    // Context-help target: an anchor into the manual, or a full URL; _debuggerHelp
    // picks the debugger manual over the main one
    string      _helpAnchor;
    string      _helpURL;
    bool        _debuggerHelp{false};

  public:
    /** Hit-tests (x,y) against every widget in 'list', newest first */
    static Widget* findWidgetInList(const WidgetList& list, int x, int y);

    /** Determine if 'find' is in the widget array */
    static bool isWidgetInList(const WidgetArray& list, Widget* find);

    /** Select either previous, current, or next widget in list to have
        focus, and deselects all others */
    static Widget* setFocusForList(const GuiObject* boss, WidgetArray& arr,
                                   const Widget* w, int direction,
                                   bool emitFocusEvents = true);

    /** Sets all widgets in this list to be dirty (must be redrawn) */
    static void setDirtyInList(const WidgetList& list);

    // Refresh font-derived state for an entire widget list, recursing into the
    // child widgets owned by composite widgets (which form their own lists).
    static void refreshFontInList(const WidgetList& list);

  private:
    // Following constructors and assignment operators not supported
    Widget() = delete;
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;
};

/* LabelWidget */
class LabelWidget : public Widget, public CommandSender
{
  public:
    // Command ids sendCommand()'d when a link is clicked / a URL link is clicked
    struct Cmd {
      static constexpr GuiCmd::Code
        Clicked = GuiCmd::of("LabelWidget.Clicked"),
        OpenUrl = GuiCmd::of("LabelWidget.OpenUrl");
    };

  protected:
    /**
      Take this size.  A label derives its own from its text (the ctor below), so
      nothing outside builds one this way -- it is here for ButtonWidget, whose
      size is its own affair and which passes it down.
    */
    LabelWidget(GuiObject* boss, const GUI::Font& font, int w, int h,
                string_view text = "", TextAlign align = TextAlign::Left,
                ColorId shadowColor = kNone);

  public:
    /**
      Size me from my own text, which is all a dialog ever states about a label.
    */
    LabelWidget(GuiObject* boss, const GUI::Font& font, string_view text = "",
                TextAlign align = TextAlign::Left, ColorId shadowColor = kNone);
    ~LabelWidget() override = default;

    /** The command sendCommand() is called with when this label is clicked as a link */
    void setCmd(GuiCmd::Code cmd) { _cmd = cmd; }

    /** Sets the label text to the given integer, formatted as a string */
    virtual void setValue(int value);
    /** Sets the label text; also resizes to fit it if setAutoResize(true) was called */
    void setLabel(string_view label);
    /** Whether setLabel() resizes this widget to fit its new text */
    void setAutoResize(bool state) { _autoResize = state; }
    void setAlign(TextAlign align) { _align = align; setDirty(); }
    const string& getLabel() const { return _label; }
    /** Whether this control accepts interaction (set by editable subclasses like CheckboxWidget) */
    bool isEditable() const { return _editable; }

    /** Marks label[start, start+len) as a link, optionally underlined; click sends Cmd::Clicked */
    void setLink(size_t start = string::npos, int len = 0, bool underline = false);
    /** Turns (part of) the label into a clickable URL link; returns whether one was found/set */
    bool setUrl(string_view url = {}, string_view label = {},
                string_view placeHolder = {});
    const string& getUrl() const { return _url; }

    /** Hover/click handling for the label's link, if it has one */
    void handleMouseEntered() override;
    void handleMouseLeft() override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;

    /*
      A static text IS a label, so what it needs and what it has been given are
      simply its text and its width.  Saying so lets GUI::alignLabels() give a
      group of them one column -- exactly as it does for the controls that draw
      their own label -- so the widgets they name line up beside them, and no
      dialog measures a label or pads one with trailing spaces to fake a column.
    */
    int naturalLabelWidth() const override { return _font.getStringWidth(_label); }
    int labelWidth() const override { return _w; }
    void setLabelWidth(int w) override { setWidth(w); }

    /** Re-derives this label's size from the live font, like the auto-size ctor */
    void refreshFont() override;

  protected:
    /** Draws the label text (and its link underline/highlight, if any) */
    void drawWidget(bool hilite) override;

  protected:
    // The text drawn
    string    _label;
    // Whether this control accepts interaction (see isEditable())
    bool      _editable{false};
    // Whether setLabel() resizes this widget to fit new text
    bool      _autoResize{false};
    // Horizontal alignment of the text
    TextAlign _align{TextAlign::Left};
    // Command sent (via CommandSender) on click; 0 means non-interactive
    GuiCmd::Code _cmd{GuiCmd::None};
    // Byte range within _label rendered as a link (npos/0 = none), and whether underlined
    size_t    _linkStart{string::npos};
    int       _linkLen{0};
    bool      _linkUnderline{false};
    // The URL a link points to, set by setUrl()
    string    _url;

  private:
    // Following constructors and assignment operators not supported
    LabelWidget() = delete;
    LabelWidget(const LabelWidget&) = delete;
    LabelWidget(LabelWidget&&) = delete;
    LabelWidget& operator=(const LabelWidget&) = delete;
    LabelWidget& operator=(LabelWidget&&) = delete;
};

/* ButtonWidget */
class ButtonWidget : public LabelWidget
{
  public:
    /**
      Size me from my own label: as wide as it needs plus a comfortable margin,
      and a little taller than a line of text.  This is what a button standing on
      its own wants, so a dialog states nothing about it but the label — and I
      follow a live font change by myself (see refreshFont).

      Buttons that must share ONE width (a column of them, an OK/Cancel group)
      are still built this way: no button can know what the widest of its
      neighbours needs, so the LAYOUT equalizes them — see GUI::alignButtons().
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 string_view label, GuiCmd::Code cmd = GuiCmd::None,
                 bool repeat = false);
    /**
      An icon, at a size you give me.  For the caller whose button must match
      something else (the high scores dialog's prev/next arrows, sized to the
      pop-up beside them).  A button free to size itself belongs in one of the
      two below, which need no size at all.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font, int dw, int dh,
                 const GUI::Icon& icon, GuiCmd::Code cmd = GuiCmd::None,
                 bool repeat = false);

    /**
      Size me from my own icon, and from my label if I have one: I am laid out
      around my bitmap (see drawWidget), so only I can say how much room that
      needs — nobody passes an icon button a size.  A dialog that swaps my icon
      for a different one — a larger variant for a larger font, a different
      state — just calls setIcon(), and I re-size to it.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font, const GUI::Icon& icon,
                 GuiCmd::Code cmd = GuiCmd::None, bool repeat = false);
    ButtonWidget(GuiObject* boss, const GUI::Font& font, const GUI::Icon& icon,
                 string_view label, GuiCmd::Code cmd = GuiCmd::None,
                 bool repeat = false);
    ~ButtonWidget() override = default;

    /** Fires the button on Event::UISelect (simulates a mouse click) */
    bool handleEvent(Event::Type event) override;

    /* Sets/changes the button's icon **/
    void setIcon(const GUI::Icon& icon);

    // Trim the self-size margin so a small button (a debugger op button) is not
    // as big as a dialog button.  Only affects an auto-sized (label-only) button
    void setCompact(bool compact = true)
    {
      _compact = compact;
      if(_autoSize)
      {
        setWidth(autoWidth());
        setHeight(autoHeight());
      }
    }

    // A repeating button fires on mouse-down (and keeps firing via handleMouseClicks);
    // a normal button fires once, on mouse-up
    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseEntered() override;
    void handleMouseLeft() override;

    /*
      A button (and so a checkbox, and a radio button) draws its label INSIDE
      itself, not in a column to its left, so it has no label column to align and
      GUI::alignButtons() is what sizes a group of them.  These undo the
      LabelWidget behaviour we would otherwise inherit -- which would let
      GUI::alignLabels() resize a button to the width of its label alone.
    */
    int naturalLabelWidth() const override { return 0; }
    int labelWidth() const override { return 0; }
    void setLabelWidth(int) override { }

    // Re-derives an auto-sized button's dimensions from its label/icon; a button
    // sized from outside keeps its size (see the .cxx for the full rationale)
    void refreshFont() override;

  public:
    // The room a button leaves around its bitmap: an icon-only button centers
    // its bitmap in this, and an icon-and-label one draws its label after it
    static int iconGap(const GUI::Font& font) {
      return ((font.getMaxCharWidth() + 1) & ~0b1) + 1;
    }

  protected:
    /**
      Take a size from outside.  PROTECTED on purpose: a dialog must not bake a
      button's geometry at construction — that size is never re-derived on a live
      font change (Widget::refreshFont leaves _w/_h alone, and this c'tor
      leaves _autoSize false), so it goes stale.  Size yourself from your label
      or icon and let layout() say the rest.  Only a SUBCLASS that is a button
      of its own kind (TimeLineWidget, the launcher's path button) uses this.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font, int w, int h,
                 string_view label, GuiCmd::Code cmd = GuiCmd::None,
                 bool repeat = false);

    // The width my content needs: an icon-and-label button is laid out around
    // its icon -- a half-gap, the icon, a half-gap, then the label (see
    // drawWidget); an icon-only one just centers its icon; a plain one is
    // sized by its label alone
    int autoWidth() const
    {
      if(_icon == nullptr)
        return _compact
          ? _font.getStringWidth(_label)
              + static_cast<int>(_font.getMaxCharWidth() * 1.25)
          : calcWidth(_font, _label);

      return _useText
        ? _icon->width() + static_cast<int>(_bmx * 1.5)
            + _font.getStringWidth(_label)
        : _icon->width() + iconGap(_font);
    }

    // The height my content needs.  A compact button keeps to its line and no
    // more; the standard margin is what gives a dialog button its presence,
    // which a small op button beside a grid does not want
    int autoHeight() const {
      return _compact ? _font.getLineHeight() : calcHeight(_font);
    }

  public:
    // How tall a button is.  Unlike its width — which is its own business, and
    // which only GUI::alignButtons() ever overrides — a button's height is a unit
    // other things measure themselves against (a navigation bar is one button
    // tall, a file list four), so it is asked for from outside; Dialog::
    // buttonHeight() is the wrapper they use
    static int calcHeight(const GUI::Font& font) {
      return font.getLineHeight() * 1.25;
    }

    // The width this label needs, plus the margin that makes it look like a
    // button.  A button with a fixed label applies this itself and no one else
    // needs it; it is here for the dialog that must state a width because the
    // label is not final (see the width-only ctor above), which says so with a
    // specimen label rather than a pixel count
    static int calcWidth(const GUI::Font& font, string_view label) {
      return font.getStringWidth(label) + font.getMaxCharWidth() * 2.5;
    }
    // The same, for a label of the given length -- how a dialog states a button
    // width in characters rather than pixels (see Dialog::standardButtonWidth)
    static int calcWidth(const GUI::Font& font, int chars) {
      return font.getMaxCharWidth() * (chars + 2.5);
    }

  protected:
    /** Draws the button's frame, then its icon and/or label */
    void drawWidget(bool hilite) override;

  protected:
    bool _repeat{false}; // button repeats
    // Whether the label is drawn (an icon-only button leaves it false)
    bool _useText{true};
    bool _compact{false}; // trim the self-size margin (a small op button)
    // What I draw beside or instead of my label; null means label-only.  Icons
    // are constexpr and outlive every button holding one
    const GUI::Icon* _icon{nullptr};
    // Gap between icon and label, in pixels (see iconGap()); 0 for an icon-only button
    int  _bmx{0};
    // Set only by the label-only ctor: I sized myself, so a font change re-sizes
    // me.  A button whose size came from elsewhere (an icon's extents, a width
    // the layout imposes) keeps it, and the layout re-applies it
    bool _autoSize{false};

  private:
    // Following constructors and assignment operators not supported
    ButtonWidget() = delete;
    ButtonWidget(const ButtonWidget&) = delete;
    ButtonWidget(ButtonWidget&&) = delete;
    ButtonWidget& operator=(const ButtonWidget&) = delete;
    ButtonWidget& operator=(ButtonWidget&&) = delete;
};

/* CheckboxWidget */
class CheckboxWidget : public ButtonWidget
{
  public:
    struct Cmd {
      static constexpr GuiCmd::Code
        CheckAction = GuiCmd::of("LabelWidget.CheckAction");
    };
    // How the checked box is filled: a solid square, a "greyed out" cross
    // (Inactive), or a filled circle (Circle, for a radio-button-like use)
    enum class FillType: uInt8 { Normal, Inactive, Circle };

  public:
    CheckboxWidget(GuiObject* boss, const GUI::Font& font,
                   string_view label, GuiCmd::Code cmd = GuiCmd::None);
    ~CheckboxWidget() override = default;

    /** Whether the box can be toggled by clicking; an uneditable one is greyed out */
    void setEditable(bool editable);
    /** Chooses how a checked box is drawn (see FillType) */
    virtual void setFill(FillType type);

    /** Sets the checked state; 'changed' additionally flags it as changed-from-default */
    virtual void setState(bool state, bool changed = false);
    void toggleState()     { setState(!_state); }
    bool getState() const  { return _state;     }

    /** Toggles the state on click, if editable */
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;

    /** Re-derives the box size and label layout from the live font */
    void refreshFont() override;

    /** Side length of the (square) checkbox for the given font */
    static int boxSize(const GUI::Font& font) {
      return font.isLarge() ? 22 : 14; // box is square
    }
    /** Horizontal space the box plus its gap take up, before the label starts */
    static int prefixSize(const GUI::Font& font) {
      return boxSize(font) + font.getMaxCharWidth() * 0.75;
    }

  protected:
    /** Draws the box (frame + fill/tick) and the label beside it */
    void drawWidget(bool hilite) override;

    // Compute the height and the box offset from the current font: the label is
    // centered in the height, like the text of any other control, and the box of
    // the given size is centered on the label.  Shared with RadioButtonWidget,
    // whose button takes the place of the box
    void alignBox(int boxSize);

  protected:
    // Whether the box is currently checked
    bool _state{false};
    // Whether the box frame is drawn at all (false for FillType::Circle)
    bool _drawBox{true};
    // Set by setState(..., true); drawn with kDbgChangedColor instead of the normal fill
    bool _changed{false};

    // Outer/inner ring icons for the radio-button variant (see RadioButtonWidget);
    // unused (null) for a plain checkbox
    const GUI::Icon* _outerCircle{nullptr};
    const GUI::Icon* _innerCircle{nullptr};
    // The tick/fill icon drawn when checked, per setFill(); null means a plain
    // filled square (FillType::Normal)
    const GUI::Icon* _img{nullptr};
    // Vertical offset of the box within the widget (see alignBox())
    int _boxY{0};
    int _boxSize{14};

  private:
    // Following constructors and assignment operators not supported
    CheckboxWidget() = delete;
    CheckboxWidget(const CheckboxWidget&) = delete;
    CheckboxWidget(CheckboxWidget&&) = delete;
    CheckboxWidget& operator=(const CheckboxWidget&) = delete;
    CheckboxWidget& operator=(CheckboxWidget&&) = delete;
};

/* SliderWidget */
class SliderWidget : public ButtonWidget
{
  public:
    /**
      Build me from CHARACTER counts, not pixels, the way EditTextWidget does:
      how many characters long a TRACK the dialog wants (0 = a reasonable
      default), and how many characters wide the value readout beside it must be
      (0 = none).  I turn those into pixels from the font.  How long a track the
      dialog wants is its own decision; how tall a slider is never was, so there
      is no height.
    */
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int trackChars = 0, GuiCmd::Code cmd = GuiCmd::None,
                 int valueChars = 0, string_view valueUnit = "",
                 int valueLabelGap = 0, bool forceLabelSign = false);
    ~SliderWidget() override = default;

    /** Clamps to [min, max], updates the value label, and sends _cmd if it actually changed */
    void setValue(int value) override;
    int getValue() const { return BSPF::clamp(_value, _valueMin, _valueMax); }

    void setMinValue(int value);
    int  getMinValue() const { return _valueMin; }
    void setMaxValue(int value);
    int  getMaxValue() const { return _valueMax; }
    // The amount UIUp/UIDown (and friends) move the value by
    void setStepValue(int value);
    int  getStepValue() const { return _stepValue; }
    /** The text shown in the value readout beside the track */
    void setValueLabel(string_view valueLabel);
    /** Sets the value label from an integer, applying the +/- sign if forceLabelSign was set */
    void setValueLabel(int value);
    const string& getValueLabel() const { return _valueLabel; }
    void setValueUnit(string_view valueUnit);
    /** Number of evenly-spaced tickmarks drawn along the track; 0 draws none */
    void setTickmarkIntervals(int numIntervals);

    // Dragging (mouse down + move within the track) and the wheel/keyboard both
    // just resolve to a new value via setValue()
    void handleMouseMoved(int x, int y) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleEvent(Event::Type event) override;

    /**
      My track: the part between my left edge and my value readout.  How long
      it is IS the dialog's decision (it says how finely the value can be
      dragged), but it says it by handing me a width, never by reaching inside
      me for the pieces.
    */
    int trackWidth() const {
      return _w - _valueLabelGap - _valueLabelWidth;
    }
    void setTrackWidth(int w) {
      _w = w + _valueLabelGap + _valueLabelWidth;
      setDirty();
    }

  protected:
    /** Draws the track, filled bar, tickmarks, handle, and value readout */
    void drawWidget(bool hilite) override;

    // Converts between a value in [_valueMin, _valueMax] and its pixel offset
    // along the track; used to draw the handle and to interpret a drag/click
    int valueToPos(int value) const;
    int posToValue(int pos) const;

  protected:
    // Current value, and the amount each step (keyboard/wheel) changes it by
    int    _value{-INT_MAX}, _stepValue{1};
    int    _valueMin{0}, _valueMax{100};
    // True while the handle is being dragged
    bool   _isDragging{false};
    // Text and unit suffix shown in the value readout beside the track
    string _valueLabel;
    string _valueUnit;
    // Gap before the readout, and the readout's own width, in pixels
    int    _valueLabelGap{0};
    int    _valueLabelWidth{0};
    // Whether a positive value label is prefixed with '+'
    bool   _forceLabelSign{false};
    // Number of tickmark intervals drawn along the track (see setTickmarkIntervals)
    int    _numIntervals{0};

  private:
    // Following constructors and assignment operators not supported
    SliderWidget() = delete;
    SliderWidget(const SliderWidget&) = delete;
    SliderWidget(SliderWidget&&) = delete;
    SliderWidget& operator=(const SliderWidget&) = delete;
    SliderWidget& operator=(SliderWidget&&) = delete;
};

#endif  // WIDGET_HXX
//
