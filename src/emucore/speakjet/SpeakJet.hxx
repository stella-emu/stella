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

#ifndef SPEAKJET_HXX
#define SPEAKJET_HXX

#include "bspf.hxx"

/**
  Decodes the Magnevation SpeakJet command stream.

  The chip is driven by a byte stream: codes 128-199 are allophones,
  200-254 sound effects, 0-31 control codes (some taking a second byte),
  and 255 ends a phrase.  This class turns that stream into the sounds and
  parameter changes it describes, and hands them to a Sink to be rendered.

  Reference: Magnevation SpeakJet User's Manual rev 1.1, Tables D and E.

  @author  Stephen Anthony
*/
class SpeakJet
{
  public:
    // Power-on defaults, from the manual's control code descriptions
    static constexpr uInt8 DEFAULT_VOLUME = 96;
    static constexpr uInt8 DEFAULT_SPEED = 114;
    static constexpr uInt8 DEFAULT_PITCH = 88;
    static constexpr uInt8 DEFAULT_BEND = 5;

    // The highest Bend the manual allows
    static constexpr uInt8 MAX_BEND = 15;

    // The MSA codes: allophones up to LAST_ALLOPHONE, then sound effects
    static constexpr uInt8 FIRST_SOUND = 128;
    static constexpr uInt8 LAST_ALLOPHONE = 199;
    static constexpr uInt8 LAST_SOUND = 254;

    enum class Param: uInt8 { Volume, Speed, Pitch, Bend };

    /**
      One sound to render, either an allophone or a sound effect.
    */
    struct Utterance
    {
      // MSA code, 128-254
      uInt8 code{0};
      // Mnemonic from Table E, e.g. "IY" or "M1"; also the sample's name
      string_view name;
      // Table E's duration, scaled by any Fast or Slow; only good for silence
      uInt32 durationMs{0};

      // What Fast (7) or Slow (8) asks of the sound's length, in percent
      uInt32 scalePct{100};
      // Set by a preceding Stress (14) or Relax (15) code
      bool stress{false};
      bool relax{false};
    };

    /**
      Receives what the decoder makes of the command stream.
    */
    class Sink
    {
      public:
        Sink() = default;
        virtual ~Sink() = default;

        virtual void play(const Utterance& utterance) = 0;
        virtual void pause(uInt32 durationMs) = 0;
        // Silence of a fixed length, which Speed does not scale
        virtual void delay(uInt32 durationMs) = 0;
        virtual void setParam(Param param, uInt8 value) = 0;
        virtual void endOfPhrase() { }

      private:
        Sink(const Sink&) = delete;
        Sink(Sink&&) = delete;
        Sink& operator=(const Sink&) = delete;
        Sink& operator=(Sink&&) = delete;
    };

  public:
    explicit SpeakJet(Sink& sink);
    ~SpeakJet() = default;

    /**
      Feed one byte of the command stream.

      @param code  The byte received from the controller pins
    */
    void write(uInt8 code);

    /**
      Restore the power-on parameter defaults and drop any pending state.
    */
    void reset();

    /**
      Look up an MSA code's mnemonic, or an empty view if it has none.
    */
    static string_view nameOf(uInt8 code);

    /**
      Whether a code produces voiced (periodic) sound, which is all that
      Pitch reaches.
    */
    static bool isVoiced(uInt8 code);

    /**
      Whether a code is a vowel, nasal, resonant, R-colour vowel or
      diphthong, which sound steadily throughout.
    */
    static bool isContinuant(uInt8 code);

  private:
    // Codes 20-30 take a value byte; this is the one we are waiting on
    enum class Pending: uInt8 { None, Volume, Speed, Pitch, Bend, Repeat,
                                Delay, Ignored };

  private:
    Sink& mySink;

    // Which two-byte command is waiting for its value
    Pending myPending{Pending::None};

    // How many times the next sound is to be played; Repeat sets it
    uInt8 myRepeat{1};

    // Modifiers that apply only to the next sound
    bool myStress{false};
    bool myRelax{false};

    // Duration scaling from Fast (7) or Slow (8), as a percentage
    uInt32 myNextScale{100};

  private:
    SpeakJet() = delete;
    SpeakJet(const SpeakJet&) = delete;
    SpeakJet(SpeakJet&&) = delete;
    SpeakJet& operator=(const SpeakJet&) = delete;
    SpeakJet& operator=(SpeakJet&&) = delete;
};

#endif  // SPEAKJET_HXX
