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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef INPUT_MENU_HXX
#define INPUT_MENU_HXX

class OSystem;
class InputTextDialog;

#include "DialogContainer.hxx"

/**
  The base dialog for all input menus in Stella.

  @author  Thomas Jentzsch
*/
class InputMenu : public DialogContainer
{
  public:
    /**
      Create a new menu stack
    */
    explicit InputMenu(OSystem& osystem);
    ~InputMenu() override;

    void setTitle(const string& title, bool yesNo = false);
    void setLabels(const StringList& text, bool yesNo = false);
    //bool confirmed();

  private:
    /**
      Return (and possibly create) the bottom-most dialog of this container.
    */
    Dialog* baseDialog() override;

  private:
    InputTextDialog* myInputTextDialog{nullptr};

  private:
    // Following constructors and assignment operators not supported
    InputMenu() = delete;
    InputMenu(const InputMenu&) = delete;
    InputMenu(InputMenu&&) = delete;
    InputMenu& operator=(const InputMenu&) = delete;
    InputMenu& operator=(InputMenu&&) = delete;
};

#endif
