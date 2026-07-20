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

#ifndef ROM_WIDGET_HXX
#define ROM_WIDGET_HXX

class GuiObject;
class EditTextWidget;
class RomListWidget;

#include "Base.hxx"
#include "Command.hxx"
#include "Widget.hxx"

class RomWidget : public Widget, public CommandSender
{
  public:
    // This enum needs to be seen outside the class
    enum {
      kInvalidateListing  = 'INli'
    };

  public:
    RomWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont);
    ~RomWidget() override = default;

    void loadConfig() override;

    void invalidate(bool forcereload = true)
    { myListIsDirty = true; if(forcereload) loadConfig(); }

    void scrollTo(int line);

    void setArea(int x, int y, int w, int h) override;

    // I fill whatever area I am given, so I have no height of my own to
    // constrain the window with (see the note in TabWidget::naturalSize)
    Common::Size naturalSize() const override { return {}; }

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Position the bank display and the disassembly listing within the area
    // this widget occupies; shared by the ctor and setArea()
    void reflow();

    void toggleBreak(int disasm_line);
    void setPC(int disasm_line);
    void runtoPC(int disasm_line);
    void setTimer(int disasm_line);
    void disassemble(int disasm_line);
    void patchROM(int disasm_line, string_view bytes, Common::Base::Fmt base);
    uInt16 getAddress(int disasm_line);

  private:
    RomListWidget*   myRomList{nullptr};
    EditTextWidget*  myBank{nullptr};
    StaticTextWidget* myInfoLabel{nullptr};

    bool myListIsDirty{true};

  private:
    // Following constructors and assignment operators not supported
    RomWidget() = delete;
    RomWidget(const RomWidget&) = delete;
    RomWidget(RomWidget&&) = delete;
    RomWidget& operator=(const RomWidget&) = delete;
    RomWidget& operator=(RomWidget&&) = delete;
};

#endif  // ROM_WIDGET_HXX
