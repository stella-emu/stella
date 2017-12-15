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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef REWIND_DIALOG_HXX
#define REWIND_DIALOG_HXX

class CommandSender;
class DialogContainer;
class OSystem;

#include "Dialog.hxx"

class RewindDialog : public Dialog
{
  public:
    RewindDialog(OSystem& osystem, DialogContainer& parent, int max_w, int max_h);
    virtual ~RewindDialog() = default;

  private:
    // Following constructors and assignment operators not supported
    RewindDialog() = delete;
    RewindDialog(const RewindDialog&) = delete;
    RewindDialog(RewindDialog&&) = delete;
    RewindDialog& operator=(const RewindDialog&) = delete;
    RewindDialog& operator=(RewindDialog&&) = delete;
};

#endif
