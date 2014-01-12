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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef ROM_WIDGET_HXX
#define ROM_WIDGET_HXX

class GuiObject;
class EditTextWidget;

#include "Base.hxx"
#include "Command.hxx"
#include "RomListWidget.hxx"

class RomWidget : public Widget, public CommandSender
{
  public:
    // This enum needs to be seen outside the class
    enum {
      kInvalidateListing  = 'INli'
    };

  public:
    RomWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
              int x, int y, int w, int h);
    virtual ~RomWidget();

    void invalidate(bool forcereload = true)
    { myListIsDirty = true; if(forcereload) loadConfig(); }

    void scrollTo(int line) { myRomList->setSelected(line); }
    void handleCommand(CommandSender* sender, int cmd, int data, int id);
    void loadConfig();

  private:
    void setBreak(int disasm_line, bool state);
    void setPC(int disasm_line);
    void runtoPC(int disasm_line);
    void patchROM(int disasm_line, const string& bytes,
                  Common::Base::Format base);

  private:
    RomListWidget*   myRomList;
    EditTextWidget*  myBank;

    bool myListIsDirty;
};

#endif
