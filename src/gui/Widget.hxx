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
    Widget(GuiObject* boss, const GUI::Font& font, int x, int y, int w, int h);
    ~Widget() override = default;

    int getAbsX() const override { return _x + _boss->getChildX(); }
    int getAbsY() const override { return _y + _boss->getChildY(); }
    virtual int getLeft() const { return _x; }
    virtual int getTop() const { return _y; }
    virtual int getRight() const { return _x + getWidth(); }
    virtual int getBottom() const { return _y + getHeight(); }
    // The height needed to display this container's fixed content without
    // clipping; 0 unless recordContentHeight() has been called (e.g. for
    // widgets which simply fill their available area)
    int getContentHeight() const { return _contentHeight; }
    virtual void setPosX(int x);
    virtual void setPosY(int y);
    virtual void setPos(int x, int y);
    virtual void setPos(const Common::Point& pos);
    void setWidth(int w) override;
    void setHeight(int h) override;
    virtual void setSize(int w, int h);
    virtual void setSize(const Common::Point& pos);
    // Set position and size together.  The base just forwards to setPos() and
    // the virtual setWidth()/setHeight(); widgets that must react to a geometry
    // change as a whole (e.g. RomImageWidget rescales its image) override it.
    // This is the single entry point the layout manager uses to place a widget.
    virtual void setArea(int x, int y, int w, int h);

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

    void draw() override;
    void setDirty() override;
    void setDirtyChain() override;
    void receivedFocus();
    void lostFocus();
    void addFocusWidget(Widget* w) override { _focusList.push_back(w); }
    int addToFocusList(const WidgetArray& list) override {
      Vec::append(_focusList, list);
      return static_cast<int>(_focusList.size());
    }

    /** Set/clear FLAG_ENABLED */
    virtual void setEnabled(bool e);

    bool isEnabled() const          { return _flags & FLAG_ENABLED;         }
    bool isVisible() const override { return !(_flags & FLAG_INVISIBLE);    }
    bool isHighlighted() const      { return _flags & FLAG_HILITED; }
    bool hasMouseFocus() const      { return _flags & FLAG_MOUSE_FOCUS; }
    virtual bool wantsFocus() const { return _flags & FLAG_RETAIN_FOCUS;    }
    bool wantsTab() const           { return _flags & FLAG_WANTS_TAB;       }
    bool wantsRaw() const           { return _flags & FLAG_WANTS_RAWDATA;   }

    virtual void setID(uInt32 id) { _id = id;   }
    uInt32 getID() const  { return _id; }

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
      A self-labeling control (SliderWidget, PopUpWidget) draws its own label in
      a column to the left of its track/value box.  That column is inside the
      widget, so no layout can line one up with another's: instead a group of
      them is given ONE column, sized to the longest of their labels, by
      GUI::alignLabels() — which is what these three are for.  Everything else
      has no label of its own and reports nothing.

      naturalLabelWidth() is what my own label needs; labelWidth() is what I have
      been given; setLabelWidth() re-partitions me, keeping my track the width it
      already was.
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
    virtual void refreshFontMetrics();

    void setTextColor(ColorId color)   { _textcolor = color;   setDirty(); }
    void setTextColorHi(ColorId color) { _textcolorhi = color; setDirty(); }
    void setBGColor(ColorId color)     { _bgcolor = color;     setDirty(); }
    void setBGColorHi(ColorId color)   { _bgcolorhi = color;   setDirty(); }
    void setShadowColor(ColorId color) { _shadowcolor = color; setDirty(); }

    void setToolTip(string_view text,
      Event::Type event1 = Event::Type::NoType, EventMode = EventMode::kEmulationMode);
    void setToolTip(string_view text,
      Event::Type event1, Event::Type event2, EventMode = EventMode::kEmulationMode);
    void setToolTip(Event::Type event1, EventMode mode = EventMode::kEmulationMode);
    void setToolTip(Event::Type event1, Event::Type event2,
      EventMode mode = EventMode::kEmulationMode);
    virtual string getToolTip(const Common::Point& pos) const;
    virtual bool changedToolTip(const Common::Point& oldPos,
                                const Common::Point& newPos) const { return false; }

    void setHelpAnchor(string_view helpAnchor, bool debugger = false);
    void setHelpURL(string_view helpURL);

    virtual void loadConfig() { }

    /**
      Record the height needed to display this widget's content, i.e. the
      largest 'bottom' over the sibling widgets created under the same boss
      (excluding this widget itself).  The debugger's content widgets place
      their children on the boss directly, so this captures their fixed extent
      while ignoring the container, which is sized to fill the whole tab area.
      Used by resizeable dialogs to keep fixed tab content fully visible.
    */
    void recordContentHeight();

  protected:
    void drawChain() override;

    virtual void drawWidget(bool hilite) { }

    virtual void receivedFocusWidget() { }
    virtual void lostFocusWidget() { }

    virtual Widget* findWidget(int x, int y) { return this; }

    void releaseFocus() override { assert(_boss); _boss->releaseFocus(); }

    virtual bool wantsToolTip() const { return hasMouseFocus() && hasToolTip(); }
    virtual bool hasToolTip() const;

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override
         { assert(_boss); _boss->handleCommand(sender, cmd, data, id); }

    string getHelpURL() const override;
    bool hasHelp() const override { return !getHelpURL().empty(); }

  protected:
    GuiObject*  _boss{nullptr};
    const GUI::Font& _font;
    uInt32      _id{0};
    bool        _hasFocus{false};
    int         _fontWidth{0};
    int         _lineHeight{0};
    int         _contentHeight{0};
    ColorId     _bgcolor{kWidColor};
    ColorId     _bgcolorhi{kWidColor};
    ColorId     _bgcolorlo{kBGColorLo};
    ColorId     _textcolor{kTextColor};
    ColorId     _textcolorhi{kTextColorHi};
    ColorId     _textcolorlo{kBGColorLo};
    ColorId     _shadowcolor{kShadowColor};
    string      _toolTipText;
    Event::Type _toolTipEvent1{Event::NoType};
    Event::Type _toolTipEvent2{Event::NoType};
    EventMode   _toolTipMode{EventMode::kEmulationMode};
    string      _helpAnchor;
    string      _helpURL;
    bool        _debuggerHelp{false};

  public:
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
    static void refreshFontMetricsInList(const WidgetList& list);

  private:
    // Following constructors and assignment operators not supported
    Widget() = delete;
    Widget(const Widget&) = delete;
    Widget(Widget&&) = delete;
    Widget& operator=(const Widget&) = delete;
    Widget& operator=(Widget&&) = delete;
};

/* StaticTextWidget */
class StaticTextWidget : public Widget, public CommandSender
{
  public:
    enum {
      kClickedCmd = 'STcl',
      kOpenUrlCmd = 'STou'
    };

  public:
    StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h,
                     string_view text = "", TextAlign align = TextAlign::Left,
                     ColorId shadowColor = kNone);
    StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y,
                     string_view text = "", TextAlign align = TextAlign::Left,
                     ColorId shadowColor = kNone);
    ~StaticTextWidget() override = default;

    void setCmd(int cmd) { _cmd = cmd; }

    virtual void setValue(int value);
    void setLabel(string_view label);
    void setAlign(TextAlign align) { _align = align; setDirty(); }
    const string& getLabel() const { return _label; }
    bool isEditable() const { return _editable; }

    void setLink(size_t start = string::npos, int len = 0, bool underline = false);
    bool setUrl(string_view url = {}, string_view label = {},
                string_view placeHolder = {});
    const string& getUrl() const { return _url; }

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

    void refreshFontMetrics() override;

  protected:
    void drawWidget(bool hilite) override;

  protected:
    string    _label;
    bool      _editable{false};
    TextAlign _align{TextAlign::Left};
    int       _cmd{0};
    size_t    _linkStart{string::npos};
    int       _linkLen{0};
    bool      _linkUnderline{false};
    string    _url;

  private:
    // Following constructors and assignment operators not supported
    StaticTextWidget() = delete;
    StaticTextWidget(const StaticTextWidget&) = delete;
    StaticTextWidget(StaticTextWidget&&) = delete;
    StaticTextWidget& operator=(const StaticTextWidget&) = delete;
    StaticTextWidget& operator=(StaticTextWidget&&) = delete;
};

/* ButtonWidget */
class ButtonWidget : public StaticTextWidget
{
  public:
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h,
                 string_view label, int cmd = 0, bool repeat = false);
    /**
      Take this width, but size your own height.  For a button whose LABEL is not
      final — the command menu re-labels its buttons as the console state changes
      — since it must be as wide as the widest label it can ever show, and would
      otherwise resize under the user.  Everything else uses the ctor below.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w,
                 string_view label, int cmd = 0, bool repeat = false);

    /**
      Size me from my own label: as wide as it needs plus a comfortable margin,
      and a little taller than a line of text.  This is what a button standing on
      its own wants, so a dialog states nothing about it but the label — and I
      follow a live font change by myself (see refreshFontMetrics).

      Buttons that must share ONE width (a column of them, an OK/Cancel group)
      are still built this way: no button can know what the widest of its
      neighbours needs, so the LAYOUT equalizes them — see GUI::alignButtons().
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 string_view label, int cmd = 0, bool repeat = false);
    /**
      A raw bitmap, at a size you give me.  For the caller that draws its own
      graphic and must match it to something else (the high scores dialog's
      prev/next arrows, sized to the pop-up beside them).  An ICON belongs in one
      of the two below, which need no size at all.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int dw, int dh,
                 const uInt32* bitmap, int bmw, int bmh,
                 int cmd = 0, bool repeat = false);

    /**
      Size me from my own icon, and from my label if I have one: I am laid out
      around my bitmap (see drawWidget), so only I can say how much room that
      needs — nobody passes an icon button a size.  A dialog that swaps my icon
      for a different one — a larger variant for a larger font, a different
      state — just calls setIcon(), and I re-size to it.
    */
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 const GUI::Icon& icon,
                 int cmd = 0, bool repeat = false);
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 const GUI::Icon& icon, string_view label,
                 int cmd = 0, bool repeat = false);
    ~ButtonWidget() override = default;

    bool handleEvent(Event::Type event) override;

    /* Sets/changes the button's bitmap **/
    void setBitmap(const uInt32* bitmap, int bmw, int bmh);
    void setIcon(const GUI::Icon& icon);

    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseEntered() override;
    void handleMouseLeft() override;

    /*
      A button (and so a checkbox, and a radio button) draws its label INSIDE
      itself, not in a column to its left, so it has no label column to align and
      GUI::alignButtons() is what sizes a group of them.  These undo the
      StaticTextWidget behaviour we would otherwise inherit -- which would let
      GUI::alignLabels() resize a button to the width of its label alone.
      SliderWidget, which DOES draw a label beside its track, overrides them again
    */
    int naturalLabelWidth() const override { return 0; }
    int labelWidth() const override { return 0; }
    void setLabelWidth(int) override { }

    void refreshFontMetrics() override;

  public:
    // The room a button leaves around its bitmap: an icon-only button centers
    // its bitmap in this, and an icon-and-label one draws its label after it
    static int iconGap(const GUI::Font& font)
    {
      return ((font.getMaxCharWidth() + 1) & ~0b1) + 1;
    }

  protected:
    // The width my content needs: an icon-and-label button is laid out around
    // its bitmap -- a half-gap, the bitmap, a half-gap, then the label (see
    // drawWidget); an icon-only one just centers its bitmap; a plain one is
    // sized by its label alone
    int autoWidth() const
    {
      if(!_useBitmap)
        return calcWidth(_font, _label);

      return _useText
        ? _bmw + static_cast<int>(_bmx * 1.5) + _font.getStringWidth(_label)
        : _bmw + iconGap(_font);
    }

  public:

    // How tall a button is.  Unlike its width — which is its own business, and
    // which only GUI::alignButtons() ever overrides — a button's height is a unit
    // other things measure themselves against (a navigation bar is one button
    // tall, a file list four), so it is asked for from outside; Dialog::
    // buttonHeight() is the wrapper they use
    static int calcHeight(const GUI::Font& font)
    {
      return font.getLineHeight() * 1.25;
    }

    // The width this label needs, plus the margin that makes it look like a
    // button.  A button with a fixed label applies this itself and no one else
    // needs it; it is here for the dialog that must state a width because the
    // label is not final (see the width-only ctor above), which says so with a
    // specimen label rather than a pixel count
    static int calcWidth(const GUI::Font& font, string_view label)
    {
      return font.getStringWidth(label) + font.getMaxCharWidth() * 2.5;
    }
    // The same, for a label of the given length -- how a dialog states a button
    // width in characters rather than pixels (see Dialog::standardButtonWidth)
    static int calcWidth(const GUI::Font& font, int chars)
    {
      return font.getMaxCharWidth() * (chars + 2.5);
    }

  protected:
    void drawWidget(bool hilite) override;

  protected:
    bool _repeat{false}; // button repeats
    bool _useText{true};
    bool _useBitmap{false};
    const uInt32* _bitmap{nullptr};
    int  _bmw{0}, _bmh{0}, _bmx{0};
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
    enum { kCheckActionCmd  = 'CBAC' };
    enum class FillType: uInt8 { Normal, Inactive, Circle };

  public:
    CheckboxWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   string_view label, int cmd = 0);
    ~CheckboxWidget() override = default;

    void setEditable(bool editable);
    virtual void setFill(FillType type);

    virtual void setState(bool state, bool changed = false);
    void toggleState()     { setState(!_state); }
    bool getState() const  { return _state;     }

    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;

    void refreshFontMetrics() override;

    static int boxSize(const GUI::Font& font)
    {
      return font.isLarge() ? 22 : 14; // box is square
    }
    static int prefixSize(const GUI::Font& font)
    {
      return boxSize(font) + font.getMaxCharWidth() * 0.75;
    }

  protected:
    void drawWidget(bool hilite) override;

    // Compute the height and the box offset from the current font: the label is
    // centered in the height, like the text of any other control, and the box of
    // the given size is centered on the label.  Shared with RadioButtonWidget,
    // whose button takes the place of the box
    void alignBox(int boxSize);

  protected:
    bool _state{false};
    bool _holdFocus{true};
    bool _drawBox{true};
    bool _changed{false};

    const uInt32* _outerCircle{nullptr};
    const uInt32* _innerCircle{nullptr};
    const uInt32* _img{nullptr};
    ColorId _fillColor{kColor};
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
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h,
                 string_view label = "", int labelWidth = 0, int cmd = 0,
                 int valueLabelWidth = 0, string_view valueUnit = "",
                 int valueLabelGap = 0, bool forceLabelSign = false);

    /**
      Take this TRACK width, and my height from the font.  How long a track the
      dialog wants is its own decision; how tall a slider is never was.
    */
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w,
                 string_view label = "", int labelWidth = 0, int cmd = 0,
                 int valueLabelWidth = 0, string_view valueUnit = "",
                 int valueLabelGap = 0, bool forceLabelSign = false);

    // ...and with a default track width as well
    SliderWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y,
                 string_view label = "", int labelWidth = 0, int cmd = 0,
                 int valueLabelWidth = 0, string_view valueUnit = "",
                 int valueLabelGap = 0, bool forceLabelSign = false);
    ~SliderWidget() override = default;

    void setValue(int value) override;
    int getValue() const { return BSPF::clamp(_value, _valueMin, _valueMax); }

    void setMinValue(int value);
    int  getMinValue() const { return _valueMin; }
    void setMaxValue(int value);
    int  getMaxValue() const { return _valueMax; }
    void setStepValue(int value);
    int  getStepValue() const { return _stepValue; }
    void setValueLabel(string_view valueLabel);
    void setValueLabel(int value);
    const string& getValueLabel() const { return _valueLabel; }
    void setValueUnit(string_view valueUnit);
    void setTickmarkIntervals(int numIntervals);

    void handleMouseMoved(int x, int y) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleEvent(Event::Type event) override;

    int naturalLabelWidth() const override {
      return _label.empty() ? 0 : _font.getStringWidth(_label);
    }
    int labelWidth() const override { return _labelWidth; }
    void setLabelWidth(int w) override;

    /**
      My track: the part between my label and my value readout.  How long it is
      IS the dialog's decision (it says how finely the value can be dragged), but
      it says it by handing me a width, never by reaching inside me for the pieces.
    */
    int trackWidth() const {
      return _w - _labelWidth - _valueLabelGap - _valueLabelWidth;
    }
    void setTrackWidth(int w) {
      _w = w + _labelWidth + _valueLabelGap + _valueLabelWidth;
      setDirty();
    }

  protected:
    void drawWidget(bool hilite) override;

    int valueToPos(int value) const;
    int posToValue(int pos) const;

  protected:
    int    _value{-INT_MAX}, _stepValue{1};
    int    _valueMin{0}, _valueMax{100};
    bool   _isDragging{false};
    int    _labelWidth{0};
    string _valueLabel;
    string _valueUnit;
    int    _valueLabelGap{0};
    int    _valueLabelWidth{0};
    bool   _forceLabelSign{false};
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
