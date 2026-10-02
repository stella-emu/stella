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

#ifndef MEMVIEW_ACCESS_LAYER_HXX
#define MEMVIEW_ACCESS_LAYER_HXX

#include "MemViewLayer.hxx"
#include "Device.hxx"

/**
  This is the specific class for representing a visible layer for holding and
  drawing access counter (aka heatmap) data for the Memory View functionality.

  @author Christian Hammers
*/

class MemViewAccessLayer : public MemViewLayer
{

  friend class MemViewWidget;

  public:

    using ColorTab = std::array<uInt32, 256>;
    using HeatmapValue = float;
    static constexpr HeatmapValue BASE_ACCESS_VALUE = 128.0;

  public:

    /**
      Constructor

      @param dialog       Boss dialog we are living in
      @param params       Reference to the parameters common for everything within this view
      @param colorTab     A table with 256 colors to be used for this heatmap display
    */
    MemViewAccessLayer(Dialog& dialog, MemViewParams &params, const ColorTab& colorTab);
    ~MemViewAccessLayer() override;

    /**
      Draws content if layer is visible.
    */
    void draw() override;

    /**
      Renders content if layer is visible.
    */
    void render() override;

    /**
      Updates this layer with new access counter values coming from the emulation system.

      @param accessData         Pointer to the new access data values to take over
      @param currentDecrement   The fade away value for an active heatmap cell calculated
                                based on the user's setting and the elapsed CPU cycles since
                                last update.
      @param elapsedFrames      Number of complete elapsed TV frames since last update.
    */
    void updateAccessData(Device::AccessCounter* accessData,
      const MemViewAccessLayer::HeatmapValue& currentDecrement,
      int elapsedFrames
    );

    /**
      Resets the heatmap to show everything black (or transparent) again
    */
    void clearHeatmap();

    /**
      Load the total access counters since ROM start to be shown in heatmap.
    */
    void loadTotals(bool skipNextUpdate);

    /**
      Get the total access value since system start at the specified original offset

      @param offset   Offset position
      @return The access counter
    */
    Device::AccessCounter getTotalValue(unsigned int offset) const;

    /**
      Get the delta access value since last time cleared at the specified original offset

      @param offset   Offset position
      @return The access counter delta
    */
    Device::AccessCounter getDeltaValue(unsigned int offset) const;

  private:

    const ColorTab& myColorTab;  // ARGB
    std::vector<Device::AccessCounter> myLastAccessData;
    std::vector<Device::AccessCounter> myStartAccessData;
    std::vector<HeatmapValue> myHeatmap;
    double myHeatmapGain{1.0};
    bool myFirstRun{true};
    bool mySkipNextUpdate{false};

  private:

    void compareAccessData(const Device::AccessCounter* newData,
      const MemViewAccessLayer::HeatmapValue& currentDecrement,
      int elapsedFrames
    );

  private:
    // Following constructors and assignment operators not supported
    MemViewAccessLayer() = delete;
    MemViewAccessLayer(const MemViewAccessLayer&) = delete;
    MemViewAccessLayer(MemViewAccessLayer&&) = delete;
    MemViewAccessLayer& operator=(const MemViewAccessLayer&) = delete;
    MemViewAccessLayer& operator=(MemViewAccessLayer&&) = delete;

};

#endif  // MEMVIEW_ACCESS_LAYER_HXX
