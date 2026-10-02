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

#ifndef MEMVIEW_LAYER_HXX
#define MEMVIEW_LAYER_HXX

class FBSurface;
class MemViewParams;
class Dialog;

/**
  This represents the base class for a stacked visible layer of some data that is used
  in the MemViewWidget to display the different data and access counter (heatmaps)
  of the Memory View functionality.

  @author Christian Hammers
*/

class MemViewLayer
{

  public:

    /**
      Constructor

      @param dialog       Boss dialog we are living in
      @param params       Reference to the parameters common for everything within this view
      @param isDataLayer  Indicator for being used as a data layer (showing the single bits
                          of the memory data) or heatmap data (false, painting whole bytes)
    */
    MemViewLayer(Dialog& dialog, MemViewParams &params, bool isDataLayer);
    virtual ~MemViewLayer();

    /**
      Enables/disables the visibility of this layer based on what the user selects what he
      whishes to see.

      @param visible      True when being shown, false to omit
    */
    virtual void setVisibility(bool visible) { myVisibility = visible; }

    /**
      Draws content if layer is visible.
    */
    virtual void draw() = 0;

    /**
      Renders content if layer is visible.
    */
    virtual void render() = 0;

  public:

    Dialog& myDialog;
    MemViewParams& myParams;
    bool myIsDataLayer;
    shared_ptr<FBSurface> mySurface;
    bool myVisibility{false};

    // A field is either one bit (if myIsDataLayer == true) or one byte (myIsDataLayer == false)
    // This holds the calculated ARGB values to be shown
    uIntArray myFields;

  protected:

    /**
      Allocate/deallocate surface at backend
    */
    void allocateSurface();
    void deallocateSurface();

    bool beginDraw(uInt32*& destAddr, uInt32& pitchWords);
    void endDraw();

    static uInt32* drawSeparatorLine(const MemViewParams& params, uInt32*& lineStart,
      const uInt32& backgroundColor, const uInt32& separatorColor);

    static inline bool appendLine(const uInt32* src, uInt32*& dest, const uInt32*& destStop,
      const int& firstPixelReps, const int& defaultPixelReps, int unitCount);

    void drawImpl(
      const MemViewParams& params,
      const uIntArray& srcData,
      uInt32 *destAddr,
      int pitchWords,
      uInt32 separatorColor = 0x00000000,
      uInt32 backgroundColor = 0x00000000) const;

    void renderImpl();

  private:
    // Following constructors and assignment operators not supported
    MemViewLayer() = delete;
    MemViewLayer(const MemViewLayer&) = delete;
    MemViewLayer(MemViewLayer&&) = delete;
    MemViewLayer& operator=(const MemViewLayer&) = delete;
    MemViewLayer& operator=(MemViewLayer&&) = delete;
};

#endif  // MEMVIEW_LAYER_HXX
