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

#ifndef KIDVID_HXX
#define KIDVID_HXX

class Event;
class Sound;
class OSystem;

#include "bspf.hxx"
#include "Control.hxx"

/**
  The KidVid Voice Module, created by Coleco.  This class emulates the
  KVVM cassette player by mixing WAV data into the sound stream.  The
  WAV files are located at:

    http://www.atariage.com/2600/archives/KidVidAudio/index.html

  This code was heavily borrowed from z26.

  @author  Thomas Jentzsch & z26 team
*/
class KidVid : public Controller
{
  public:
    /**
      Create a new KidVid controller plugged into the specified jack

      @param jack      The jack the controller is plugged into
      @param event     The event object to use for events
      @param osystem   The OSystem object to use
      @param system    The system using this controller
      @param romMd5    The md5 of the ROM using this controller
      @param callback  Called to pass messages back to the parent controller
    */
    KidVid(Jack jack, const Event& event, const OSystem& osystem,
           const System& system, string_view romMd5,
           const onMessageCallbackForced& callback);
    ~KidVid() override = default;

  public:
    /**
      Write the given value to the specified digital pin for this
      controller.  Writing is only allowed to the pins associated
      with the PIA.  Therefore you cannot write to pin six.

      @param pin The pin of the controller jack to write to
      @param value The value to write to the pin
    */
    void write(DigitalPin pin, bool value) override;

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update() override;

    /**
      Saves the current state of this controller to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out) const override;

    /**
      Loads the current state of this controller from the given Serializer.

      @param in The serializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in) override;

    /**
      Returns the name of this controller.
    */
    string name() const override { return "KidVid"; }

  private:
    // Get name of the current sample file
    string getFileName() const;

    // Map myTape (1-4) + myGame to an index
    uInt32 tapeIndex() const;

    // Map myTape (1-4) to the cassette label game number (1-3)
    // Tape selection order is 2,3,1 (keyboard) or 1-4 (select button)
    // but games are numbered 1,2,3 on the cassette labels
    uInt32 gameNumber() const;

    // Open/close a WAV sample file
    void openSampleFiles();

    // Jump to next song in the sequence
    void setNextSong();

  private:
    const OSystem& myOSystem;
    const string myBasePath;

    // Sends messages back to the parent class
    Controller::onMessageCallbackForced myCallback;

    enum class Game: uInt8 {
      Smurfs,
      BBears
    };
    static constexpr uInt32
      NumBlocks      = 6,              // number of bytes / block
      NumBlockBits   = NumBlocks * 8,  // number of bits / block
      SongPosSize    = 44 + 38 + 42 + 62 + 80 + 62,
      SongStartSize  = 104,
      TapeFrames     = 60,
      ClickFrames    = 3,              // eliminate click noise at song end
      KVData00Offset = 12 * 8,         // KVData00 - ourData = 12 (2 * 6)
      KVData80Offset = 42 * 8,         // KVData80 - ourData = 42 (7 * 6)
      KVPauseOffset  = 36 * 8          // KVPause  - ourData = 36 (6 * 6)
    ;

    // Which game is using the KidVid (Smurfs or Berenstain Bears)
    Game myGame{Game::Smurfs};

    // Whether the KidVid device is enabled (only for games that it
    // supports, and if it's plugged into the right port)
    bool myEnabled{false};

    // Indicates if the sample files have been found
    bool myFilesFound{false};

    // Is the tape currently 'busy' / in use?
    bool myTapeBusy{false};

    // True when a song is currently playing
    bool mySongPlaying{false};

    // Set after a state load when a song was mid-play; triggers
    // resumption of WAV playback at the correct offset in update()
    bool myContinueSong{false};

    // Current position in the song sequence
    uInt32 mySongPointer{0};

    // Remaining length of the current WAV sample in frames
    uInt32 mySongLength{0};

    // True when the current song ends with a beep rather than silence
    bool myBeep{false};

    // Which physical tape is inserted (1-4, or 0 if none)
    uInt32 myTape{0};

    // Current bit index into ourData
    uInt32 myIdx{0};

    // Current block number within the tape sequence
    uInt32 myBlock{0};

    // Number of bits remaining in the current block
    uInt32 myBlockIdx{0};

    // Number of blocks and data on tape
    static constexpr std::array<uInt8, NumBlocks> ourBlocks = {
      2+40, 2+21, 2+35,     /* Smurfs tapes 3, 1, 2 */
      42+60, 42+78, 42+60   /* BBears tapes 1, 2, 3 (40 extra blocks for intro) */
    };
    static constexpr std::array<uInt8, NumBlockBits> ourData = {
      /* KVData44, Smurfs */
      0x7b,  // 0111 1011b  ; (1)0
      0x1e,  // 0001 1110b  ; 1
      0xc6,  // 1100 0110b  ; 00
      0x31,  // 0011 0001b  ; 01
      0xec,  // 1110 1100b  ; 0
      0x60,  // 0110 0000b  ; 0+

      /* KVData48, BBears */
      0x7b,  // 0111 1011b  ; (1)0
      0x1e,  // 0001 1110b  ; 1
      0xc6,  // 1100 0110b  ; 00
      0x3d,  // 0011 1101b  ; 10
      0x8c,  // 1000 1100b  ; 0
      0x60,  // 0110 0000b  ; 0+

      /* KVData00 */
      0xf6,  // 1111 0110b
      0x31,  // 0011 0001b
      0x8c,  // 1000 1100b
      0x63,  // 0110 0011b
      0x18,  // 0001 1000b
      0xc0,  // 1100 0000b

      /* KVData01 */
      0xf6,  // 1111 0110b
      0x31,  // 0011 0001b
      0x8c,  // 1000 1100b
      0x63,  // 0110 0011b
      0x18,  // 0001 1000b
      0xf0,  // 1111 0000b

      /* KVData02 */
      0xf6,  // 1111 0110b
      0x31,  // 0011 0001b
      0x8c,  // 1000 1100b
      0x63,  // 0110 0011b
      0x1e,  // 0001 1110b
      0xc0,  // 1100 0000b

      /* KVData03 */
      0xf6,  // 1111 0110b
      0x31,  // 0011 0001b
      0x8c,  // 1000 1100b
      0x63,  // 0110 0011b
      0x1e,  // 0001 1110b
      0xf0,  // 1111 0000b

      /* KVPause */
      0x3f,  // 0011 1111b
      0xf0,  // 1111 0000b
      0x00,  // 0000 0000b
      0x00,  // 0000 0000b
      0x00,  // 0000 0000b
      0x00,  // 0000 0000b

      /* KVData80 */
      0xf7,  // 1111 0111b  ; marks end of tape (green/yellow screen)
      0xb1,  // 1011 0001b
      0x8c,  // 1000 1100b
      0x63,  // 0110 0011b
      0x18,  // 0001 1000b
      0xc0   // 1100 0000b
    };
    static constexpr std::array<uInt8, SongPosSize> ourSongPositions = {
      /* kvs1 44 */
      11, 12+0x80, 13+0x80, 14, 15+0x80, 16, 8+0x80, 17, 18+0x80, 19, 20+0x80,
      21, 8+0x80, 22, 15+0x80, 23, 18+0x80, 14, 20+0x80, 16, 18+0x80,
      17, 15+0x80, 19, 8+0x80, 21, 20+0x80, 22, 18+0x80, 23, 15+0x80,
      14, 20+0x80, 16, 8+0x80, 22, 15+0x80, 23, 18+0x80, 14, 20+0x80,
      16, 8+0x80, 9,

      /* kvs2 38 */
      25+0x80, 26, 27, 28, 8, 29, 30, 26, 27, 28, 8, 29, 30, 26, 27, 28, 8, 29,
      30, 26, 27, 28, 8, 29, 30, 26, 27, 28, 8, 29, 30, 26, 27, 28, 8, 29,
      30+0x80, 9,

      /* kvs3 42 */
      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 34, 42, 36, 43, 40, 39, 38, 37,
      34, 43, 36, 39, 40, 37, 38, 43, 34, 37, 36, 43, 40, 39, 38, 37, 34, 43,
      36, 39, 40, 37, 38+0x80, 9,

      /* kvb1 62 */
      0, 1, 45, 2, 3, 46, 4, 5, 47, 6, 7, 48, 4, 3, 49, 2, 1, 50, 6, 7, 51,
      4, 5, 52, 6, 1, 53, 2, 7, 54, 6, 5, 45, 2, 1, 46, 4, 3, 47, 2, 5, 48,
      4, 7, 49, 6, 1, 50, 2, 5, 51, 6, 3, 52, 4, 7, 53, 2, 1, 54, 6+0x80, 9,

      /* kvb2 80 */
      0, 1, 56, 4, 3, 57, 2, 5, 58, 6, 7, 59, 2, 3, 60, 4, 1, 61, 6, 7, 62,
      2, 5, 63, 6, 1, 64, 4, 7, 65, 6, 5, 66, 4, 1, 67, 2, 3, 68, 6, 5, 69,
      2, 7, 70, 4, 1, 71, 2, 5, 72, 4, 3, 73, 6, 7, 74, 2, 1, 75, 6, 3, 76,
      4, 5, 77, 6, 7, 78, 2, 3, 79, 4, 1, 80, 2, 7, 81, 4+0x80, 9,

      /* kvb3 62 */
      0, 1, 83, 2, 3, 84, 4, 5, 85, 6, 7, 86, 4, 3, 87, 2, 1, 88, 6, 7, 89,
      2, 5, 90, 6, 1, 91, 4, 7, 92, 6, 5, 93, 4, 1, 94, 2, 3, 95, 6, 5, 96,
      2, 7, 97, 4, 1, 98, 6, 5, 99, 4, 3, 100, 2, 7, 101, 4, 1, 102, 2+0x80, 9
    };
    static constexpr std::array<uInt32, SongStartSize> ourSongStart = {
      /* kvshared */
      44,          /* Welcome + intro Berenstain Bears */
      980829,      /* boulders in the road */
      1178398,     /* standing ovations */
      1430063,     /* brother bear */
      1691136,     /* good work */
      1841665,     /* crossing a bridge */
      2100386,     /* not bad (applause) */
      2283843,     /* ourgame */
      2629588,     /* start the parade */
      2824805,     /* rewind */
      3059116,

      /* kvs1 */
      44,          /* Harmony intro 1 */
      164685,      /* falling notes (intro 2) */
      395182,      /* instructions */
      750335,      /* high notes are high */
      962016,      /* my hat's off to you */
      1204273,     /* 1 2 3 do re mi */
      1538258,     /* Harmony */
      1801683,     /* congratulations (all of the Smurfs voted) */
      2086276,     /* line or space */
      2399093,     /* hooray */
      2589606,     /* hear yeeh */
      2801287,     /* over the river */
      3111752,     /* musical deduction */
      3436329,

      /* kvs2 */
      44,          /* Handy intro + instructions */
      778557,      /* place in shape */
      1100782,     /* sailor mate + whistle */
      //  1281887,
      1293648,     /* attention */
      1493569,     /* colours */
      1801682,     /* congratulations (Handy and friends voted) */
      2086275,

      /* kvs3 */
      44,          /* Greedy and Clumsy intro + instructions */
      686829,      /* red */
      893806,      /* don't count your chicken */
      1143119,     /* yellow */
      1385376,     /* thank you */
      1578241,     /* mixin' and matchin' */
      1942802,     /* fun / colour shake */
      2168595,     /* colours can be usefull */
      2493172,     /* hip hip horay */
      2662517,     /* green */
      3022374,     /* purple */
      3229351,     /* white */
      3720920,

      /* kvb1 */
      44,          /* 3 */ // can be one too late!
      592749,      /* 5 */
      936142,      /* 2 */
      1465343,     /* 4 */
      1787568,     /* 1 */
      2145073,     /* 7 */
      2568434,     /* 9 */
      2822451,     /* 8 */
      3045892,     /* 6 */
      3709157,     /* 0 */
      4219542,

      /* kvb2 */
      44,          /* A */
      303453,      /* B */
      703294,      /* C */
      1150175,     /* D */
      1514736,     /* E */
      2208577,     /* F */
      2511986,     /* G */
      2864787,     /* H */
      3306964,     /* I */
      3864389,     /* J */
      4148982,     /* K */
      4499431,     /* L */
      4824008,     /* M */
      5162697,     /* N */
      5581354,     /* O */
      5844779,     /* P */
      6162300,     /* Q */
      6590365,     /* R */
      6839678,     /* S */
      7225407,     /* T */
      7552336,     /* U */
      7867505,     /* V */
      8316738,     /* W */
      8608387,     /* X */
      8940020,     /* Y */
      9274005,     /* Z */
      9593878,

      /* kvb3 */
      44,          /* cat */
      341085,      /* one */
      653902,      /* red */
      1018463,     /* two */
      1265424,     /* dog */
      1669969,     /* six */
      1919282,     /* hat */
      2227395,     /* ten */
      2535508,     /* mom */
      3057653,     /* dad */
      3375174,     /* ball */
      3704455,     /* fish */
      4092536,     /* nine */
      4487673,     /* bear */
      5026282,     /* four */
      5416715,     /* bird */
      5670732,     /* tree */
      6225805,     /* rock */
      6736190,     /* book */
      7110159,     /* road */
      7676992
    };

  private:
    // Following constructors and assignment operators not supported
    KidVid() = delete;
    KidVid(const KidVid&) = delete;
    KidVid(KidVid&&) = delete;
    KidVid& operator=(const KidVid&) = delete;
    KidVid& operator=(KidVid&&) = delete;
};

#endif  // KIDVID_HXX
