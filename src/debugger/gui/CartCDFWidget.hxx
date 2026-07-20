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

#ifndef CARTRIDGE_CDF_WIDGET_HXX
#define CARTRIDGE_CDF_WIDGET_HXX

class PopUpWidget;
class CheckboxWidget;
class DataGridWidget;
class StaticTextWidget;
class SliderWidget;

#include "CartCDF.hxx"
#include "CartARMWidget.hxx"

class CartridgeCDFWidget : public CartridgeARMWidget
{
  public:
    CartridgeCDFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       CartridgeCDF& cart);
    ~CartridgeCDFWidget() override = default;

    void saveOldState() override;
    void loadConfig() override;
    string bankState() override;

    // Start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    // End of functions for Cartridge RAM tab

  protected:
    void layoutContent(GUI::BoxLayout& col) const override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // The datastream table: the pointer and increment grids side by side, with the
    // stream numbers (and the named streams below them) labelling their rows
    unique_ptr<GUI::Layout> layoutDatastreams() const;

  private:
    struct CartState {
      ByteArray fastfetchoffset;
      IntArray datastreampointers;
      IntArray datastreamincrements;
      IntArray addressmaps;
      IntArray mcounters;
      IntArray mfreqs;
      IntArray mwaves;
      IntArray mwavesizes;
      IntArray samplepointer;
      uInt32 random{0};
      ByteArray internalram;
    };

    CartridgeCDF& myCart;
    PopUpWidget* myBank{nullptr};

    StaticTextWidget *myPointersLabel{nullptr}, *myIncrementsLabel{nullptr},
                     *myFastFetchOffsetLabel{nullptr}, *myMusicLabel{nullptr},
                     *myCountersLabel{nullptr}, *myFrequenciesLabel{nullptr},
                     *myWaveformsLabel{nullptr}, *myWaveformSizesLabel{nullptr},
                     *mySamplePointerLabel{nullptr};

    DataGridWidget* myDatastreamPointers{nullptr};
    DataGridWidget* myDatastreamIncrements{nullptr};
    DataGridWidget* myCommandStreamPointer{nullptr};
    DataGridWidget* myCommandStreamIncrement{nullptr};
    DataGridWidget* myJumpStreamPointers{nullptr};
    DataGridWidget* myJumpStreamIncrements{nullptr};
    DataGridWidget* myMusicCounters{nullptr};
    DataGridWidget* myMusicFrequencies{nullptr};
    DataGridWidget* myMusicWaveforms{nullptr};
    DataGridWidget* myMusicWaveformSizes{nullptr};
    DataGridWidget* mySamplePointer{nullptr};
    DataGridWidget* myFastFetcherOffset{nullptr};
    std::array<StaticTextWidget*, 10> myDatastreamLabels{nullptr};

    CheckboxWidget* myFastFetch{nullptr};
    CheckboxWidget* myDigitalSample{nullptr};

    CartState myOldState;

    enum { kBankChanged = 'bkCH' };

  private:
    bool isCDFJ() const;
    bool isCDFJplus() const;

    static constexpr string_view describeCDFVersion(
        CartridgeCDF::CDFSubtype subtype) {
      switch(subtype)
      {
        using enum CartridgeCDF::CDFSubtype;
        case CDF0:      return "CDF (v0)";
        case CDF1:      return "CDF (v1)";
        case CDFJ:      return "CDFJ";
        case CDFJplus:  return "CDFJ+";
        default:        throw std::runtime_error("unreachable");
      }
    }

  private:
    // Following constructors and assignment operators not supported
    CartridgeCDFWidget() = delete;
    CartridgeCDFWidget(const CartridgeCDFWidget&) = delete;
    CartridgeCDFWidget(CartridgeCDFWidget&&) = delete;
    CartridgeCDFWidget& operator=(const CartridgeCDFWidget&) = delete;
    CartridgeCDFWidget& operator=(CartridgeCDFWidget&&) = delete;
};

#endif  // CARTRIDGE_CDF_WIDGET_HXX
