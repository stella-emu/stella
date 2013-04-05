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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CART_DEBUG_WIDGET_HXX
#define CART_DEBUG_WIDGET_HXX

class GuiObject;
class ButtonWidget;

#include "Font.hxx"
#include "Command.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "StringListWidget.hxx"
#include "StringParser.hxx"

class CartDebugWidget : public Widget, public CommandSender
{
  public:
    CartDebugWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int w, int h)
      : Widget(boss, font, x, y, w, h),
        CommandSender(boss),
        myFontWidth(font.getMaxCharWidth()),
        myFontHeight(font.getFontHeight()),
        myLineHeight(font.getLineHeight())
    {
      _type = kCartDebugWidget;
    }

    virtual ~CartDebugWidget() { };

  public:
    int addBaseInformation(int bytes, const string& manufacturer,
        const string& desc)
    {
      const int lwidth = _font.getStringWidth("Manufacturer: "),
                fwidth = _w - lwidth - 30;
      EditTextWidget* w = 0;
      StringListWidget* sw = 0;
      ostringstream buf;

      int x = 10, y = 10;

      // Add ROM size, manufacturer and bankswitch info
      new StaticTextWidget(_boss, _font, x, y, lwidth,
            myFontHeight, "ROM Size: ", kTextAlignLeft);
      buf << bytes << " bytes";
      if(bytes >= 1024)
        buf << " / " << (bytes/1024) << "KB";

      w = new EditTextWidget(_boss, _font, x+lwidth, y,
            fwidth, myFontHeight, buf.str());
      w->setEditable(false);
      y += myLineHeight + 4;

      new StaticTextWidget(_boss, _font, x, y, lwidth,
            myFontHeight, "Manufacturer: ", kTextAlignLeft);
      w = new EditTextWidget(_boss, _font, x+lwidth, y,
            fwidth, myFontHeight, manufacturer);
      w->setEditable(false);
      y += myLineHeight + 4;

      StringParser bs(desc);
      const StringList& sl = bs.stringList();
      uInt32 lines = sl.size();
      if(lines < 3) lines = 3;
      if(lines > 6) lines = 6;

      new StaticTextWidget(_boss, _font, x, y, lwidth,
            myFontHeight, "Description: ", kTextAlignLeft);
      sw = new StringListWidget(_boss, _font, x+lwidth, y,
            fwidth, lines * myLineHeight, false);
      sw->setEditable(false);
      sw->setList(sl);
      y += sw->getHeight() + 4;

      return y;
    }

    virtual void loadConfig() { };
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id) { };

  protected:
    // These will be needed by most of the child classes;
    // we may as well make them protected variables
    int myFontWidth, myFontHeight, myLineHeight;
};

#endif
