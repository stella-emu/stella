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

#ifndef WHATS_NEW_DIALOG_HXX
#define WHATS_NEW_DIALOG_HXX

#include "Dialog.hxx"

class StaticTextWidget;

class WhatsNewDialog : public Dialog
{
  public:
    WhatsNewDialog(OSystem& osystem, DialogContainer& parent);
    ~WhatsNewDialog() override = default;

  protected:
    void layout() override;

  private:
    void add(string_view text);

  private:
    std::vector<StaticTextWidget*> myLines;
    std::vector<int> myLineAdvance;

  private:
    // Following constructors and assignment operators not supported
    WhatsNewDialog(const WhatsNewDialog&) = delete;
    WhatsNewDialog(WhatsNewDialog&&) = delete;
    WhatsNewDialog& operator=(const WhatsNewDialog&) = delete;
    WhatsNewDialog& operator=(WhatsNewDialog&&) = delete;
};

#endif  // WHATS_NEW_DIALOG_HXX
