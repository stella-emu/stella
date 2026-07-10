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

#ifndef TIA_WINDOW_DIALOG_HXX
#define TIA_WINDOW_DIALOG_HXX

class OSystem;
class DialogContainer;
class TiaDisplayWidget;

#include "Dialog.hxx"

/**
  The base dialog of the debugger's companion TIA window.  It hosts a
  TiaDisplayWidget (a scalable, zoom/pan-able palette+phosphor view of the TIA
  image); additional debugger controls will be added here later.  As a Dialog
  it is a CommandSender/receiver, so those controls can drive the debugger.

  @author  Stephen Anthony
*/
class TiaWindowDialog : public Dialog
{
  public:
    TiaWindowDialog(OSystem& osystem, DialogContainer& parent,
                    int x, int y, int w, int h);
    ~TiaWindowDialog() override = default;

    void loadConfig() override;

  protected:
    void layout() override;

  private:
    TiaDisplayWidget* myTiaDisplay{nullptr};

  private:
    // Following constructors and assignment operators not supported
    TiaWindowDialog() = delete;
    TiaWindowDialog(const TiaWindowDialog&) = delete;
    TiaWindowDialog(TiaWindowDialog&&) = delete;
    TiaWindowDialog& operator=(const TiaWindowDialog&) = delete;
    TiaWindowDialog& operator=(TiaWindowDialog&&) = delete;
};

#endif  // TIA_WINDOW_DIALOG_HXX
