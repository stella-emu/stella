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

#ifndef CARTRIDGECDF_WIDGET_HXX
#define CARTRIDGECDF_WIDGET_HXX

class CartridgeCDF;
class PopUpWidget;
class CheckboxWidget;
class DataGridWidget;
class StaticTextWidget;

#include "CartDebugWidget.hxx"

class CartridgeCDFWidget : public CartDebugWidget
{
  public:
    CartridgeCDFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeCDF& cart);
    virtual ~CartridgeCDFWidget() = default;

  private:
    struct CartState {
      ByteArray tops;
      ByteArray bottoms;
      IntArray datastreampointers;
      IntArray datastreamincrements;
      IntArray addressmaps;
      IntArray mcounters;
      IntArray mfreqs;
      IntArray mwaves;
      IntArray mwavesizes;
      IntArray samplepointer;
      uInt32 random;
      ByteArray internalram;
    };

    CartridgeCDF& myCart;
    PopUpWidget* myBank;

    DataGridWidget* myDatastreamPointers;
    DataGridWidget* myDatastreamIncrements;
    DataGridWidget* myDatastreamPointers2;
    DataGridWidget* myDatastreamIncrements2;
    DataGridWidget* myMusicCounters;
    DataGridWidget* myMusicFrequencies;
    DataGridWidget* myMusicWaveforms;
    DataGridWidget* myMusicWaveformSizes;
    DataGridWidget* mySamplePointer;
    StaticTextWidget* myDatastreamLabels[10];

    CheckboxWidget* myFastFetch;
    CheckboxWidget* myDigitalSample;
    CartState myOldState;

    enum { kBankChanged = 'bkCH' };

  private:
    void saveOldState() override;

    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    string bankState() override;

    // start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    // end of functions for Cartridge RAM tab

    // Following constructors and assignment operators not supported
    CartridgeCDFWidget() = delete;
    CartridgeCDFWidget(const CartridgeCDFWidget&) = delete;
    CartridgeCDFWidget(CartridgeCDFWidget&&) = delete;
    CartridgeCDFWidget& operator=(const CartridgeCDFWidget&) = delete;
    CartridgeCDFWidget& operator=(CartridgeCDFWidget&&) = delete;
};

#endif
