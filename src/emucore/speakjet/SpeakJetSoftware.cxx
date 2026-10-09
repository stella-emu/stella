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

#include <cmath>

#include "FSNode.hxx"
#include "OSystem.hxx"
#include "Sound.hxx"
#include "SpeakJetTables.hxx"
#include "SpeakJetSoftware.hxx"

namespace {
  // Splice length between consecutive sounds: only enough to stop a hard
  // join clicking, never the chip's own overlap
  constexpr uInt32 XFADE_MS = 3;

  // and never more than this fraction of the sound being spliced
  constexpr size_t XFADE_DIVISOR = 8;

  // READY drops while this much audio is still queued, standing in for the
  // chip's 64-byte input buffer
  constexpr uInt32 READY_MS = 250;

  // Sounds shorter than this are left alone; WSOLA needs room to work
  constexpr uInt32 MIN_SCALE_MS = 12;

  // Ignore scaling this close to 1, it is not worth the artefacts
  constexpr double SCALE_DEADBAND = 0.02;

  // While idle, keep this much of a decay queued so it plays without a break
  constexpr uInt32 DECAY_LEAD_MS = 40;

  // Silence queued ahead of a phrase, so the queue does not run dry while a
  // sound is held back for the next byte
  constexpr uInt32 PRIME_MS = 50;

  // A clip's pitch is unsteady this long past its level ramp, so a join into
  // a voiced continuant trims that too
  constexpr uInt32 JOIN_HEAD_MS = 20;

  // How far a sound may be time-scaled to take up the change at a join
  constexpr double JOIN_MIN_SCALE = 0.5;
  constexpr double JOIN_MAX_SCALE = 2.0;

  // A Speed sent after a sound already plays this much of that sound's end
  constexpr double SPEED_TAIL_MS = 17.0;

  // Out of a vowel into a nasal or liquid the chip moves on this much sooner
  // than the join alone says, the total unchanged
  constexpr double NASAL_LEAD_MS = 24.0;

  // How much of what was last queued the next sound is aligned to
  constexpr uInt32 ALIGN_CONTEXT_MS = 30;

  // A vowel running straight into a nasal or liquid
  constexpr bool leadsIn(int from, int to)
  {
    return ((from >= 128 && from <= 139) || (from >= 149 && from <= 164)) &&
           to >= 140 && to <= 148;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSoftware::SpeakJetSoftware(const OSystem& osystem, string_view deadPort)
  : myOSystem{osystem},
    myDecoder{*this},
    mySamples{osystem.baseDir().getPath() + "speakjet" + FSNode::PATH_SEPARATOR},
    myVoice{mySamples}
{
  const string& path = mySamples.path();

  if(mySamples.found() == 0)
    myAboutString = " (software SpeakJet, no samples in \'" + path + "\')";
  else
    myAboutString = " (software SpeakJet, " + std::to_string(mySamples.found()) +
                    "/127 samples in \'" + path + "\'" +
                    (mySamples.perBend() ? ", per-Bend" : "") +
                    (mySamples.perPitch() ? ", per-Pitch" : "") + ")";

  // Say why, when this is a fallback rather than a choice
  if(!deadPort.empty())
    myAboutString += " [\'" + string{deadPort} + "\' unavailable]";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSoftware::~SpeakJetSoftware() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSoftware::ready()
{
  return myOSystem.sound().speechQueued() < READY_MS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::reset()
{
  myOSystem.sound().stopSpeech();
  myTail.clear();
  myPendingSound.clear();
  myPendingDecay.clear();
  myDecay.clear();
  myVoice.reset();
  myPhrase.clear();
  myAfterPause = false;
  myFlowing = false;
  myStarting = true;
  myDecoder.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::update()
{
  ++myFrame;

  // Two frames of nothing ends the phrase, so the tail is no longer held back
  // for a join; only frames the ROM could have sent in count
  if((!myTail.empty() || !myPendingSound.empty()) && ready() &&
     ++myIdleFrames >= 2)
  {
    releasePending(-1);
    flush();
    showPhrase();
    // Nothing follows, so no closure
    myAfterPause = false;
    myStarting = true;
  }

  // Nothing follows, so the decay plays out, fed a little at a time so a
  // sound arriving meanwhile is not held up behind all of it
  if(myPendingSound.empty() && !myDecay.empty() &&
     myOSystem.sound().speechQueued() < DECAY_LEAD_MS)
  {
    myKind = "decay";
    myTraceCode = -1;
    append(takeDecay(rate() * DECAY_LEAD_MS / 1000), 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::play(const SpeakJet::Utterance& utterance)
{
  if(myShowPhrases)
  {
    if(!myPhrase.empty())
      myPhrase += ' ';
    myPhrase += SpeakJet::nameOf(utterance.code);
  }

  // A pause just before this sound, which the block below consumes
  const bool afterPause = myAfterPause;

  // Pitch reaches only voiced sounds
  const bool voiced = SpeakJet::isVoiced(utterance.code);
  const auto index = SZT(utterance.code - SpeakJet::FIRST_SOUND);

  // A sound recorded at the Pitch in force needs none of the pitch laws, only
  // the little left over to the nearest sampled setting
  uInt8 atPitch = SpeakJet::DEFAULT_PITCH;
  const Clip* sampled = voiced
      ? mySamples.pitchClip(utterance.code, myPitch, myBend, atPitch) : nullptr;
  const Clip* chosen = sampled ? sampled : &mySamples.setFor(myBend)[index];

  // Off the default Bend a sound rings on longer, which inside a phrase is
  // decay; it is held to the default Bend's length at the same Pitch
  Clip capped;
  if(myBend != SpeakJet::DEFAULT_BEND && utterance.code <= SpeakJet::LAST_ALLOPHONE)
  {
    const Clip& ref = sampled ? mySamples.pitchSet(SpeakJet::DEFAULT_BEND, atPitch)[index]
                              : mySamples.setFor(SpeakJet::DEFAULT_BEND)[index];
    const auto keep = ref.body.size();

    if(keep > 0 && chosen->body.size() > keep)
    {
      capped.body.assign(chosen->body.begin(), chosen->body.begin() + I64(keep));
      capped.decay.assign(chosen->body.begin() + I64(keep), chosen->body.end());
      capped.decay.insert(capped.decay.end(), chosen->decay.begin(), chosen->decay.end());
      SpeakJetSamples::findRamps(capped, rate());
      chosen = &capped;
    }
  }
  const Clip& clip = *chosen;

  // A missing sample still takes its documented time
  if(clip.body.empty())
  {
    pause(utterance.durationMs);
    return;
  }

  // Speed, Fast and Slow stretch the same part of a sound and leave the rest
  const double lenMs = DBL(clip.body.size()) * 1000.0 / DBL(rate());
  const double fixedMs = BSPF::clamp(SpeakJetTables::fixedMs(utterance.code),
                                     -lenMs / 2, lenMs / 2);
  double targetMs = fixedMs + (lenMs - fixedMs) *
      DBL(utterance.scalePct) / 100.0 * SpeakJetTables::speedFactor(mySpeed);

  // Below the default Pitch a made sound keeps its length there, whatever
  // Pitch set it is played from
  const bool held = myPitch <= SpeakJet::DEFAULT_PITCH && SpeakJetVoice::makes(utterance.code);
  if(held && sampled)
  {
    const Clip& base = mySamples.pitchSet(SpeakJet::DEFAULT_BEND, SpeakJet::DEFAULT_PITCH)[index];
    const Clip& set = mySamples.pitchSet(SpeakJet::DEFAULT_BEND, atPitch)[index];
    if(!base.body.empty() && !set.body.empty())
      targetMs *= DBL(base.body.size()) / DBL(set.body.size());
  }
  // and any other voiced sound by the law for the setting, less whatever the
  // recording already has
  else if(voiced && !held)
    targetMs *= SpeakJetTables::phraseDuration(myPitch) /
                SpeakJetTables::pitchDuration(atPitch);

  double gain = SpeakJetTables::volumeFactor(myVolume) * SpeakJetTables::SPEECH_GAIN;
  if(voiced)
    gain *= SpeakJetTables::pitchGain(myPitch, atPitch);

  // A1, A2, A3 and M2 are trains of voicing pulses, which follow Pitch too
  const bool pulsed = utterance.code == 211 || utterance.code == 212 ||
                      utterance.code == 213 || utterance.code == 254;
  const double pitch = voiced || pulsed
      ? SpeakJetTables::pitchFactor(myPitch) / SpeakJetTables::pitchFactor(atPitch) : 1.0;

  // The same sound again is not started over but keeps sounding, so it
  // becomes another part of the pending sound, spliced mid-sound
  if(!myPendingSound.empty() && utterance.code == myPendingCode && !myAfterPause)
  {
    const double timing = DBL(utterance.scalePct) / 100.0 *
                          SpeakJetTables::speedFactor(mySpeed);
    double addMs = 0.0;

    // What a copy adds where it is known, else the sound's length and merge
    if(const auto repeatMs = SpeakJetTables::repeatMs(utterance.code))
    {
      addMs = *repeatMs * timing;
      if(voiced && !held)
        addMs *= SpeakJetTables::phraseDuration(myPitch);
    }
    else
      addMs = targetMs +
              SpeakJetTables::joinMs(utterance.code, utterance.code) * timing;

    myPendingParts.push_back({addMs, pitch, gain, myPitch, myVolume});
    myIdleFrames = 0;
    return;
  }

  Samples out{clip.body.begin(), clip.body.end()};
  Samples decay{clip.decay.begin(), clip.decay.end()};

  // Something follows the previous sound, so the join between them applies;
  // length and pitch are applied on release, in one pass
  const double timing = DBL(utterance.scalePct) / 100.0 *
                        SpeakJetTables::speedFactor(mySpeed);
  const bool lead = !myPendingSound.empty() && leadsIn(myPendingCode, utterance.code);
  releasePending(utterance.code, timing);
  if(lead)
    targetMs += NASAL_LEAD_MS * timing;

  // A pause does not take away the closure of a stop that follows it
  if(myAfterPause)
  {
    const auto ms = U32(SpeakJetTables::closureMs(utterance.code) * timing);
    if(ms > 0)
    {
      const size_t length = rate() * (ms + XFADE_MS) / 1000;
      const bool decaying = !myDecay.empty();
      Samples gap = takeDecay(length);

      gap.resize(length, 0);
      myKind = "closure";
      myTraceCode = utterance.code;
      append(gap, decaying ? 0 : myNextXfadeMs);
      myNextXfadeMs = XFADE_MS;

      // Silence stands between them, so this sound starts speech again
      myFlowing = false;
    }
    myAfterPause = false;
  }

  // The chip moves straight on to this one, ending any decay
  myDecay.clear();

  // Between two sounds a quiet code takes time its clip does not have
  if(myFlowing && !afterPause && SpeakJetTables::quietPadMs(utterance.code) > 0)
  {
    const double padMs = DBL(SpeakJetTables::quietPadMs(utterance.code)) *
                         DBL(utterance.scalePct) / 100.0 *
                         SpeakJetTables::speedFactor(mySpeed);

    out.insert(out.end(), SZT(DBL(rate()) * padMs / 1000.0), 0);
    targetMs += padMs;
  }

  myPendingSound = std::move(out);
  myPendingDecay = std::move(decay);
  myPendingCode = utterance.code;
  myPendingHead = clip.head;
  myPendingTail = clip.tail;
  myPendingF0 = DBL(atPitch);
  myPendingTrimHead = myFlowing;
  myPendingSpeed = mySpeed;
  myPendingBend = myBend;
  myPendingParts.assign(1, {targetMs, pitch, gain, myPitch, myVolume});
  myIdleFrames = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::releasePending(int nextCode, double nextTiming)
{
  if(myPendingSound.empty())
    return;

  const Samples sample = std::move(myPendingSound);
  myPendingSound.clear();

  double extraMs = 0.0;
  double closureMs = 0.0;
  if(nextCode >= 0 && myPendingCode >= 0)
  {
    // A join takes its timing from the sound coming in; the splice overlaps
    // the two by XFADE_MS, so that is given back
    extraMs = SpeakJetTables::joinMs(U8(myPendingCode), U8(nextCode)) * nextTiming +
              DBL(XFADE_MS);
    if(leadsIn(myPendingCode, nextCode))
      extraMs -= NASAL_LEAD_MS * nextTiming;

    // A stop has a closure, even after a voiceless fricative
    closureMs = BSPF::clamp(extraMs, 0.0,
                            SpeakJetTables::closureMs(U8(nextCode)) * nextTiming);
    extraMs -= closureMs;
  }

  // At a join the next sound takes over the oscillators, so this one's decay
  // goes, except through a stop's closure, which it fills
  if(nextCode >= 0 && closureMs <= 0.0)
    myPendingDecay.clear();

  // The rise and fall a recording carries come off wherever this sound is not
  // at an end of the speech, its duration unchanged
  const bool trimTail = nextCode >= 0 && closureMs <= 0.0;

  // A repeat either runs the articulation again or the chip holds the sound
  const bool held = SpeakJetTables::holdsRepeat(U8(myPendingCode));

  // Voicing runs on out of any allophone but a stop or an affricate, which
  // release with an attack of their own
  const bool release = myPrevCode == 165 || (myPrevCode >= 170 && myPrevCode <= 182) ||
                       myPrevCode >= 191;
  const bool prevOk = myPrevCode >= 128 && !release;
  const bool joinsOn = myPendingTrimHead && prevOk && myPendingCode >= 128 &&
                       SpeakJet::isContinuant(U8(myPendingCode));
  const int thisCode = myPendingCode;

  if(SpeakJetVoice::makes(thisCode))
  {
    vector<Part> parts{myPendingParts};
    myPendingParts.clear();

    // The join belongs to the end of the whole sound
    Part& last = parts.back();
    if(last.targetMs > 0.0)
      last.targetMs = BSPF::clamp(last.targetMs + extraMs, last.targetMs * JOIN_MIN_SCALE,
                                  last.targetMs * JOIN_MAX_SCALE);

    // Voicing carries on into the next sound only if it follows at once, and
    // after a voiceless stop or an affricate starts from nothing
    const bool onward = nextCode >= 0 && closureMs <= 0.0 && SpeakJetVoice::makes(nextCode);
    if(!myPendingTrimHead)
      myVoice.stop();
    const bool continued = myVoice.live();
    const bool cold = !myPendingTrimHead || myPrevCode == 165 || myPrevCode == 182 ||
                      (myPrevCode >= 191 && myPrevCode <= 199);
    const Samples out = myVoice.speak(thisCode, parts, myPendingBend, myPendingSpeed,
                                      cold, !onward, nextCode);

    const double gain = SpeakJetTables::volumeFactor(parts.back().volume) *
                        SpeakJetTables::SPEECH_GAIN;
    for(Int16& v: myPendingDecay)
      v = SpeakJetDSP::limit(DBL(v) * gain);
    myDecay = std::move(myPendingDecay);
    myPendingDecay.clear();

    myKind = "sound";
    myTraceCode = thisCode;
    append(out, continued ? 0 : myNextXfadeMs);
    myPrevCode = thisCode;
    myNextXfadeMs = onward ? 0 : XFADE_MS;
    myFlowing = trimTail;
    playClosure(closureMs, nextCode);
    return;
  }

  // Only the first and last parts sit at an end of the speech, so only they
  // can keep a ramp
  size_t headTrim = myPendingTrimHead ? myPendingHead : 0;
  if(joinsOn)
    headTrim = std::max(headTrim, std::min(size_t{rate() * JOIN_HEAD_MS / 1000},
                                           sample.size() / SpeakJetSamples::RAMP_DIVISOR));
  const size_t tailTrim = trimTail ? myPendingTail : 0;

  // The parts are laid end to end first, then scaled and pitched in one pass,
  // so the pitch lattice runs on through a repeat as the chip's oscillator does
  Samples out;
  vector<SpeakJetDSP::Span> spans;
  vector<double> shares;
  double targetMs = 0.0;

  for(size_t i = 0; i < myPendingParts.size(); ++i)
  {
    const Part& part = myPendingParts[i];
    const bool last = i + 1 == myPendingParts.size();
    double target = part.targetMs;

    // The join belongs to the end of the whole sound, so only the last part
    if(last && target > 0.0)
      target = BSPF::clamp(target + extraMs, target * JOIN_MIN_SCALE,
                           target * JOIN_MAX_SCALE);
    targetMs += target;
    shares.push_back(target);

    if(out.empty())
      out.assign(sample.begin() + I32(headTrim),
                 sample.end() - I32(held ? tailTrim : (last ? tailTrim
                                                            : myPendingTail)));
    else if(!held)
      SpeakJetDSP::sustain(out, {sample.begin() + I32(myPendingHead),
                                 sample.end() - I32(last ? tailTrim : myPendingTail)},
                           rate(), myPendingF0);

    spans.push_back({out.size(), part.pitch, part.gain});
  }
  myPendingParts.clear();

  // A held sound is one piece however many copies were written, so each part
  // takes its share of it by time
  if(held && spans.size() > 1 && targetMs > 0.0)
  {
    double at = 0.0;

    for(size_t i = 0; i < spans.size(); ++i)
    {
      at += shares[i];
      spans[i].end = SZT(DBL(out.size()) * at / targetMs);
    }
    spans.back().end = out.size();
  }

  const double naturalMs = DBL(out.size()) * 1000.0 / DBL(rate());
  const double factor = naturalMs > 0.0 ? targetMs / naturalMs : 1.0;

  if(std::abs(factor - 1.0) >= SCALE_DEADBAND &&
     out.size() * 1000 / rate() >= MIN_SCALE_MS)
  {
    out = SpeakJetDSP::timeScale(out, factor, rate(), !myPendingDecay.empty(),
                                 myPendingF0);
    for(SpeakJetDSP::Span& span: spans)
      span.end = std::min(out.size(), SZT(DBL(span.end) * factor));
  }
  spans.back().end = out.size();

  // Pitch after the time scale, the decay with it so the phase carries across
  const size_t split = out.size();

  out.insert(out.end(), myPendingDecay.begin(), myPendingDecay.end());
  spans.back().end = out.size();
  out = SpeakJetDSP::pitchShift(out, spans, rate(), myPendingF0);
  myPendingDecay.assign(out.begin() + I32(split), out.end());
  out.resize(split);

  size_t at = 0;
  for(const SpeakJetDSP::Span& span: spans)
  {
    for(size_t i = at; i < std::min(span.end, out.size()); ++i)
      out[i] = SpeakJetDSP::limit(DBL(out[i]) * span.gain);
    at = span.end;
  }
  for(Int16& v: myPendingDecay)
    v = SpeakJetDSP::limit(DBL(v) * spans.back().gain);

  myDecay = std::move(myPendingDecay);
  myPendingDecay.clear();

  myKind = "sound";
  myTraceCode = myPendingCode;
  myAlignNext = joinsOn;
  append(out, myNextXfadeMs);
  myAlignNext = false;
  myPrevCode = thisCode;
  myNextXfadeMs = XFADE_MS;
  myFlowing = trimTail;
  myVoice.carryFrom(SpeakJetVoice::carries(thisCode) && closureMs <= 0.0 &&
                    SpeakJetVoice::makes(nextCode) ? thisCode : -1);

  playClosure(closureMs, nextCode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::showPhrase()
{
  if(!myShowPhrases || myPhrase.empty())
    return;

  // A trailing pause says nothing
  if(myPhrase.back() == ',')
    myPhrase.pop_back();
  cerr << "SpeakJet: " << myPhrase << '\n';
  myPhrase.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::playClosure(double closureMs, int nextCode)
{
  // The closure straight on from the sound, plus the overlap its own splice
  // takes back; the next sound then ends what is left
  if(closureMs > 0.0)
  {
    const size_t length = rate() * (U32(closureMs) + XFADE_MS) / 1000;
    const bool decaying = !myDecay.empty();
    Samples gap = takeDecay(length);
    gap.resize(length, 0);
    myKind = "closure";
    myTraceCode = nextCode;
    append(gap, decaying ? 0 : XFADE_MS);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::pause(uInt32 durationMs)
{
  // Speed scales a pause as it does a sound
  delay(U32(DBL(durationMs) * SpeakJetTables::speedFactor(mySpeed) + 0.5));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::delay(uInt32 durationMs)
{
  // A pause is not a join, so the sound before it keeps its full length, and
  // even one of no length separates the two sounds
  releasePending(-1);
  myAfterPause = true;
  myVoice.carryFrom(-1);
  if(myShowPhrases && !myPhrase.empty() && myPhrase.back() != ',')
    myPhrase += ',';

  if(rate() == 0 || durationMs == 0)
    return;

  // The decay runs on into the pause, straight on from its own sound
  const size_t length = rate() * durationMs / 1000;
  const bool decaying = !myDecay.empty();
  Samples gap = takeDecay(length);
  gap.resize(length, 0);

  myKind = "pause";
  myTraceCode = -1;
  append(gap, decaying ? 0 : myNextXfadeMs);
  myIdleFrames = 0;
  myNextXfadeMs = XFADE_MS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::setParam(SpeakJet::Param param, uInt8 value)
{
  switch(param)
  {
    case SpeakJet::Param::Volume:  myVolume = value;  break;
    case SpeakJet::Param::Speed:
      // The chip reads the next code before a sound ends, so a new Speed
      // already plays the end of the sound before it
      if(myPendingCode >= 0 && !myPendingParts.empty())
        myPendingParts.back().targetMs += SPEED_TAIL_MS *
            (SpeakJetTables::speedFactor(value) / SpeakJetTables::speedFactor(mySpeed) - 1.0);
      mySpeed = value;
      break;
    case SpeakJet::Param::Pitch:   myPitch = value;   break;
    case SpeakJet::Param::Bend:
      // Higher would index past every per-Bend table
      myBend = std::min(value, SpeakJet::MAX_BEND);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::append(sShortSpan pcm, uInt32 xfadeMs)
{
  if(pcm.empty() || rate() == 0)
    return;

  if(myTrace)
    cerr << "SJT " << myFrame << ' ' << myKind << ' ' << myTraceCode << ' '
         << (myQueued * 1000 / rate()) << ' '
         << (pcm.size() * 1000 / rate()) << '\n';
  myQueued += pcm.size();

  // A phrase starting with nothing queued gets a cushion first; a ROM stalling
  // mid-phrase must not, or silence drops into a word
  if(myStarting && myOSystem.sound().speechQueued() == 0)
  {
    const Samples lead(rate() * PRIME_MS / 1000, 0);

    myOSystem.sound().queueSpeech(lead, rate());
    myStarting = false;
  }

  // Start the new sound at the phase the last one ended at, within half a
  // period, so the fundamental does not stumble at the join
  if(myAlignNext && myContext.size() >= 64 && myPendingF0 > 0.0)
  {
    const size_t period = SZT(DBL(rate()) / myPendingF0 + 0.5);
    const size_t n = std::min(myContext.size(), 2 * period);
    const size_t reach = std::min(period / 2 + 1, pcm.size() > n + 8 ? pcm.size() - n - 8 : 0);
    const Int16* a = myContext.data() + (myContext.size() - n);
    double best = -1e30;
    size_t at = 0;

    for(size_t off = 0; off <= reach; ++off)
    {
      double dot = 0.0, energy = 1.0;

      for(size_t i = 0; i < n; i += 2)
      {
        dot += DBL(a[i]) * DBL(pcm[off + i]);
        energy += DBL(pcm[off + i]) * DBL(pcm[off + i]);
      }

      const double score = dot / std::sqrt(energy);
      if(score > best)
      {
        best = score;
        at = off;
      }
    }
    pcm = pcm.subspan(at);
  }

  // Short sounds get a proportionally shorter fade, or they are eaten
  const size_t xfade = std::min(size_t{rate() * xfadeMs / 1000},
                                pcm.size() / XFADE_DIVISOR);
  const size_t over = std::min({xfade, myTail.size(), pcm.size()});
  Samples out;

  out.reserve(myTail.size() + pcm.size());

  // Whatever of the tail the new sound does not overlap goes out as it is
  out.insert(out.end(), myTail.begin(), myTail.end() - over);

  const Samples mixed = SpeakJetDSP::crossfade({myTail.end() - I32(over), over},
                                               {pcm.begin(), over});

  out.insert(out.end(), mixed.begin(), mixed.end());

  // Hold back the end of this sound for the next crossfade
  const size_t keep = std::min({size_t{rate() * XFADE_MS / 1000},
                                pcm.size() / XFADE_DIVISOR,
                                pcm.size() - over});

  out.insert(out.end(), pcm.begin() + I32(over), pcm.end() - I32(keep));
  myTail.assign(pcm.end() - I32(keep), pcm.end());

  // What was last queued, for aligning the next sound
  {
    const size_t want = rate() * ALIGN_CONTEXT_MS / 1000;
    Samples both{out};
    both.insert(both.end(), myTail.begin(), myTail.end());
    myContext.assign(both.end() - I32(std::min(want, both.size())), both.end());
  }

  myOSystem.sound().queueSpeech(out, rate());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSoftware::flush()
{
  if(myTail.empty())
    return;

  myQueued += myTail.size();
  myOSystem.sound().queueSpeech(myTail, rate());
  myTail.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSoftware::Samples SpeakJetSoftware::takeDecay(size_t count)
{
  const auto n = I32(std::min(count, myDecay.size()));
  Samples out{myDecay.begin(), myDecay.begin() + n};

  myDecay.erase(myDecay.begin(), myDecay.begin() + n);
  return out;
}
