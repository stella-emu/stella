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

#ifndef MEMVIEW_DATA_MARKER_HXX
#define MEMVIEW_DATA_MARKER_HXX

class MemViewParams;
class Dialog;
class FBSurface;

class MemViewMarkerLayer
{

  public:

    static constexpr int MARKER_THICKNESS = 1;
    static constexpr int BUFFER_WIDTH = MemViewParams::myMaxZoom * 8 + 2 * MARKER_THICKNESS;
    static constexpr int BUFFER_HEIGHT = MemViewParams::myMaxZoom + 2 * MARKER_THICKNESS;

  public:

    /**
      Constructor

      @param dialog   The dialog we are living in
      @param params   Reference to the MemViewParams object
      @param color    The RGB color to use for the marker
    */
    MemViewMarkerLayer(Dialog& dialog, MemViewParams &params, uInt32 color);
    ~MemViewMarkerLayer();

    /**
      Enable/disable the marker and set the byte to mark (when enabled)

      @param enable     On or off
      @param byteOffset Offset of the byte to mark within our rearranged data
      @return true if something has changed
    */
    bool set(bool enable, int byteOffset = 0);

    /**
      Update the marker if something has moved but the byte to be marked is
      still the same.

      @return true if something has changed
    */
    bool update() { return set(myEnabled, myLastOffset); }

    /*
      Render the marker
    */
    void render();

  private:
    /**
      Allocate/deallocate surface at backend
    */
    void allocateSurface();
    void deallocateSurface();

  private:
    Dialog& myDialog;
    MemViewParams& myParams;

    Common::Point mySourcePos;
    Common::Point myDestPos;
    Common::Size mySize;
    int myDrawnZoomLevel{0};
    const uInt32 myColor;
    bool myEnabled{false};
    bool myVisibility{false};
    int myLastOffset{0};

    shared_ptr<FBSurface> mySurface;
    std::array<uInt32, SZT(BUFFER_WIDTH) * BUFFER_HEIGHT> myDummyBuffer{};

  private:
    // Following constructors and assignment operators not supported
    MemViewMarkerLayer(const MemViewMarkerLayer&) = delete;
    MemViewMarkerLayer(MemViewMarkerLayer&&) = delete;
    MemViewMarkerLayer& operator=(const MemViewMarkerLayer&) = delete;
    MemViewMarkerLayer& operator=(MemViewMarkerLayer&&) = delete;
};

#endif  // MEMVIEW_DATA_MARKER_HXX
