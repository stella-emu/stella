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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef RIOT_RAM_WIDGET_HXX
#define RIOT_RAM_WIDGET_HXX

class GuiObject;
class InputTextDialog;
class ButtonWidget;
class DataGridWidget;
class DataGridOpsWidget;
class EditTextWidget;
class StaticTextWidget;

#include "CartDebug.hxx"
#include "RamWidget.hxx"

class RiotRamWidget : public RamWidget
{
  public:
    RiotRamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                  int x, int y, int w);
    virtual ~RiotRamWidget();

  private:
    uInt8 getValue(int addr) const;
    void setValue(int addr, uInt8 value);
    string getLabel(int addr) const;

    void fillList(uInt32 start, uInt32 size, IntArray& alist,
                  IntArray& vlist, BoolArray& changed) const;
    uInt32 readPort(uInt32 start) const;
    const ByteArray& currentRam(uInt32 start) const;

  private:
    CartDebug& myDbg;

  private:
    // Following constructors and assignment operators not supported
    RiotRamWidget() = delete;
    RiotRamWidget(const RiotRamWidget&) = delete;
    RiotRamWidget(RiotRamWidget&&) = delete;
    RiotRamWidget& operator=(const RiotRamWidget&) = delete;
    RiotRamWidget& operator=(RiotRamWidget&&) = delete;
};

#endif
