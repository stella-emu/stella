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

#ifndef OVERLAY_MENU_HXX
#define OVERLAY_MENU_HXX

class OSystem;
class Dialog;

#include "DialogContainer.hxx"

/**
  A generic DialogContainer for opening any single dialog over TIA mode,
  without requiring a dedicated subclass per dialog type.

  @author  Stephen Anthony
*/
class OverlayMenu : public DialogContainer
{
  public:
    explicit OverlayMenu(OSystem& osystem) : DialogContainer(osystem) { }
    ~OverlayMenu() override { delete myBaseDialog; }

    // Takes ownership of the dialog; deletes any previously held dialog
    void setDialog(Dialog* dialog) {
      delete myBaseDialog;
      myBaseDialog = dialog;
    }

    Dialog* baseDialog() override { return myBaseDialog; }

  private:
    Dialog* myBaseDialog{nullptr};

  private:
    // Following constructors and assignment operators not supported
    OverlayMenu() = delete;
    OverlayMenu(const OverlayMenu&) = delete;
    OverlayMenu(OverlayMenu&&) = delete;
    OverlayMenu& operator=(const OverlayMenu&) = delete;
    OverlayMenu& operator=(OverlayMenu&&) = delete;
};

#endif  // OVERLAY_MENU_HXX
