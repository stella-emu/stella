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

#ifndef AUDIO_WIDGET_HXX
#define AUDIO_WIDGET_HXX

class GuiObject;
class DataGridWidget;

#include "Widget.hxx"
#include "Command.hxx"

class AudioWidget : public Widget, public CommandSender
{
  public:
    AudioWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont);
    ~AudioWidget() override = default;

    void loadConfig() override;

    // Reflow entry point for the resizable debugger: move/resize the widget and
    // lay the registers/labels out for the available width (recomputes _h)
    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how tall I am -- that is however tall
    // my register rows make me -- so report what my own layout tree comes to
    Common::Size naturalSize() const override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Build the layout tree from the current font and position/size the
    // registers within the current width; shared by the ctor and setArea()
    void reflow();

    // The register rows as the engine sees them, built without positioning
    // anything, so reflow() and naturalSize() are one layout asked two questions
    unique_ptr<GUI::Layout> buildLayout() const;

  private:
    // ID's for the various widgets
    // We need ID's, since there are more than one of several types of widgets
    enum: uInt8 {
      kAUDFID,
      kAUDCID,
      kAUDVID
    };

    DataGridWidget*   myAudF{nullptr};
    StaticTextWidget* myAud0F{nullptr};
    StaticTextWidget* myAud1F{nullptr};
    DataGridWidget*   myAudC{nullptr};
    DataGridWidget*   myAudV{nullptr};
    StaticTextWidget* myAudEffV{nullptr};

    // Labels promoted from anonymous locals so reflow() can reposition them
    std::array<StaticTextWidget*, 3> myRegLabels{nullptr};      // AUDF/AUDC/AUDV
    std::array<StaticTextWidget*, 2> myChannelLabels{nullptr};  // channel 0/1
    StaticTextWidget* mySlash{nullptr};                         // between freqs

    // Audio channels
    enum: uInt8
    {
      kAud0Addr,
      kAud1Addr
    };

  private:
    void changeFrequencyRegs();
    void changeControlRegs();
    void changeVolumeRegs();
    void handleFrequencies();
    void handleVolume();
    uInt32 getEffectiveVolume();

    // Following constructors and assignment operators not supported
    AudioWidget() = delete;
    AudioWidget(const AudioWidget&) = delete;
    AudioWidget(AudioWidget&&) = delete;
    AudioWidget& operator=(const AudioWidget&) = delete;
    AudioWidget& operator=(AudioWidget&&) = delete;
};

#endif  // AUDIO_WIDGET_HXX
