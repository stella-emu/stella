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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Serializer.hxx"
#include "Serializable.hxx"
#include "System.hxx"
#include "CartMVC.hxx"

/**
  Implementation of MovieCart.
  1K of memory is presented on the bus, but is repeated to fill the 4K image space.
  Contents are dynamically altered with streaming image and audio content as specific
  128-byte regions are entered.
  Original implementation: github.com/lodefmode/moviecart

  @author  Rob Bairos
*/

#define LO_JUMP_BYTE(X) ((X) & 0xff)
#define HI_JUMP_BYTE(X) ((((X) & 0xff00) >> 8) | 0x10)

#define COLOR_BLUE      0x9A
// #define COLOR_WHITE     0x0E

#define OSD_FRAMES      180
#define BACK_SECONDS    10

#define TITLE_CYCLES    1000000

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
  Simulate retrieval 512 byte chunks from a serial source
*/
class StreamReader : public Serializable
{
  public:
    StreamReader() = default;

    bool open(const string& path) {
      myFile = Serializer(path, Serializer::Mode::ReadOnly);
      if(myFile)
        myFileSize = myFile.size();

      return bool(myFile);
    }

    void blankPartialLines(bool index) {
      constexpr int colorSize = 192 * 5;
      if (index)
      {
        // top line
        myColor[0] = 0;
        myColor[1] = 0;
        myColor[2] = 0;
        myColor[3] = 0;
        myColor[4] = 0;
      }
      else
      {
        // bottom line
        myColor[colorSize - 5] = 0;
        myColor[colorSize - 4] = 0;
        myColor[colorSize - 3] = 0;
        myColor[colorSize - 2] = 0;
        myColor[colorSize - 1] = 0;
      }

      myColorBK[0] = 0;
    }

    void swapField(bool index, bool odd) {
      uInt8* offset = index ? myBuffer1 : myBuffer2;

      myVersion  = offset + VERSION_DATA_OFFSET;
      myFrame    = offset + FRAME_DATA_OFFSET;
      myAudio    = offset + AUDIO_DATA_OFFSET;
      myGraph    = offset + GRAPH_DATA_OFFSET;
      myTimecode = offset + TIMECODE_DATA_OFFSET;
      myColor    = offset + COLOR_DATA_OFFSET;
      myColorBK  = offset + COLORBK_DATA_OFFSET;

      if (!odd)
          myColorBK++;
    }

    bool readField(uInt32 fnum, bool index) {
      if(myFile)
      {
        size_t offset = ((fnum + 0) * CartridgeMVC::MVC_FIELD_PAD_SIZE);

        if(offset + CartridgeMVC::MVC_FIELD_PAD_SIZE < myFileSize)
        {
          myFile.setPosition(offset);
          if(index)
            myFile.getByteArray(myBuffer1, CartridgeMVC::MVC_FIELD_SIZE);
          else
            myFile.getByteArray(myBuffer2, CartridgeMVC::MVC_FIELD_SIZE);

          return true;
        }
      }
      return false;
    }

    uInt8 readVersion() { return *myVersion++; }
    uInt8 readFrame()   { return *myFrame++;   }
    uInt8 readColor() { return *myColor++; }
    uInt8 readColorBK() { return *myColorBK++;   }

    uInt8 readGraph() {
      return myGraphOverride ? *myGraphOverride++ : *myGraph++;
    }

    void overrideGraph(const uInt8* p) { myGraphOverride = p; }

    uInt8 readAudio() { return *myAudio++; }

    uInt8 peekAudio() const { return *myAudio; }

    void startTimeCode() { myGraph = myTimecode; }

    bool save(Serializer& out) const override {
      try
      {
        out.putByteArray(myBuffer1, CartridgeMVC::MVC_FIELD_SIZE);
        out.putByteArray(myBuffer2, CartridgeMVC::MVC_FIELD_SIZE);

      #if 0  // FIXME - determine whether we need to load/save this
        const uInt8*  myAudio
        const uInt8*  myGraph
        const uInt8*  myGraphOverride
        const uInt8*  myTimecode
        const uInt8*  myColor
        const uInt8*  myColorBK
        const uInt8*  myVersion
        const uInt8*  myFrame
      #endif
      }
      catch(...)
      {
        return false;
      }
      return true;
    }

    bool load(Serializer& in) override {
      try
      {
        in.getByteArray(myBuffer1, CartridgeMVC::MVC_FIELD_SIZE);
        in.getByteArray(myBuffer2, CartridgeMVC::MVC_FIELD_SIZE);

      #if 0  // FIXME - determine whether we need to load/save this
        const uInt8*  myAudio
        const uInt8*  myGraph
        const uInt8*  myGraphOverride
        const uInt8*  myTimecode
        const uInt8*  myColor
        const uInt8*  myColorBK
        const uInt8*  myVersion
        const uInt8*  myFrame
      #endif
      }
      catch(...)
      {
        return false;
      }
      return true;
    }

  private:
    static constexpr int
        VERSION_DATA_OFFSET = 0,
        FRAME_DATA_OFFSET = 4,
        AUDIO_DATA_OFFSET = 7,
        GRAPH_DATA_OFFSET = 269,
        TIMECODE_DATA_OFFSET = 1229,
        COLOR_DATA_OFFSET = 1289,
        COLORBK_DATA_OFFSET = 2249,
        END_DATA_OFFSET = 2441;

    const uInt8*  myAudio{nullptr};

    const uInt8*  myGraph{nullptr};
    const uInt8*  myGraphOverride{nullptr};

    const uInt8*  myTimecode{nullptr};
    uInt8*        myColor{nullptr};
    uInt8*        myColorBK{nullptr};
    const uInt8*  myVersion{nullptr};
    const uInt8*  myFrame{nullptr};

    uInt8 myBuffer1[CartridgeMVC::MVC_FIELD_SIZE];
    uInt8 myBuffer2[CartridgeMVC::MVC_FIELD_SIZE];

    Serializer myFile;
    size_t myFileSize{0};
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
  State of current switches and joystick positions to control MovieCart
*/
class MovieInputs : public Serializable
{
  public:
    MovieInputs() = default;

    void init() {
      bw = fire = select = reset = false;
      right = left = up = down = false;
    }

    bool bw{false}, fire{false}, select{false}, reset{false};
    bool right{false}, left{false}, up{false}, down{false};

    void updateDirection(uInt8 val) {
      right = val & TRANSPORT_RIGHT;
      left  = val & TRANSPORT_LEFT;
      up    = val & TRANSPORT_UP;
      down  = val & TRANSPORT_DOWN;
    }

    void updateTransport(uInt8 val) {
      bw     = val & TRANSPORT_BW;
      fire   = val & TRANSPORT_BUTTON;
      select = val & TRANSPORT_SELECT;
      reset  = val & TRANSPORT_RESET;
    }

    bool save(Serializer& out) const override {
      try
      {
        out.putBool(bw);      out.putBool(fire);
        out.putBool(select);  out.putBool(reset);
        out.putBool(right);   out.putBool(left);
        out.putBool(up);      out.putBool(down);
      }
      catch(...)
      {
        return false;
      }
      return true;
    }

    bool load(Serializer& in) override {
      try
      {
        bw = in.getBool();      fire = in.getBool();
        select = in.getBool();  reset = in.getBool();
        right = in.getBool();   left = in.getBool();
        up = in.getBool();      down = in.getBool();
      }
      catch(...)
      {
        return false;
      }
      return true;
    }

  private:
    static constexpr uInt8
        TRANSPORT_RIGHT   = 0x10,
        TRANSPORT_LEFT    = 0x08,
        TRANSPORT_DOWN    = 0x04,
        TRANSPORT_UP      = 0x02,
        TRANSPORT_UNUSED1 = 0x01; // Right-2

    static constexpr uInt8
        TRANSPORT_BW      = 0x10,
        TRANSPORT_UNUSED2 = 0x08,
        TRANSPORT_SELECT  = 0x04,
        TRANSPORT_RESET   = 0x02,
        TRANSPORT_BUTTON  = 0x01;
  };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
  Various kernel, OSD and scale definitions
  @author  Rob Bairos
*/

#define TIMECODE_HEIGHT     12
#define MAX_LEVEL           11
#define DEFAULT_LEVEL       6
#define BLANK_LINE_SIZE   (30+3+37-1) // 70-1


// Automatically generated
// Several not used
// #define addr_kernel_48 0x800
#define addr_transport_direction 0x880
#define addr_transport_buttons 0x894
#define addr_right_line 0x948
#define addr_set_gdata6 0x948
#define addr_set_aud_right 0x94e
#define addr_set_gdata9 0x950
#define addr_set_gcol9 0x954
#define addr_set_gcol6 0x956
#define addr_set_gdata5 0x95a
#define addr_set_gcol5 0x95e
#define addr_set_gdata8 0x962
#define addr_set_colubk_r 0x966
#define addr_set_gcol7 0x96a
#define addr_set_gdata7 0x96e
#define addr_set_gcol8 0x972
// #define addr_left_line 0x980
#define addr_set_gdata1 0x982
#define addr_set_gcol1 0x988
#define addr_set_aud_left 0x98c
#define addr_set_gdata4 0x990
#define addr_set_gcol4 0x992
#define addr_set_gdata0 0x994
#define addr_set_gcol0 0x998
#define addr_set_gdata3 0x99c
#define addr_set_colupf_l 0x9a0
#define addr_set_gcol2 0x9a4
#define addr_set_gdata2 0x9a8
#define addr_set_gcol3 0x9ac
#define addr_pick_continue 0x9be
// #define addr_main_start 0xa00
// #define addr_aud_bank_setup 0xa0c
// #define addr_tg0 0xa24
// #define addr_title_again 0xa3b
// #define addr_wait_cnt 0xa77
#define addr_end_lines 0xa80
#define addr_set_aud_endlines 0xa80
#define addr_set_overscan_size 0xa9a
#define addr_set_vblank_size 0xab0
#define addr_pick_extra_lines 0xab9
#define addr_pick_transport 0xac6
// #define addr_wait_lines 0xac9
// #define addr_transport_done1 0xada
// #define addr_draw_title 0xb00
#define addr_title_loop 0xb50
// #define addr_black_bar 0xb52
// #define addr_animate_bar1 0xb58
// #define addr_animate_bar_again1 0xb5a
// #define addr_animate_dex1 0xb65
#define addr_audio_bank 0xb80
// #define addr_reset_loop 0xbfa

// scale adjustments, automatically generated
static constexpr uInt8 scale0[16] = {
  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8   /* 0.0000 */
};
static constexpr uInt8 scale1[16] = {
  6,  6,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  9,  9   /* 0.1667 */
};
static constexpr uInt8 scale2[16] = {
  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10   /* 0.3333 */
};
static constexpr uInt8 scale3[16] = {
  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 11   /* 0.5000 */
};
static constexpr uInt8 scale4[16] = {
  3,  3,  4,  5,  5,  6,  7,  7,  8,  9,  9, 10, 11, 11, 12, 13   /* 0.6667 */
};
static constexpr uInt8 scale5[16] = {
  1,  2,  3,  4,  5,  5,  6,  7,  8,  9, 10, 10, 11, 12, 13, 14   /* 0.8333 */
};
static constexpr uInt8 scale6[16] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15   /* 1.0000 */
};
static constexpr uInt8 scale7[16] = {
  0,  0,  0,  1,  3,  4,  5,  7,  8, 10, 11, 12, 14, 15, 15, 15   /* 1.3611 */
};
static constexpr uInt8 scale8[16] = {
  0,  0,  0,  0,  1,  3,  5,  7,  8, 10, 12, 14, 15, 15, 15, 15   /* 1.7778 */
};
static constexpr uInt8 scale9[16] = {
  0,  0,  0,  0,  0,  2,  4,  6,  9, 11, 13, 15, 15, 15, 15, 15   /* 2.2500 */
};
static constexpr uInt8 scale10[16] = {
  0,  0,  0,  0,  0,  1,  3,  6,  9, 12, 14, 15, 15, 15, 15, 15   /* 2.7778 */
};
static const uInt8* scales[11] = {
  scale0, scale1, scale2, scale3, scale4, scale5,
  scale6, scale7, scale8, scale9, scale10
};

// lower bit is ignored anyways
static constexpr uInt8 shiftBright[16 + MAX_LEVEL - 1] = {
  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
  7,  8,  9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15
};

// Compiled kernel
static constexpr unsigned char kernelROM[] = {
 133, 2, 185, 50, 248, 133, 27, 185, 62, 248, 133, 28, 185, 74, 248, 133,
 27, 185, 86, 248, 133, 135, 185, 98, 248, 190, 110, 248, 132, 136, 164, 135,
 132, 28, 133, 27, 134, 28, 134, 27, 164, 136, 102, 137, 176, 210, 136, 16,
 207, 96, 0, 1, 1, 1, 0, 0, 48, 48, 50, 53, 56, 48, 249, 129,
 129, 128, 248, 0, 99, 102, 102, 102, 230, 99, 140, 252, 140, 136, 112, 0,
 192, 97, 99, 102, 102, 198, 198, 198, 248, 198, 248, 0, 193, 32, 48, 24,
 24, 25, 24, 24, 24, 24, 126, 0, 249, 97, 97, 97, 97, 249, 0, 0,
 0, 0, 0, 0, 248, 128, 128, 224, 128, 248, 255, 255, 255, 255, 255, 255,
 173, 128, 2, 74, 74, 74, 133, 129, 234, 133, 128, 133, 128, 133, 128, 133,
 128, 76, 72, 249, 165, 12, 10, 173, 130, 2, 42, 41, 23, 133, 129, 234,
 133, 128, 133, 128, 133, 128, 76, 72, 249, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 169, 159, 133, 28, 133, 42, 169, 0,
 162, 223, 133, 25, 160, 98, 169, 248, 133, 7, 169, 231, 133, 27, 169, 242,
 133, 6, 169, 247, 133, 28, 169, 0, 133, 9, 169, 172, 133, 6, 169, 253,
 133, 27, 169, 216, 133, 7, 134, 27, 132, 6, 169, 0, 133, 9, 133, 43,
 169, 0, 169, 207, 133, 42, 133, 28, 169, 54, 133, 7, 169, 0, 133, 25,
 162, 191, 160, 114, 169, 243, 133, 27, 169, 66, 133, 6, 169, 239, 133, 28,
 169, 0, 133, 8, 169, 238, 133, 6, 169, 251, 133, 27, 169, 182, 133, 7,
 134, 27, 132, 6, 169, 0, 133, 8, 169, 128, 133, 32, 133, 33, 76, 72,
 249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 120, 216, 162, 255, 154, 169, 0, 149, 0, 202, 208, 251, 169, 128, 133, 130,
 169, 251, 133, 131, 169, 1, 133, 37, 133, 38, 169, 3, 133, 4, 133, 5,
 133, 2, 162, 4, 133, 128, 202, 208, 251, 133, 128, 133, 16, 133, 17, 169,
 208, 133, 32, 169, 224, 133, 33, 133, 2, 133, 42, 165, 132, 106, 106, 133,
 6, 133, 7, 169, 85, 133, 137, 32, 0, 251, 176, 239, 169, 0, 133, 9,
 133, 37, 133, 27, 133, 16, 234, 133, 17, 133, 28, 133, 27, 133, 28, 169,
 6, 133, 4, 169, 2, 133, 5, 169, 1, 133, 38, 169, 0, 133, 32, 169,
 240, 133, 33, 133, 42, 162, 5, 202, 208, 253, 133, 43, 76, 128, 250, 255,
 169, 0, 162, 0, 160, 0, 132, 27, 132, 28, 133, 25, 132, 27, 169, 207,
 133, 13, 169, 51, 133, 14, 169, 204, 133, 15, 162, 29, 32, 201, 250, 169,
 2, 133, 0, 162, 3, 32, 201, 250, 169, 0, 133, 0, 169, 2, 133, 1,
 162, 37, 32, 201, 250, 162, 0, 134, 1, 162, 0, 240, 9, 32, 201, 250,
 234, 234, 133, 128, 133, 128, 76, 148, 248, 133, 2, 169, 0, 177, 130, 133,
 25, 165, 129, 240, 5, 198, 129, 173, 128, 20, 200, 202, 208, 235, 96, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 162, 30, 32, 82, 251, 169, 2, 133, 0, 162, 3, 32, 82, 251, 169, 0,
 133, 0, 169, 2, 133, 1, 162, 37, 32, 82, 251, 169, 0, 133, 1, 198,
 132, 165, 132, 133, 133, 160, 255, 162, 30, 32, 88, 251, 162, 54, 32, 82,
 251, 160, 11, 32, 0, 248, 169, 0, 133, 27, 133, 28, 133, 27, 133, 28,
 162, 54, 32, 82, 251, 165, 132, 133, 133, 160, 1, 162, 30, 32, 88, 251,
 56, 96, 169, 0, 133, 133, 160, 0, 132, 134, 24, 165, 133, 101, 134, 133,
 133, 133, 2, 133, 9, 202, 208, 242, 96, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 250, 0, 250, 0, 250,
};

// OSD labels
static constexpr uInt8 brightLabelEven[] = {
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 225, 48, 12, 252,
  6, 140, 231, 96, 0,
  0, 113, 48, 12, 96,
  6, 140, 192, 96, 0,
  0, 225, 49, 15, 96,
  6, 152, 195, 96, 0,
  0, 49, 48, 12, 96,
  6, 140, 231, 96, 0,
  0, 225, 48, 12, 96,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0
};

static constexpr uInt8 brightLabelOdd[] = {
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  7, 252, 126, 99, 0,
  0, 113, 48, 12, 96,
  6, 140, 192, 96, 0,
  0, 97, 49, 12, 96,
  7, 248, 223, 224, 0,
  0, 113, 49, 12, 96,
  6, 156, 195, 96, 0,
  0, 113, 48, 12, 96,
  7, 142, 127, 96, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0
};

static constexpr uInt8 volumeLabelEven[] = {
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 199, 192, 14, 254,
  113, 112, 99, 112, 0,
  0, 140, 192, 14, 192,
  51, 48, 99, 240, 0,
  0, 28, 192, 15, 254,
  31, 48, 99, 240, 0,
  0, 12, 192, 15, 192,
  30, 112, 119, 176, 0,
  0, 7, 252, 12, 254,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0
};

static constexpr uInt8 volumeLabelOdd[] = {
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  97, 224, 99, 112, 0,
  0, 142, 192, 14, 192,
  51, 112, 99, 112, 0,
  0, 28, 192, 15, 192,
  59, 48, 99, 240, 0,
  0, 28, 192, 15, 192,
  30, 112, 99, 176, 0,
  0, 14, 192, 13, 192,
  14, 224, 62, 48, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0
};

// Level bars
// 8 rows * 5 columns = 40
static constexpr uInt8 levelBarsEvenData[] = {
  /*0*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*1*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*2*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*3*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*4*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*5*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*6*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*7*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*8*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*9*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  /*10*/
  0, 0, 0, 0, 0,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
};

static constexpr uInt8 levelBarsOddData[] = {
  /*0*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*1*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x41, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*2*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*3*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x41, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*4*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*5*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x41, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*6*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*7*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x41, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*8*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*9*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x41,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  /*10*/
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};

////////////////////////////////////////////////////////////////////////////////
class MovieCart : public Serializable
{
  public:
    MovieCart() = default;

    bool init(const string& path);
    bool process(uInt16 address);

    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    uInt8 readROM(uInt16 address) const {
      return myROM[address & 1023];
    }

    void writeROM(uInt16 address, uInt8 data) {
      myROM[address & 1023] = data;
    }

  private:
    enum Mode : uInt8
    {
      Volume,
      Bright,
      Time,
      Last = Time
    };

    enum class TitleState
    {
      Display,
      Exiting,
      Stream
    };

    void stopTitleScreen() {
      // clear carry, one bit difference from 0x38 sec
      writeROM(addr_title_loop + 0, 0x18);
    }

    void writeColor(uInt16 address, uInt8 val);
    void writeAudioData(uInt16 address, uInt8 val) {
      writeROM(address, myVolumeScale[val]);
    }
    void writeAudio(uInt16 address) {
      writeAudioData(address, myStream.readAudio());
    }
    void writeGraph(uInt16 address) {
      writeROM(address, myStream.readGraph());
    }

    void runStateMachine();

    void fill_addr_right_line();
    void fill_addr_left_line(bool again);
    void fill_addr_end_lines();
    void fill_addr_blank_lines();

    void updateTransport();

    // data
    uInt8 myROM[1024];

    // title screen state
    int        myTitleCycles{0};
    TitleState myTitleState{TitleState::Display};

    // address info
    bool  myA7{false};
    bool  myA10{false};
    uInt8 myA10_Count{0};

    // state machine info
    uInt8 myState{3};
    bool  myPlaying{true};
    bool  myOdd{true};
    bool  myBufferIndex{false};

    uInt8 myLines{0};
    Int32 myFrameNumber{1};  // signed

    uInt8 myMode{Mode::Volume};
    uInt8 myBright{DEFAULT_LEVEL};
    uInt8 myForceColor{0};

    // expressed in frames
    uInt8 myDrawLevelBars{0};
    uInt8 myDrawTimeCode{0};

    StreamReader myStream;
    MovieInputs myInputs;
    MovieInputs myLastInputs;

    Int8  mySpeed{1};  // signed
    uInt8 myJoyRepeat{0};
    uInt8 myDirectionValue{0};
    uInt8 myButtonsValue{0};

    uInt8        myVolume{DEFAULT_LEVEL};
    const uInt8* myVolumeScale{scales[DEFAULT_LEVEL]};
    uInt8        myFirstAudioVal{0};
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MovieCart::init(const string& path)
{
  memcpy(myROM, kernelROM, 1024);

  myTitleCycles = 0;
  myTitleState = TitleState::Display;

  myA7 = false;
  myA10 = false;
  myA10_Count = 0;

  myState = 3;
  myPlaying = true;
  myOdd = true;
  myBufferIndex = false;
  myFrameNumber = 1;

  myInputs.init();
  myLastInputs.init();
  mySpeed = 1;
  myJoyRepeat = 0;
  myDirectionValue = 0;
  myButtonsValue = 0;

  myLines = 0;
  myForceColor = 0;
  myDrawLevelBars = 0;
  myDrawTimeCode = 0;
  myFirstAudioVal = 0;

  myMode = Mode::Volume;
  myVolume = DEFAULT_LEVEL;
  myVolumeScale = scales[DEFAULT_LEVEL];
  myBright = DEFAULT_LEVEL;

  if(!myStream.open(path))
    return false;

  myStream.swapField(true, myOdd);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::writeColor(uInt16 address, uInt8 v)
{
  v = (v & 0xf0) | shiftBright[(v & 0x0f) + myBright];

  if(myForceColor)
    v = myForceColor;
  if(myInputs.bw)
    v &= 0x0f;

  writeROM(address, v);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::updateTransport()
{
  myStream.overrideGraph(nullptr);

  // have to cut rate in half, to remove glitches...todo..
  {
    if(myBufferIndex)
    {
      uInt8 temp = ~(myA10_Count & 0x1e) & 0x1e;

      if (temp == myDirectionValue)
        myInputs.updateDirection(temp);

      myDirectionValue = temp;
    }
    else
    {
      uInt8 temp = ~(myA10_Count & 0x17) & 0x17;

      if(temp == myButtonsValue)
        myInputs.updateTransport(temp);

      myButtonsValue = temp;
    }

    myA10_Count = 0;
  }

  if(myInputs.reset)
  {
    myFrameNumber = 1;
    myPlaying = true;
    myDrawTimeCode = OSD_FRAMES;

    // goto update_stream;
    myLastInputs = myInputs;
    return;
  }

  uInt8 lastMainMode = myMode;

  if(myInputs.up && !myLastInputs.up)
  {
    if(myMode == 0)
      myMode = Mode::Last;
    else
      myMode--;
  }
  else if(myInputs.down && !myLastInputs.down)
  {
    if (myMode == Mode::Last)
      myMode = 0;
    else
      myMode++;
  }

  if(myInputs.left || myInputs.right)
  {
    myJoyRepeat++;
  }
  else
  {
    myJoyRepeat = 0;
    mySpeed = 1;
  }

  if(myJoyRepeat & 16)
  {
    myJoyRepeat = 0;

    if(myInputs.left || myInputs.right)
    {
      if(myMode == Mode::Time)
      {
        myDrawTimeCode = OSD_FRAMES;
        mySpeed += 4;
        if (mySpeed < 0)
          mySpeed -= 4;
      }
      else if(myMode == Mode::Volume)
      {
        myDrawLevelBars = OSD_FRAMES;
        if(myInputs.left)
        {
          if (myVolume)
            myVolume--;
        }
        else
        {
          myVolume++;
          if(myVolume >= MAX_LEVEL)
            myVolume--;
        }
      }
      else if(myMode == Mode::Bright)
      {
        myDrawLevelBars = OSD_FRAMES;
        if(myInputs.left)
        {
          if(myBright)
            myBright--;
        }
        else
        {
          myBright++;
          if(myBright >= MAX_LEVEL)
            myBright--;
        }
      }
    }
  }

  if(myInputs.select && !myLastInputs.select)
  {
    myDrawTimeCode = OSD_FRAMES;
    myFrameNumber -= 60 * BACK_SECONDS + 1;
    //goto update_stream;
    myLastInputs = myInputs;
    return;
  }

  if(myInputs.fire && !myLastInputs.fire)
    myPlaying = !myPlaying;

  switch(myMode)
  {
    case Mode::Time:
      if(lastMainMode != myMode)
        myDrawTimeCode = OSD_FRAMES;
      break;

    case Mode::Bright:
    case Mode::Volume:
    default:
      if(lastMainMode != myMode)
        myDrawLevelBars = OSD_FRAMES;
      break;
  }

  // just draw one
  if(myDrawLevelBars > myDrawTimeCode)
    myDrawTimeCode = 0;
  else
    myDrawLevelBars = 0;

  if(myPlaying)
    myVolumeScale = scales[myVolume];
  else
    myVolumeScale = scales[0];

  // update frame
  Int8 step = 1;

  if(!myPlaying)  // step while paused
  {
    if(myMode == Mode::Time)
    {
      if(myInputs.right && !myLastInputs.right)
        step = 2;
      else if(myInputs.left && !myLastInputs.left)
        step = -2;
      else
        step = (myFrameNumber & 1) ? -1 : 1;
    }
    else
    {
      step = (myFrameNumber & 1) ? -1 : 1;
    }
  }
  else
  {
    if(myMode == Mode::Time)
    {
      if(myInputs.right)
        step = mySpeed;
      else if (myInputs.left)
        step = -mySpeed;
    }
    else
    {
      step = 1;
    }
  }

  myFrameNumber += step;
  if(myFrameNumber < 1)
  {
    myFrameNumber = 1;
    mySpeed = 1;
  }

  myLastInputs = myInputs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::fill_addr_right_line()
{
  uint8_t v;
  writeAudio(addr_set_aud_right + 1);

  writeGraph(addr_set_gdata5 + 1);
  writeGraph(addr_set_gdata6 + 1);
  writeGraph(addr_set_gdata7 + 1);
  writeGraph(addr_set_gdata8 + 1);
  writeGraph(addr_set_gdata9 + 1);

  v = myStream.readColor();
  writeColor(addr_set_gcol5 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol6 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol7 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol8 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol9 + 1, v);

  // alternate between background color and playfield color
  if (myForceColor)
  {
      v = 0;
      writeROM(addr_set_colubk_r + 1, v);
  }
  else
  {
      v = myStream.readColorBK();
	  writeColor(addr_set_colubk_r + 1, v);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::fill_addr_left_line(bool again)
{
  uint8_t v;

  writeAudio(addr_set_aud_left + 1);

  writeGraph(addr_set_gdata0 + 1);
  writeGraph(addr_set_gdata1 + 1);
  writeGraph(addr_set_gdata2 + 1);
  writeGraph(addr_set_gdata3 + 1);
  writeGraph(addr_set_gdata4 + 1);

  v = myStream.readColor();
  writeColor(addr_set_gcol0 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol1 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol2 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol3 + 1, v);

  v = myStream.readColor();
  writeColor(addr_set_gcol4 + 1, v);


  // alternate between background color and playfield color
  if (myForceColor)
  {
      v = 0;
      writeROM(addr_set_colupf_l + 1, v);
  }
  else
  {
      v = myStream.readColorBK();
      writeColor(addr_set_colupf_l + 1, v);
  }

  // addr_pick_line_end
  //    jmp right_line
  //    jmp end_lines
  if(again)
  {
    writeROM(addr_pick_continue + 1, LO_JUMP_BYTE(addr_right_line));
    writeROM(addr_pick_continue + 2, HI_JUMP_BYTE(addr_right_line));
  }
  else
  {
    writeROM(addr_pick_continue + 1, LO_JUMP_BYTE(addr_end_lines));
    writeROM(addr_pick_continue + 2, HI_JUMP_BYTE(addr_end_lines));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::fill_addr_end_lines()
{
  writeAudio(addr_set_aud_endlines + 1);

  if(!myOdd)
    myFirstAudioVal = myStream.readAudio();

  // keep at overscan=29, vblank=36
  //      or overscan=30, vblank=36 + 1 blank line

  if(myOdd)
  {
    writeROM(addr_set_overscan_size + 1, 29);
    writeROM(addr_set_vblank_size + 1, 36);

    writeROM(addr_pick_extra_lines + 1, 0);
  }
  else
  {
    writeROM(addr_set_overscan_size + 1, 30);
    writeROM(addr_set_vblank_size + 1, 36);

    // extra line after vblank
    writeROM(addr_pick_extra_lines + 1, 1);
  }

  if(!myBufferIndex)
  {
    writeROM(addr_pick_transport + 1, LO_JUMP_BYTE(addr_transport_direction));
    writeROM(addr_pick_transport + 2, HI_JUMP_BYTE(addr_transport_direction));
  }
  else
  {
    writeROM(addr_pick_transport + 1, LO_JUMP_BYTE(addr_transport_buttons));
    writeROM(addr_pick_transport + 2, HI_JUMP_BYTE(addr_transport_buttons));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::fill_addr_blank_lines()
{
  // version number
  myStream.readVersion();
  myStream.readVersion();
  myStream.readVersion();
  myStream.readVersion();

  // frame number
  myStream.readFrame();
  myStream.readFrame();
  uInt8 v = myStream.readFrame();

  // make sure we're in sync with frame data
  myOdd = (v & 1);

  // 30 overscan
  // 3 vsync
  // 37 vblank

  if(myOdd)
  {
    writeAudioData(addr_audio_bank + 0, myFirstAudioVal);
    for(uInt8 i = 1; i < (BLANK_LINE_SIZE + 1); i++)
      writeAudio(addr_audio_bank + i);
  }
  else
  {
    for(uInt8 i = 0; i < (BLANK_LINE_SIZE -1); i++)
      writeAudio(addr_audio_bank + i);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MovieCart::runStateMachine()
{
  switch(myState)
  {
    case 1:
      if(myA7)
      {
        if(myLines == (TIMECODE_HEIGHT-1))
        {
          if(myDrawTimeCode)
          {
            myDrawTimeCode--;
            myForceColor = COLOR_BLUE;
            myStream.startTimeCode();
          }
        }

        // label = 12, bars = 7
        if(myLines == 21)
        {
          if(myDrawLevelBars)
          {
            myDrawLevelBars--;
            myForceColor = COLOR_BLUE;

            switch(myMode)
            {
              case Mode::Time:
                myStream.overrideGraph(nullptr);
                break;

              case Mode::Bright:
                if(myOdd)
                  myStream.overrideGraph(brightLabelOdd);
                else
                  myStream.overrideGraph(brightLabelEven);
                break;

              case Mode::Volume:
              default:
                if(myOdd)
                  myStream.overrideGraph(volumeLabelOdd);
                else
                  myStream.overrideGraph(volumeLabelEven);
                break;
            }
          }
        }

        if(myLines == 7)
        {
          if(myDrawLevelBars)
          {
            uInt8 levelValue = 0;

            switch(myMode)
            {
              case Mode::Time:
                levelValue = 0;
                break;

              case Mode::Bright:
                levelValue = myBright;
                break;

              case Mode::Volume:
              default:
                levelValue = myVolume;
                break;
            }

            if(myOdd)
              myStream.overrideGraph(&levelBarsOddData[levelValue * 40]);
            else
              myStream.overrideGraph(&levelBarsEvenData[levelValue * 40]);
          }
        }

        fill_addr_right_line();

        myLines -= 1;
        myState = 2;
      }
      break;

    case 2:
      if(!myA7)
      {
         if(myOdd)
         {
            if(myDrawTimeCode)
            {
               if (myLines == (TIMECODE_HEIGHT - 0))
                  myStream.blankPartialLines(true);
            }
            if(myDrawLevelBars)
            {
               if(myLines == 22)
                  myStream.blankPartialLines(true);
            }
        }  

        if(myLines >= 1)
        {
          fill_addr_left_line(1);

          myLines -= 1;
          myState = 1;
        }
        else
        {
          fill_addr_left_line(0);
          fill_addr_end_lines();

          myStream.swapField(myBufferIndex, myOdd);
          myStream.blankPartialLines(myOdd);

          myBufferIndex = !myBufferIndex;
          updateTransport();

          fill_addr_blank_lines();

          myState = 3;
        }
      }
      break;

    case 3:
      if(myA7)
      {
        // hit end? rewind just before end
        while (myFrameNumber >= 2 &&
            !myStream.readField(myFrameNumber, myBufferIndex))
        {
          myFrameNumber -= 2;
          myJoyRepeat = 0;
          myPlaying = false;
        }

        myForceColor = 0;
        myLines = 191;
        myState = 1;
      }
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MovieCart::process(uInt16 address)
{
  bool a12 = (address & (1 << 12)) ? 1:0;
  bool a11 = (address & (1 << 11)) ? 1:0;

  // count a10 pulses
  bool a10i = (address & (1 << 10));
  if(a10i && !myA10)
    myA10_Count++;
  myA10 = a10i;

  // latch a7 state
  if(a11)  // a12
    myA7 = (address & (1 << 7));    // each 128

  switch(myTitleState)
  {
    case TitleState::Display:
      myTitleCycles++;
      if(myTitleCycles == TITLE_CYCLES)
      {
        stopTitleScreen();
        myTitleState = TitleState::Exiting;
        myTitleCycles = 0;
      }
      break;

    case TitleState::Exiting:
      if(myA7)
        myTitleState = TitleState::Stream;
      break;

    case TitleState::Stream:
      runStateMachine();
      break;
  }

  return a12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MovieCart::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myROM, 1024);

    // title screen state
    out.putInt(myTitleCycles);
    out.putInt(static_cast<int>(myTitleState));

    // address info
    out.putBool(myA7);
    out.putBool(myA10);
    out.putByte(myA10_Count);

    // state machine info
    out.putByte(myState);
    out.putBool(myPlaying);
    out.putBool(myOdd);
    out.putBool(myBufferIndex);

    out.putByte(myLines);
    out.putInt(myFrameNumber);

    out.putByte(myMode);
    out.putByte(myBright);
    out.putByte(myForceColor);

    // expressed in frames
    out.putByte(myDrawLevelBars);
    out.putByte(myDrawTimeCode);

    if(!myStream.save(out)) throw;
    if(!myInputs.save(out)) throw;
    if(!myLastInputs.save(out)) throw;

    out.putByte(mySpeed);
    out.putByte(myJoyRepeat);
    out.putByte(myDirectionValue);
    out.putByte(myButtonsValue);

    out.putByte(myVolume);
    // FIXME - determine whether we need to load/save this
    // const uInt8* myVolumeScale{scales[DEFAULT_LEVEL]};
    out.putByte(myFirstAudioVal);
  }
  catch(...)
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MovieCart::load(Serializer& in)
{
  try
  {
    in.getByteArray(myROM, 1024);

    // title screen state
    myTitleCycles = in.getInt();
    myTitleState = static_cast<TitleState>(in.getInt());

    // address info
    myA7 = in.getBool();
    myA10 = in.getBool();
    myA10_Count = in.getByte();

    // state machine info
    myState = in.getByte();
    myPlaying = in.getBool();
    myOdd = in.getBool();
    myBufferIndex = in.getBool();

    myLines = in.getByte();
    myFrameNumber = in.getInt();

    myMode = in.getByte();
    myBright = in.getByte();
    myForceColor = in.getByte();

    // expressed in frames
    myDrawLevelBars = in.getByte();
    myDrawTimeCode = in.getByte();

    if(!myStream.load(in)) throw;
    if(!myInputs.load(in)) throw;
    if(!myLastInputs.load(in)) throw;

    mySpeed = in.getByte();
    myJoyRepeat = in.getByte();
    myDirectionValue = in.getByte();
    myButtonsValue = in.getByte();

    myVolume = in.getByte();
    // FIXME - determine whether we need to load/save this
    // const uInt8* myVolumeScale{scales[DEFAULT_LEVEL]};
    myFirstAudioVal = in.getByte();
  }
  catch(...)
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMVC::CartridgeMVC(const string& path, size_t size,
                           const string& md5, const Settings& settings,
                           size_t bsSize)
  : Cartridge(settings, md5),
    mySize{bsSize},
    myPath{path}
{
  myMovie = make_unique<MovieCart>();

  // not used
  myImage = make_unique<uInt8[]>(mySize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMVC::~CartridgeMVC()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMVC::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PageAccessType::READWRITE);
  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMVC::reset()
{
  myMovie->init(myPath);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeMVC::getImage(size_t& size) const
{
  // not used
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::patch(uInt16 address, uInt8 value)
{
  myMovie->writeROM(address, value);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMVC::peek(uInt16 address)
{
  myMovie->process(address);
  return myMovie->readROM(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::poke(uInt16 address, uInt8 value)
{
  return myMovie->process(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::save(Serializer& out) const
{
  try
  {
    if(!myMovie->save(out)) throw;
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::load(Serializer& in)
{
  try
  {
    if(!myMovie->load(in)) throw;
  }
  catch(...)
  {
    return false;
  }

  return true;
}
