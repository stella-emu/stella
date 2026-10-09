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

#ifndef SPEAKJET_DSP_HXX
#define SPEAKJET_DSP_HXX

#include "bspf.hxx"

/**
  What the software SpeakJet does to recorded sound: stretching, pitching,
  splicing and limiting it.  Nothing here keeps any state.

  @author  Stephen Anthony
*/
namespace SpeakJetDSP {

  using Samples = vector<Int16>;

  // A stretch of a sound with one Pitch and Volume in force, ending at 'end'
  struct Span {
    size_t end{0};
    double pitch{1.0};
    double gain{1.0};
  };

  // A pitch ratio this close to 1 is left alone: shifting costs more than it gains
  constexpr double PITCH_DEADBAND = 0.05;

  /**
    Stretch or compress without changing pitch, by overlap-adding frames
    at the position that best continues what came before (WSOLA).

    @param in        The sound to scale
    @param factor    Duration multiplier; below 1 shortens
    @param rate      Sample rate of 'in'
    @param pinEnd    End exactly where 'in' does, since a decay follows
    @param sourceF0  The sound's fundamental, which sizes the phase search
  */
  Samples timeScale(sShortSpan in, double factor, uInt32 rate, bool pinEnd,
                    double sourceF0);

  /**
    Shift pitch without moving the formants, by re-spacing whole pitch
    periods (TD-PSOLA), in one pass over every span of the sound.

    @param in        The sound to shift; must be voiced to mean anything
    @param spans     Where each stretch ends and its ratio of new fundamental
                     to old; above 1 raises the pitch
    @param rate      Sample rate of 'in'
    @param sourceF0  The sound's fundamental
  */
  Samples pitchShift(sShortSpan in, SpanOf<Span> spans, uInt32 rate,
                     double sourceF0);

  /**
    Mix the end of one piece into the start of the next over the whole of
    both, giving back the level a fade loses where the two differ.
  */
  Samples crossfade(sShortSpan a, sShortSpan b);

  /**
    Where in a new piece the join should fall to continue 'tail' in phase,
    searched over half a pitch period.

    @param over  How much of each side the join overlaps
  */
  size_t alignPhase(sShortSpan tail, sShortSpan piece, size_t over,
                    uInt32 rate, double f0);

  /**
    Join one part of a sustained sound onto the next, mid-sound.
  */
  void sustain(Samples& out, sShortSpan piece, uInt32 rate, double f0);

  /**
    Convert between sample rates by linear interpolation, in place.
  */
  void resample(Samples& samples, uInt32 from, uInt32 to);

  /**
    A sample value as 16 bits, easing peaks above LIMIT_KNEE into full
    scale rather than clipping them.
  */
  Int16 limit(double v);

}  // namespace SpeakJetDSP

#endif  // SPEAKJET_DSP_HXX
