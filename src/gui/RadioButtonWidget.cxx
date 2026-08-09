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

#include "FBSurface.hxx"
#include "Font.hxx"
#include "Dialog.hxx"
#include "RadioButtonWidget.hxx"

/*  Radiobutton bitmaps */
// small versions
static constexpr std::array<uInt32, 14> radio_img_outercircle_bits = {
  0b00001111110000,  0b00110000001100,  0b01000000000010,
  0b01000000000010,  0b10000000000001,  0b10000000000001,
  0b10000000000001,  0b10000000000001,  0b10000000000001,
  0b10000000000001,  0b01000000000010,  0b01000000000010,
  0b00110000001100,  0b00001111110000
};
static constexpr GUI::Icon radio_img_outercircle(14, 14, radio_img_outercircle_bits);
static constexpr std::array<uInt32, 12> radio_img_innercircle_bits = {
  0b000111111000,  0b011111111110,  0b011111111110,  0b111111111111,
  0b111111111111,  0b111111111111,  0b111111111111,  0b111111111111,
  0b111111111111,  0b011111111110,  0b011111111110,  0b000111111000
};
static constexpr GUI::Icon radio_img_innercircle(12, 12, radio_img_innercircle_bits);
static constexpr std::array<uInt32, 10> radio_img_active_bits = {
  0b0011111100,  0b0111111110,  0b1111111111,  0b1111111111,  0b1111111111,
  0b1111111111,  0b1111111111,  0b1111111111,  0b0111111110,  0b0011111100,
};
static constexpr GUI::Icon radio_img_active(10, 10, radio_img_active_bits);
static constexpr std::array<uInt32, 10> radio_img_inactive_bits = {
  0b0011111100,  0b0111111110,  0b1111001111,  0b1110000111,  0b1100000011,
  0b1100000011,  0b1110000111,  0b1111001111,  0b0111111110,  0b0011111100
};
static constexpr GUI::Icon radio_img_inactive(10, 10, radio_img_inactive_bits);

// large versions
static constexpr std::array<uInt32, 22> radio_img_outercircle_large_bits = {
  // thinner version
  //0b0000000011111100000000,
  //0b0000001100000011000000,
  //0b0000110000000000110000,
  //0b0001000000000000001000,
  //0b0010000000000000000100,
  //0b0010000000000000000100,
  //0b0100000000000000000010,
  //0b0100000000000000000010,
  //0b1000000000000000000001,
  //0b1000000000000000000001,
  //0b1000000000000000000001,
  //0b1000000000000000000001,
  //0b1000000000000000000001,
  //0b1000000000000000000001,
  //0b0100000000000000000010,
  //0b0100000000000000000010,
  //0b0010000000000000000100,
  //0b0010000000000000000100,
  //0b0001000000000000001000,
  //0b0000110000000000110000,
  //0b0000001100000011000000,
  //0b0000000011111100000000

  0b0000000011111100000000,
  0b0000001110000111000000,
  0b0000111000000001110000,
  0b0001100000000000011000,
  0b0011000000000000001100,
  0b0010000000000000000100,
  0b0110000000000000000110,
  0b0100000000000000000010,
  0b1100000000000000000011,
  0b1000000000000000000001,
  0b1000000000000000000001,
  0b1000000000000000000001,
  0b1000000000000000000001,
  0b1100000000000000000011,
  0b0100000000000000000010,
  0b0110000000000000000110,
  0b0010000000000000000100,
  0b0011000000000000001100,
  0b0001100000000000011000,
  0b0000111000000001110000,
  0b0000001110000111000000,
  0b0000000011111100000000

};
static constexpr GUI::Icon radio_img_outercircle_large(22, 22, radio_img_outercircle_large_bits);
static constexpr std::array<uInt32, 20> radio_img_innercircle_large_bits = {
  //0b00000001111110000000,
  //0b00000111111111100000,
  //0b00011111111111111000,
  //0b00111111111111111100,
  //0b00111111111111111100,
  //0b01111111111111111110,
  //0b01111111111111111110,
  //0b11111111111111111111,
  //0b11111111111111111111,
  //0b11111111111111111111,
  //0b11111111111111111111,
  //0b11111111111111111111,
  //0b11111111111111111111,
  //0b01111111111111111110,
  //0b01111111111111111110,
  //0b00111111111111111100,
  //0b00111111111111111100,
  //0b00011111111111111000,
  //0b00000111111111100000,
  //0b00000001111110000000

  0b00000000111100000000,
  0b00000011111111000000,
  0b00001111111111110000,
  0b00011111111111111000,
  0b00111111111111111100,
  0b00111111111111111100,
  0b01111111111111111110,
  0b01111111111111111110,
  0b11111111111111111111,
  0b11111111111111111111,
  0b11111111111111111111,
  0b11111111111111111111,
  0b01111111111111111110,
  0b01111111111111111110,
  0b00111111111111111100,
  0b00111111111111111100,
  0b00011111111111111000,
  0b00001111111111110000,
  0b00000011111111000000,
  0b00000000111100000000

};
static constexpr GUI::Icon radio_img_innercircle_large(20, 20, radio_img_innercircle_large_bits);
static constexpr std::array<uInt32, 18> radio_img_active_large_bits = {
  //0b000000111111000000,
  //0b000011111111110000,
  //0b000111111111111000,
  //0b001111111111111100,
  //0b011111111111111110,
  //0b011111111111111110,
  //0b111111111111111111,
  //0b111111111111111111,
  //0b111111111111111111,
  //0b111111111111111111,
  //0b111111111111111111,
  //0b111111111111111111,
  //0b011111111111111110,
  //0b011111111111111110,
  //0b001111111111111100,
  //0b000111111111111000,
  //0b000011111111110000,
  //0b000000111111000000

  0b000000000000000000,
  0b000000111111000000,
  0b000011111111110000,
  0b000111111111111000,
  0b001111111111111100,
  0b001111111111111100,
  0b011111111111111110,
  0b011111111111111110,
  0b011111111111111110,
  0b011111111111111110,
  0b011111111111111110,
  0b011111111111111110,
  0b001111111111111100,
  0b001111111111111100,
  0b000111111111111000,
  0b000011111111110000,
  0b000000111111000000,
  0b000000000000000000
};
static constexpr GUI::Icon radio_img_active_large(18, 18, radio_img_active_large_bits);
static constexpr std::array<uInt32, 18> radio_img_inactive_large_bits = {
  //0b000001111111100000,
  //0b000111111111111000,
  //0b001111111111111100,
  //0b011111100001111110,
  //0b011110000000011110,
  //0b111100000000001111,
  //0b111100000000001111,
  //0b111000000000000111,
  //0b111000000000000111,
  //0b111000000000000111,
  //0b111000000000000111,
  //0b111100000000001111,
  //0b111100000000001111,
  //0b011110000000011110,
  //0b011111100001111110,
  //0b001111111111111100,
  //0b010111111111111000,
  //0b000001111111100000

  0b000000000000000000,
  0b000000111111000000,
  0b000011111111110000,
  0b000111111111111000,
  0b001111100001111100,
  0b001111000000111100,
  0b011110000000011110,
  0b011100000000001110,
  0b011100000000001110,
  0b011100000000001110,
  0b011100000000001110,
  0b011110000000011110,
  0b001111000000111100,
  0b001111100001111100,
  0b000111111111111000,
  0b000011111111110000,
  0b000000111111000000,
  0b000000000000000000
};
static constexpr GUI::Icon radio_img_inactive_large(18, 18, radio_img_inactive_large_bits);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RadioButtonWidget::RadioButtonWidget(GuiObject* boss, const GUI::Font& font,
                                     const string& label,
                                     RadioButtonGroup* group, GuiCmd::Code cmd)
  : CheckboxWidget(boss, font, label, cmd),
    myGroup{group},
    _buttonSize{buttonSize(font)} // 14 | 22
{
  _flags = Widget::Flag::Enabled;
  _bgcolor = _bgcolorhi = kWidColor;

  _editable = true;

  if(_buttonSize == 14)
  {
    _outerCircle = &radio_img_outercircle;
    _innerCircle = &radio_img_innercircle;
  }
  else
  {
    _outerCircle = &radio_img_outercircle_large;
    _innerCircle = &radio_img_innercircle_large;
  }

  if(label.empty())
    _w = _buttonSize;
  else
    _w = font.getStringWidth(label) + _buttonSize + font.getMaxCharWidth() * 0.75;
  alignBox(static_cast<int>(_buttonSize));

  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
  setFill(CheckboxWidget::FillType::Normal);
  myGroup->addWidget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonWidget::refreshFont()
{
  // Bypass CheckboxWidget's version: a radio button has its own button size and
  // outer/inner circle images that must be re-selected for the live font
  // (mirrors the ctor).
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  Widget::refreshFont();

  _buttonSize = buttonSize(_font);
  if(_buttonSize == 14)
  {
    _outerCircle = &radio_img_outercircle;
    _innerCircle = &radio_img_innercircle;
  }
  else
  {
    _outerCircle = &radio_img_outercircle_large;
    _innerCircle = &radio_img_innercircle_large;
  }

  if(_label.empty())
    _w = _buttonSize;
  else
    _w = _font.getStringWidth(_label) + _buttonSize + _font.getMaxCharWidth() * 0.75;
  alignBox(static_cast<int>(_buttonSize));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _editable && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    if(!_state)
      setState(true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonWidget::setState(bool state, bool send)
{
  if(_state != state)
  {
    _state = state;
    setDirty();
    if(_state && send)
      sendCommand(_cmd, _state, _id);
    if(state)
      myGroup->select(this);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonWidget::setFill(FillType type)
{
  switch(type)
  {
    case CheckboxWidget::FillType::Normal:
      _img = _buttonSize == 14 ? &radio_img_active : &radio_img_active_large;
      break;
    case CheckboxWidget::FillType::Inactive:
      _img = _buttonSize == 14 ? &radio_img_inactive: &radio_img_inactive_large;
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Draw the outer bounding circle
  s.drawIcon(*_outerCircle, _x, _y + _boxY,
             hilite ? kWidColorHi : kColor);

  // Draw the inner bounding circle with enabled color
  s.drawIcon(*_innerCircle, _x + 1, _y + _boxY + 1,
             isEnabled() ? _bgcolor : kColor);

  // draw state
  if(_state)
    s.drawIcon(*_img, _x + 2, _y + _boxY + 2,
               isEnabled() ? hilite ? kWidColorHi : kCheckColor : kColor);

  // Finally draw the label
  s.drawString(_font, _label, _x + _buttonSize + _font.getMaxCharWidth() * 0.75, _y + firstTextY(), _w,
               isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonGroup::addWidget(RadioButtonWidget* widget)
{
  myWidgets.push_back(widget);
  // set first button as default
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
  widget->setState(myWidgets.size() == 1, false);
  mySelected = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonGroup::select(const RadioButtonWidget* widget)
{
  uInt32 i = 0;

  for(const auto& w : myWidgets)
  {
    if(w == widget)
    {
      setSelected(i);
      break;
    }
    ++i;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RadioButtonGroup::setSelected(uInt32 selected)
{
  uInt32 i = 0;

  mySelected = selected;
  for(const auto& w : myWidgets)
  {
    static_cast<RadioButtonWidget*>(w)->setState(i == mySelected);
    ++i;
  }
}
