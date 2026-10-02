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

#ifndef MEM_VIEW_WIDGET_HXX
#define MEM_VIEW_WIDGET_HXX

class GuiObject;
class FBSurface;
class ScrollBarWidget;
class ScrollBarHWidget;
class ContextMenu;

#include "Widget.hxx"
#include "Command.hxx"
#include "MemViewParams.hxx"
#include "MemViewDataLayer.hxx"
#include "MemViewAccessLayer.hxx"
#include "MemViewMarkerLayer.hxx"

/**
  This is the widget class for representing one complete data field (either RAM or ROM)
  with all it's layers (data, read, write, program counter).

  @author Christian Hammers
*/
class MemViewWidget : public Widget, public CommandSender
{

  public:

    static constexpr std::string_view TEXT_UNSUPPORTED = "Unsupported ROM type";
    static constexpr int DEFAULT_BANK_SIZE = 4096;
    using ColorTab = std::array<uInt32, 256>;

    // Fixed floor for a non-zoomable region's window-minimum size
    static constexpr int RAM_MIN_ZOOM = 3;

    // A zoomable region's own window-minimum floor never goes below this
    // zoom, so the window doesn't shrink to a 1px-per-bit sliver either
    static constexpr int ROM_MIN_ZOOM = 2;

  public:

    /**
      Constructor

      @param boss               Parent GUI object
      @param font               Font to use
      @param bankSize           Size of one bank to represent (bytes)
      @param bankCount          Number of banks to represent (bytes)
      @param bankHeight         Initial bank height to use (in bytes) - may change later
      @param baseAddress        Base address of the data within the address range of the system
      @param isZoomable         True when this view should be zoomable (also will have scrollbars)
      @param readColorTab       The 256 entry table with all the heatmap colors for read access data
      @param writeColorTab      The 256 entry table with all the heatmap colors for write access data
      @param pcColorTab         The 256 entry table with all the heatmap colors for PC access data
      @param dataDefaultColor   Default color for data bytes (not faded)
      @param dataFadedColor     Color to use when byte fade is active
      @param typeText           Short description of memory type to be shown in tool tip
      @param mirrorAddrOffset   Offset for a mirrored address (if any, else 0)
    */
    MemViewWidget(GuiObject* boss, const GUI::Font& font,
                  uInt16 bankSize, uInt16 bankCount, uInt16 bankHeight,
                  uInt16 baseAddress,
                  bool isZoomable,
                  const MemViewWidget::ColorTab& readColorTab,
                  const MemViewWidget::ColorTab& writeColorTab,
                  const MemViewWidget::ColorTab& pcColorTab,
                  uInt32 dataDefaultColor, uInt32 dataFadedColor,
                  string_view typeText, int mirrorAddrOffset = 0);
    ~MemViewWidget() override = default;

    using Widget::setDirty;
    void setDirty(bool renderGui);

    /**
      Derived handle methods
    */
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    void handleMouseMoved(int x, int y) override;
    void handleMouseLeft() override;
    bool wantsFocus() const override { return true; }

    string getToolTip(const Common::Point& pos) const override;
    bool changedToolTip(const Common::Point& oldPos,
      const Common::Point& newPos) const override { return true; }

    /**
      Set or update the data of this view to be displayed

      @param data   A ByteSpan pointing to the data
    */
    void updateData(const ByteSpan& data);

    /**
      Sets the size and position of the expected access data counters within our whole data range.

      @param size     Number of access counters to take over
      @param offset   Offset inside our whole data range to put the new data to

      @returns true if valid
    */
    bool setAccessDataParams(uInt32 size, uInt32 offset);

    /**
      Set or update new access counter data for the heatmaps.

      @param readData       Pointer to the new read access data values to take over
      @param writeData      Pointer to the new write access data values to take over
      @param pcData         Pointer to the new program counter access data values to take over
      @param elapsedCycles  Number of CPU cycles elapsed since the last update
      @param elapsedFrames  Number of complete elapsed TV frames since last update.
    */
    void updateAccessData(Device::AccessCounter* readData, Device::AccessCounter* writeData,
                          Device::AccessCounter* pcData,
                          uInt32 elapsedCycles, int elapsedFrames);

    /**
      Does all the rest what updateAccessData() didn't
    */
    void updateRest();

    /**
      Set new layout parameters

      @param singleRow    Only one row of banks
      @param separators   Show separators between the banks or not
      @param bankHeight   Height of a bank in bytes
    */
    void setLayoutParameters(bool singleRow, bool separators, int bankHeight);

    /**
      Set new visual parameters

      @param showData     Enable/disable the visibility of the data bytes
      @param showPc       Enable/disable the visibility of the program counter heatmap
      @param showReads    Enable/disable the visibility of the data read heatmap
      @param showWrites   Enable/disable the visibility of the data write heatmap
      @param inverted     Select if the bytes should be shown inverted
      @param byteFade     Select if the bytes should fade from MSB to LSB
    */
    void setVisualParameters(bool showData, bool showPc, bool showReads, bool showWrites,
                             bool inverted, bool byteFade);

    /**
      Set a new decay rate for the fading heatmap values

      @param percentage   The value 0..100 (0 = stay forever, 100 = fade within one frame)
    */
    void setDecayRate(int percentage);

    /**
      Reset all heatmap values to 0
    */
    void clearHeatmaps();

    /**
      Read if only single row is possible with this bank size
    */
    bool lockedSingleRow() const;

    /**
      Returns if the widget is fully setup and functional after construction
    */
    bool isSetup() const { return myIsSetup; }

    /**
      Renders all layers to the backend.
    */
    void render();

    void loadConfig() override;
    void setArea(int x, int y, int w, int h) override;

    // Content-driven size for a non-zoomable region; irrelevant otherwise
    Common::Size naturalSize() const override;

    // The smallest area this region can render in without clipping, which the
    // owning dialog declares to the layout engine (see WidgetLayout).  Never
    // wider than maxWidth; a non-zoomable region ignores it
    Common::Size minSize(int maxWidth) const;

    // Ceiling naturalSize() shrinks to fit within, for a non-zoomable region
    void setMaxCandidateSize(int w, int h) { myMaxCandidateW = w; myMaxCandidateH = h; }

  protected:

    bool hasToolTip() const override { return isSetup() && !myMouseDragging; }

    /**
      Draws the content of the widget if internal states indicate a redraw is necessary.
      Does not render to the backend.
    */
    void drawWidget(bool hilite) override;

    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:

    static constexpr int BORDER = 1;

    bool myIsZoomable;
    // Decided before myParams is built, so the layers never size their
    // buffers for an unsupported geometry
    bool myIsSetup;
    static bool isDisplayable(uInt16 bankSize, uInt16 bankCount, uInt16 bankHeight);

    // Ceiling naturalSize() searches within for a non-zoomable region, in
    // logical UI pixels; set by the dialog via setMaxCandidateSize()
    int myMaxCandidateW{0};
    int myMaxCandidateH{0};

    MemViewParams myParams;
    MemViewDataLayer myDataLayer;
    MemViewAccessLayer myReadLayer;
    MemViewAccessLayer myWriteLayer;
    MemViewAccessLayer myPcLayer;
    MemViewMarkerLayer myPcMarker;
    MemViewMarkerLayer myMouseMarker;

    string myTypeText;
    int myMirrorAddrOffset;

    int myDataIsDirty{false};

    bool myMouseDragging{false};
    int myClickX{0};
    int myClickY{0};
    int myRightClickX{0};
    int myRightClickY{0};

    ScrollBarWidget* myVScrollBar{nullptr};
    ScrollBarHWidget* myHScrollBar{nullptr};
    unique_ptr<ContextMenu> myMenu;

    ByteArray myCurrentData;     // Holds the original data (not rearranged)

  private:

    /**
      Converts an external position within the whole GUI window to our internal
      surface position.
    */
    Common::Point extPosConv(const Common::Point& pos) const;

    /**
      Checks if an external position is within our shown data area.
    */
    bool extPosInData(const Common::Point& pos) const;

    /**
      Checks if an internal position is within our shown data area.
    */
    bool intPosInData(int x, int y) const;

    /**
      Mark only our data bytes to be dirty and needs to be redrawn.
    */
    void setDirtyData();

    /**
      Clear the normal dirty flag as well as our dirty data flag
    */
    void clearEverythingDirty();

    /**
      Update our byte marker frame
    */
    void updateMarker();

  #ifdef IMAGE_SUPPORT
    void savePicture();
  #endif

    /**
      Returns the resulting size in pixels for a specific memory view configuration
      and automatically determines minimum zoom level

      @param availableWidth    Total available width of area in pixel
      @param availableHeight   Total available height of area in pixels
      @param bankWidth         Width of one bank in bytes
      @param bankHeight        Height of one bank in bytes
      @param hBanks            Number of horizontally banks next to each other
      @param vBanks            Number of vertical stacked banks
      @param separators        Use separator lines inbetween the banks
      @param minZoomLevel      Reference to receive the minimum calculated zoom level

      @return Dimensions of suggested display size
    */
    static Common::Size calcSizeAndZoom(int availableWidth, int availableHeight,
                                        int bankWidth, int bankHeight,
                                        int hBanks, int vBanks,
                                        bool separators, int& minZoomLevel);

    /**
      Finds the best layout for displaying the data on the available screen space given
      the user's preference for bankHeight (and therefore also the bankWidth) and if
      to show everything in a single row of banks or not.
      The layout arrangement is basically how many banks are shown next to each other
      and how many on top of each other (like 4x2 or 8x1 etc.).
      The best solution is determined by the highest zoom level or the best matching
      aspect ratio of the data block to the available screen space.

      @param availableWidth Width of the area to fit into, in pixels
      @param availableHeight Height of the area to fit into, in pixels
      @param bankWidth      Width of one bank in bytes
      @param bankHeight     Height of one bank in bytes
      @param separators     Show separator lines inbetween the banks
      @param singleRow      Parameter to put in preference for single bank row
                            (But could be overwritten and returned differently)
      @param hBanks         Reference to get back the number of horizontal banks
      @param vBanks         Reference to get back the number of vertical banks
      @param minZoomLevel   Reference to get back the minimum zoom level

      @return Dimensions of determined display size
    */
    Common::Size findBestLayout(int availableWidth, int availableHeight,
                                int bankWidth, int bankHeight, bool separators,
                                bool& singleRow, int& hBanks, int& vBanks,
                                int& minZoomLevel) const;

    /**
      Copies the heatmap data of all layers to the display fields in one go.
      Note: This could have been done in the MemViewAccessLayer class for each layer
      separately but it's a little bit faster to only once go through the rearrangement loop.

      @param force    Forces to update the data regardless if they are currently visible or not
    */
    void heatmapsToFields(bool force = false);

    void recalcScrollBars();

    /**
      Change the zoom level for the displayed data
    */
    void zoom(int level);

    // (Re)allocate the layer surfaces at the widget's current size,
    // needed on every live resize
    void reallocateLayerSurfaces();

  private:
    // Following constructors and assignment operators not supported
    MemViewWidget() = delete;
    MemViewWidget(const MemViewWidget&) = delete;
    MemViewWidget(MemViewWidget&&) = delete;
    MemViewWidget& operator=(const MemViewWidget&) = delete;
    MemViewWidget& operator=(MemViewWidget&&) = delete;
};

#endif  // MEM_VIEW_WIDGET_HXX
