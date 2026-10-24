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

#include "SpeakJetChip.hxx"
#include "SpeakJetTables.hxx"
#include "SpeakJetMSA.hxx"

namespace {
  using namespace SpeakJetChip;
  using Registers = SpeakJetSynth::Registers;

  // Levels are measured after a Reset, at its master volume: what one volume
  // step gives, of a plain sine and of a grain
  constexpr double FIT_MASTER = SpeakJet::INITIAL_VOLUME;
  constexpr double PLAIN_VOLUME = 127.0 / (VOLUME_STEP * FIT_MASTER);
  constexpr double GRAIN_VOLUME = PLAIN_VOLUME * ENVELOPE_PEAK;

  // Where in a sound each state is measured, as a share of its length: a
  // glide's onset, then the start and end states
  constexpr double ONSET_AT = 0.15, START_AT = 0.3, END_AT = 0.75;

  // Under this share of its state's loudest, an oscillator is all but silent
  constexpr double QUIET_LEVEL = 0.1;

  // JH, VV, ZZ and ZH: made as the continuants are, plus their noise
  constexpr int FIRST_VOICED_FRICATIVE = 165, LAST_VOICED_FRICATIVE = 168;

  // DH and the voiced stops: a voice bar, then their release
  constexpr int FIRST_VOICED_STOP = 169, LAST_VOICED_STOP = 181;

  // A stop's voice bar is oscillator 1 alone near 266Hz, a final stop's fading
  // at about 0.17; the release takes over half way (BE, DE: three periods each)
  constexpr double VOICE_BAR_HZ = 266.0;
  constexpr double FINAL_BAR_LEVEL = 0.17;
  constexpr double RELEASE_AT = 0.5;

  // A voiced stop starting a syllable (BE BO DE DO GE GO) or ending one
  constexpr bool initialStop(int code)
  {
    return code == 170 || code == 171 || code == 174 || code == 175 ||
           code == 178 || code == 179;
  }
  constexpr bool finalStop(int code)
  {
    return code == 172 || code == 173 || code == 176 || code == 177 ||
           code == 180 || code == 181;
  }

  // Voicing after a voiceless stop starts this long before its noise ends
  constexpr double HAND_OVER_MS = 10.0;

  // How far a join may stretch or shrink the sound before it
  constexpr double JOIN_MIN_SCALE = 0.5;
  constexpr double JOIN_MAX_SCALE = 2.0;

  // Out of a vowel into a nasal or liquid the chip moves on this much sooner
  // than the join alone says, the total unchanged
  constexpr double NASAL_LEAD_MS = 24.0;

  // A vowel running straight into a nasal or liquid
  constexpr bool leadsIn(int from, int to)
  {
    return ((from >= 128 && from <= 139) || (from >= 149 && from <= 164)) &&
           to >= 140 && to <= 148;
  }

  // Voicing after these starts from nothing: CH and the voiceless stops
  constexpr bool stopsVoicing(int code)
  {
    return code == 182 || (code >= 191 && code <= 199);
  }

  // PE and PO, a low sine with a little noise
  constexpr int PE = 198, PO = 199;

  // A noise sound ramps in over this long, and out over the second, as the
  // hardware's falls 20dB in about 10ms
  constexpr double NOISE_RAMP_MS = 5.0;
  constexpr double NOISE_END_MS = 11.0;

  // A measured state as registers in the chip's whole steps, Bend's offset on
  // every sounding oscillator, the code's noise; a negative frequency is silent
  Registers registersFor(const State& state, int code, uInt8 bend)
  {
    const double offset = BEND_HZ[bend] - BEND_HZ[SpeakJet::INITIAL_BEND];
    Registers r;
    for(size_t k = 0; k < 3; ++k)
    {
      r.hz[k] = state.hz[k] > 0.0 ? std::round(state.hz[k] + offset) : state.hz[k];
      r.volume[k] = std::min(31.0, std::round(state.level[k] * GRAIN_VOLUME));
    }
    if(code >= SpeakJet::FIRST_SOUND && code <= SpeakJet::LAST_ALLOPHONE)
    {
      const NoiseRegisters& noise = NOISE_REGISTERS[SZT(code - SpeakJet::FIRST_SOUND)];
      r.hz = {r.hz[0], r.hz[1], r.hz[2], DBL(noise.hz4), DBL(noise.hz5)};
      r.volume = {r.volume[0], r.volume[1], r.volume[2], DBL(noise.volume4), DBL(noise.volume5)};
      r.distortion = DBL(noise.distortion);
      r.noiseHalf = noise.half;
    }
    return r;
  }

  // A sound effect's registers at a whole tick of its program, sliding between
  // breakpoints, with Bend; 'envelope' gets the Hz its envelope runs over Pitch
  Registers effectRegisters(const Effect& fx, double tick, uInt8 bend, double& envelope)
  {
    const auto* first = &EFFECT_POINTS[fx.first];
    const auto* last = first + fx.count - 1;
    const auto* at = first;
    while(at < last && DBL((at + 1)->tick) <= tick)
      ++at;

    const auto* next = at < last ? at + 1 : at;
    const double p = next != at && tick > DBL(at->tick)
        ? (tick - DBL(at->tick)) / DBL(next->tick - at->tick) : 0.0;
    const double offset = BEND_UNITS * (DBL(bend) - DBL(SpeakJet::INITIAL_BEND)) +
                          (bend < SpeakJet::INITIAL_BEND ? BEND_LOW_UNITS : 0.0);
    const auto slide = [p](double from, double to) { return std::round(from + p * (to - from)); };
    Registers r;
    r.enveloped = at->envelope >= 0;
    for(size_t k = 0; k < 5; ++k)
    {
      const double hz = slide(at->hz[k], next->hz[k]) + offset;
      r.hz[k] = hz < 0.0 ? hz + REGISTER_WRAP : hz;
      r.volume[k] = slide(at->volume[k], next->volume[k]);
    }
    r.distortion = slide(at->distortion, next->distortion);
    envelope = !r.enveloped ? 0.0
             : next->envelope >= 0 ? slide(at->envelope, next->envelope) : DBL(at->envelope);
    return r;
  }

  // A sound's end state; a stop whose release is measured hands the voice on
  // from there
  State endState(int code)
  {
    State end = SOUNDS[SZT(code - SpeakJet::FIRST_SOUND)].end;
    for(const auto& release: RELEASES)
      if(release.code == code)
        end.hz = release.hz;
    return end;
  }

  // A silent or all but silent oscillator takes the other side's frequency,
  // so it does not slide
  Registers settle(Registers r, const Registers& other)
  {
    const double loudest = std::max({r.volume[0], r.volume[1], r.volume[2]});
    for(size_t k = 0; k < 3; ++k)
      if(r.hz[k] < 0.0 || r.volume[k] < QUIET_LEVEL * loudest)
      {
        r.hz[k] = other.hz[k] > 0.0 ? other.hz[k] : r.hz[k] > 0.0 ? r.hz[k] : 500.0;
        r.volume[k] = 0.0;
      }
    return r;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetMSA::SpeakJetMSA(Lookahead lookahead)
  : myLookahead{std::move(lookahead)}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetMSA::voices(int code)
{
  return ((code >= SpeakJet::FIRST_SOUND && SpeakJet::isContinuant(U8(code))) ||
          (code >= FIRST_VOICED_FRICATIVE && code <= LAST_VOICED_FRICATIVE)) &&
         SOUNDS[SZT(code - SpeakJet::FIRST_SOUND)].voiced;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetMSA::voicedStop(int code)
{
  return code >= FIRST_VOICED_STOP && code <= LAST_VOICED_STOP &&
         SOUNDS[SZT(code - SpeakJet::FIRST_SOUND)].voiced;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetMSA::voicedThrough(int code)
{
  return voices(code) || voicedStop(code);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::slewScale(uInt8 speed)
{
  for(size_t i = 1; i < SPEED_SLEW.size(); ++i)
    if(DBL(speed) <= SPEED_SLEW[i][0])
    {
      const auto& lo = SPEED_SLEW[i - 1];
      const auto& hi = SPEED_SLEW[i];
      return lo[1] + (DBL(speed) - lo[0]) / (hi[0] - lo[0]) * (hi[1] - lo[1]);
    }
  return SPEED_SLEW.back()[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::pitchHz(uInt8 setting)
{
  const double p = BSPF::clamp(DBL(setting), PITCH_HZ.front()[0], PITCH_HZ.back()[0]);
  for(size_t i = 1; i < PITCH_HZ.size(); ++i)
    if(p <= PITCH_HZ[i][0])
      return PITCH_HZ[i - 1][1] + (p - PITCH_HZ[i - 1][0]) /
             (PITCH_HZ[i][0] - PITCH_HZ[i - 1][0]) * (PITCH_HZ[i][1] - PITCH_HZ[i - 1][1]);
  return PITCH_HZ.back()[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::silence()
{
  mySegments.clear();
  myPos = 0;
  myAfterPause = false;
  myClosureMs = 0.0;
  myHandOverMs = 0.0;
  myJoin = {};
  myJumpWait = false;
  rest();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::rest()
{
  myLive = false;
  myColdNext = true;
  myLastCode = -1;

  // Time passes unsaid, so the output stage is at rest; nothing rings on, as
  // the chip's apparent decay is only its output's DC settling
  mySynth.reset();
  mySounding = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::setParam(SpeakJet::Param param, uInt8 value)
{
  switch(param)
  {
    case SpeakJet::Param::Volume:  myVolume = value;  break;
    case SpeakJet::Param::Speed:   mySpeed = value;   break;
    case SpeakJet::Param::Pitch:   myPitch = value;   break;
    case SpeakJet::Param::Bend:
      // Higher would index past every per-Bend table
      myBend = std::min(value, SpeakJet::MAX_BEND);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::play(const SpeakJet::Utterance& utterance)
{
  const uInt8 code = utterance.code;
  const auto index = SZT(code - SpeakJet::FIRST_SOUND);

  // The sound coming next, a copy bringing its own; until the chip can see
  // what follows, nothing does
  const Next next = utterance.more
      ? Next{code, mySpeed, utterance.scalePct} : myLookahead();

  // Speed stretches part of a sound and leaves the rest; Fast and Slow then
  // stretch that part's tick, all but its last ticks into silence
  const double lenMs = LENGTH_MS[index];
  const double fixedMs = BSPF::clamp(SpeakJetTables::fixedMs(code), -lenMs / 2, lenMs / 2);
  const double speedTiming = SpeakJetTables::speedFactor(mySpeed);
  const double fastSlow = stretch(utterance.scalePct, mySpeed);
  const double timing = fastSlow * speedTiming;
  const double bodyMs = (lenMs - fixedMs) * speedTiming;
  const double lastMs = next.code >= 0 ? 0.0 : std::min(bodyMs,
      SPEECH_LAST_TICKS * TICK_MS[SpeakJet::INITIAL_SPEED] * speedTiming);
  double ms = fixedMs + (bodyMs - lastMs) * fastSlow + lastMs;

  // An effect runs its program on the tick Speed sets, a pulsed one with the
  // timing of speech, all but its last frame stretched by Fast and Slow
  double tickMs = 0.0, lastTickMs = 0.0;
  if(code > SpeakJet::LAST_ALLOPHONE)
  {
    const Effect& fx = EFFECTS[SZT(code - SpeakJet::LAST_ALLOPHONE - 1)];
    lastTickMs = fx.enveloped ? TICK_MS[SpeakJet::INITIAL_SPEED] * speedTiming
                              : TICK_MS[std::min(mySpeed, SpeakJet::MAX_SPEED)];
    tickMs = lastTickMs * fastSlow;
    ms = DBL(fx.lastFrame) * tickMs + DBL(fx.ticks - fx.lastFrame) * lastTickMs;
  }

  // Below the default Pitch a voiced sound keeps its length; above, the law
  const bool voiced = SpeakJet::isVoiced(code);
  const bool held = myPitch <= SpeakJet::INITIAL_PITCH && voices(code);
  const double pitchLaw = voiced && !held ? SpeakJetTables::phraseDuration(myPitch) : 1.0;
  ms *= pitchLaw;

  // The same sound again carries on or is said over, taking what a copy adds
  const bool again = std::cmp_equal(code, myLastCode);
  if(again)
  {
    if(const auto repeatMs = SpeakJetTables::repeatMs(code))
      ms = *repeatMs * timing * pitchLaw;
    else
      ms += SpeakJetTables::joinMs(code, code) * timing;
  }
  if(leadsIn(myLastCode, code))
    ms += NASAL_LEAD_MS * timing;

  // A stop before it may have given it time to start under its noise
  ms += myHandOverMs;
  myHandOverMs = 0.0;

  // The join with the sound coming next, whose Speed it takes, and a stop
  // its closure.  Until the chip can see what follows, the join stays open
  const bool seen = utterance.more || next.code >= 0 || next.pause;
  const double baseMs = ms;
  double closureMs = 0.0;
  if(next.code >= 0 && next.code != code && ms > 0.0)
    ms = BSPF::clamp(ms + extra(code, next, closureMs), ms * JOIN_MIN_SCALE,
                     ms * JOIN_MAX_SCALE);

  // A pause does not take away the closure of a stop after it
  if(myAfterPause)
    quiet(SpeakJetTables::closureMs(code) * timing, Kind::Closure);
  myAfterPause = false;

  // A stop opens with the closure the sound before it left room for
  if(myClosureMs > 0.0)
    quiet(myClosureMs, Kind::Closure);
  myClosureMs = 0.0;

  const bool flowing = myLastCode >= 0;
  myLastCode = code;
  const size_t before = mySegments.size();
  if(voicedThrough(code))
    expand(code, ms, !again || !SpeakJetTables::holdsRepeat(code), next.code);
  else
  {
    unvoiced(code, ms, tickMs, lastTickMs);
    if(stopsVoicing(code))
      myColdNext = true;
  }
  const size_t sound = mySegments.size();

  // Between two sounds a quiet code takes time its sound does not have
  if(flowing && SpeakJetTables::quietPadMs(code) > 0)
    quiet(DBL(SpeakJetTables::quietPadMs(code)) * timing);
  myClosureMs = closureMs;
  myJoin = {!seen && sound > before, code, baseMs, mySegments.size() - sound};
  if(next.code >= 0 && !utterance.more)
    handOver(code, next.code, mySegments.size() - sound);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::pause(uInt32 durationMs)
{
  // Speed scales a pause as it does a sound
  delay(U32(DBL(durationMs) * SpeakJetTables::speedFactor(mySpeed) + 0.5));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::delay(uInt32 durationMs)
{
  // A pause is not a join, and voicing after it starts from nothing
  quiet(DBL(durationMs));
  myClosureMs = 0.0;
  myHandOverMs = 0.0;
  myJoin = {};
  myLastCode = -1;
  myColdNext = true;
  myAfterPause = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::expand(int code, double ms, bool rearticulate, int into)
{
  const Sound& sound = SOUNDS[SZT(code - SpeakJet::FIRST_SOUND)];
  const auto total = SZT(std::max(0.0, ms) * SpeakJetSynth::RATE / 1000.0);
  if(total == 0)
    return;

  const State end = endState(code);

  // Each state is reached by one slide from wherever the registers are; a
  // glide's slides span the points its states were measured at
  struct Point { size_t from; Registers target; size_t span; bool jump; };
  const auto share = [total](double f) { return SZT(f * DBL(total)); };
  const auto regs = [&](const State& s) { return registersFor(s, code, myBend); };
  State bar = sound.start;
  vector<Point> points;
  if(!rearticulate)
    points = {{0, regs(end), 0, false}};
  else if(initialStop(code))
  {
    // The voice bar, then the release
    bar.level[1] = bar.level[2] = 0.0;
    points = {{0, regs(bar), 0, true},
              {share(RELEASE_AT), regs(end), 0, true}};
  }
  else if(finalStop(code))
  {
    // Out of the vowel, then the voice bar fading
    bar = {{VOICE_BAR_HZ, sound.start.hz[1], sound.start.hz[2]},
           {FINAL_BAR_LEVEL, 0.0, 0.0}, 0.0};
    points = {{0, regs(sound.start), 0, false},
              {share(RELEASE_AT), regs(bar), 0, true}};
  }
  else if(sound.glide)
    points = {{0, regs(sound.onset), 0, false},
              {share(ONSET_AT), regs(sound.start), share(START_AT - ONSET_AT), false},
              {share(END_STATE_AT), regs(end), share(END_AT - END_STATE_AT), false}};
  else
    points = {{0, regs(sound.start), 0, false},
              {share(END_STATE_AT), regs(end), 0, false}};

  // Running straight into a stop, its entry slide ends the sound
  Registers to;
  size_t slide = 0;
  if(entry(code, into, to, slide))
  {
    const size_t from = total - std::min(total, slide);
    if(from > points.back().from)
      points.push_back({from, to, 0, false});
  }

  for(size_t i = 0; i < points.size(); ++i)
  {
    const size_t until = i + 1 < points.size() ? points[i + 1].from : total;
    if(until <= points[i].from)
      continue;
    mySegments.push_back({
      .samples = until - points[i].from,
      .target = points[i].target,
      .span = points[i].span,
      .jump = points[i].jump,
      .kind = Kind::Voiced,
      .code = code,
      .last = i + 1 == points.size(),
      .speed = mySpeed,
      .pitch = myPitch,
      .master = myVolume
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::stretch(uInt32 scalePct, uInt8 speed)
{
  const uInt8 s = std::min(speed, SpeakJet::MAX_SPEED);
  if(scalePct == SpeakJet::FAST_PCT)
    return TICK_MS[FAST_SPEED_BASE + s / 2] / TICK_MS[s];
  if(scalePct == SpeakJet::SLOW_PCT)
    return SLOW_TICK_SCALE - SLOW_TICK_FIXED_MS / TICK_MS[s];
  return 1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::extra(int code, const Next& next, double& closureMs) const
{
  const auto to = U8(next.code);
  // Fast and Slow on the incoming sound stretch only its closure
  const double speedTiming = SpeakJetTables::speedFactor(next.speed);
  double extraMs = SpeakJetTables::joinMs(U8(code), to) * speedTiming;
  if(leadsIn(code, to))
    extraMs -= NASAL_LEAD_MS * speedTiming;
  const double closure = BSPF::clamp(extraMs, 0.0, SpeakJetTables::closureMs(to) * speedTiming);
  closureMs = closure * stretch(next.scalePct, next.speed);
  return extraMs - closure;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetMSA::entry(int code, int into, Registers& to, size_t& samples) const
{
  // The oscillators slide at the ordinary rate to the stop's entry state
  const State end = endState(code);
  for(const auto& stop: ENTRIES)
    if(stop.code == into)
    {
      const double scale = slewScale(mySpeed);
      double slideMs = 0.0;
      for(size_t k = 0; k < 3; ++k)
        if(end.hz[k] > 0.0 && end.level[k] > 0.0)
          slideMs = std::max(slideMs, std::abs(stop.hz[k] - end.hz[k]) /
                                      (SLEW_HZ_PER_MS[k] * scale));
      to = registersFor({stop.hz, end.level, end.pedestal}, code, myBend);
      samples = SZT(slideMs * SpeakJetSynth::RATE / 1000.0);
      return true;
    }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::join()
{
  const Next next = myLookahead();
  if(next.code < 0 && !next.pause)
    return;
  myJoin.open = false;
  if(next.code < 0 || next.code == myJoin.code || mySegments.size() <= myJoin.after)
    return;

  double closureMs = 0.0;
  const double ms = BSPF::clamp(myJoin.ms + extra(myJoin.code, next, closureMs),
                                myJoin.ms * JOIN_MIN_SCALE, myJoin.ms * JOIN_MAX_SCALE);
  myClosureMs = closureMs;

  // The sound's last segment takes the difference, as far as it still can
  const size_t at = mySegments.size() - 1 - myJoin.after;
  Segment& last = mySegments[at];
  const size_t least = at == 0 ? myPos + 1 : 1;
  const double samples = DBL(last.samples) + (ms - myJoin.ms) * SpeakJetSynth::RATE / 1000.0;
  last.samples = std::max(least, SZT(std::max(0.0, samples)));

  // Into a stop, the entry slide ends the sound
  Registers to;
  size_t slide = 0;
  if(last.kind == Kind::Voiced && entry(myJoin.code, next.code, to, slide) &&
     slide > 0 && last.samples >= least + slide)
  {
    Segment tail = last;
    tail.samples = slide;
    tail.target = to;
    tail.span = 0;
    tail.jump = false;
    last.samples -= slide;
    last.last = false;
    mySegments.insert(mySegments.begin() + I32(at) + 1, tail);
  }
  handOver(myJoin.code, next.code, myJoin.after);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::handOver(int code, int next, size_t after)
{
  // The voicing's formants rise and the noise falls away together (begin()),
  // within the stop's own time; nothing between them, no closure
  if(after > 0 || mySegments.empty() || !stopsVoicing(code) || !voicedThrough(next) ||
     myClosureMs > 0.0)
    return;
  Segment& last = mySegments.back();
  if(last.kind != Kind::Noise)
    return;
  const size_t least = mySegments.size() == 1 ? myPos + 1 : 1;
  const size_t cut = std::min(SZT(HAND_OVER_MS * SpeakJetSynth::RATE / 1000.0),
                              last.samples > least ? last.samples - least : 0);
  last.samples -= cut;
  last.handsOver = true;
  myHandOverMs = DBL(cut) * 1000.0 / SpeakJetSynth::RATE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::quiet(double ms, Kind kind)
{
  const auto samples = SZT(std::max(0.0, ms) * SpeakJetSynth::RATE / 1000.0);
  if(samples == 0)
    return;

  mySegments.push_back({
    .samples = samples,
    .target = myRegs,
    .kind = kind,
    .speed = mySpeed,
    .pitch = myPitch,
    .master = myVolume
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::unvoiced(int code, double ms, double tickMs, double lastTickMs)
{
  const auto samples = SZT(std::max(0.0, ms) * SpeakJetSynth::RATE / 1000.0);
  if(samples == 0)
    return;

  const Registers target = registersFor(State{}, code, myBend);
  const Kind kind = code > SpeakJet::LAST_ALLOPHONE ? Kind::Effect
                  : code == PE || code == PO ? Kind::Thump
                  : target.volume[3] > 0.0 || target.volume[4] > 0.0 ? Kind::Noise : Kind::Quiet;
  mySegments.push_back({
    .samples = samples,
    .target = target,
    .kind = kind,
    .code = code,
    .last = true,
    .speed = mySpeed,
    .pitch = myPitch,
    .master = myVolume,
    .bend = myBend,
    .tick = std::max(1.0, tickMs * SpeakJetSynth::RATE / 1000.0),
    .lastTick = std::max(1.0, lastTickMs * SpeakJetSynth::RATE / 1000.0)
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::synth(const SpeakJetSynth::Registers& regs)
{
  if(!mySounding)
  {
    mySynth.restart();
    mySounding = true;
  }
  return mySynth.sample(regs);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::still()
{
  mySounding = false;
  SpeakJetSynth::Registers regs = myRegs;
  regs.volume = {};
  return mySynth.sample(regs);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetMSA::runsOn() const
{
  if(mySegments.size() > 1)
    return mySegments[1].kind == Kind::Voiced;

  // A stop's closure comes first
  const Next next = myLookahead();
  return next.code >= 0 && voicedThrough(next.code) && myClosureMs <= 0.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetMSA::begin()
{
  const Segment& s = mySegments.front();
  myEndKnown = false;

  if(s.kind != Kind::Voiced)
  {
    if(s.kind == Kind::Noise)
      myRegs = s.target;
    else if(s.kind == Kind::Effect)
    {
      const Effect& fx = EFFECTS[SZT(s.code - SpeakJet::LAST_ALLOPHONE - 1)];
      double envelope = 0.0;
      myRegs = effectRegisters(fx, 0.0, s.bend, envelope);
    }
    myFrom = myTo = myRegs;
    mySlide = 1;
    return;
  }

  // Voicing from nothing starts at its state, rising from silence if cold as
  // a stop's noise before it falls away; running on, it slides there
  myJumpWait = false;
  if(!myLive)
  {
    myTo = settle(s.target, s.target);
    myFrom = myTo;
    myCold = myColdNext;
    myColdNext = false;
    if(myCold)
      for(size_t k = 0; k < 3; ++k)
        myFrom.volume[k] = 0.0;
    if(myLastKind == Kind::Noise)
    {
      for(size_t k = 3; k < 5; ++k)
      {
        myFrom.hz[k] = myRegs.hz[k];
        myFrom.volume[k] = myRegs.volume[k];
      }
      myFrom.distortion = myRegs.distortion;
    }
    myRegs = myFrom;
    mySlide = myCold || myLastKind == Kind::Noise
        ? SZT(COLD_FADE_MS * SpeakJetSynth::RATE / 1000.0) : 1;
    myLive = true;
    myRunMs = 0.0;
    return;
  }

  myFrom = myRegs;
  myTo = settle(s.target, myFrom);
  double ms = 0.0;
  const double scale = slewScale(s.speed);
  for(size_t k = 0; k < 3; ++k)
  {
    if(myFrom.volume[k] <= 0.0)
      myFrom.hz[k] = myTo.hz[k];
    ms = std::max(ms, std::abs(myTo.hz[k] - myFrom.hz[k]) / (SLEW_HZ_PER_MS[k] * scale));
  }

  // A silent noise oscillator takes its new setting at once
  for(size_t k = 3; k < 5; ++k)
    if(myFrom.volume[k] <= 0.0)
      myFrom.hz[k] = myTo.hz[k];
  if(myFrom.volume[3] <= 0.0 && myFrom.volume[4] <= 0.0)
    myFrom.distortion = myTo.distortion;
  mySlide = s.jump ? 1 : std::max({size_t{1}, SZT(ms * SpeakJetSynth::RATE / 1000.0), s.span});
  myJumpWait = s.jump;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetMSA::next()
{
  // A run of voicing ends faded, and silence follows
  if(mySegments.empty())
  {
    rest();
    return 0.0;
  }

  if(myJoin.open)
    join();
  if(myPos == 0)
    begin();
  const Segment& s = mySegments.front();

  // Pitch glides to each setting
  const double want = pitchHz(s.pitch);
  const double step = PITCH_GLIDE_HZ_PER_MS * 1000.0 / SpeakJetSynth::RATE;
  myF0 = myF0 <= 0.0 ? want : myF0 + BSPF::clamp(want - myF0, -step, step);

  double y = 0.0;
  if(s.kind == Kind::Voiced)
  {
    // From nothing, voicing's pitch rises into place
    const double f0 = myF0 * (myCold ? COLD_GLIDE_FROM + (1.0 - COLD_GLIDE_FROM) *
                                       std::min(1.0, myRunMs / COLD_GLIDE_MS) : 1.0);

    // The registers on their slide; a stop's jump waits for the next period
    if(myJumpWait && (!mySounding || mySynth.startsPeriod(f0)))
      myJumpWait = false;
    const double p = myJumpWait ? 0.0 : std::min(1.0, DBL(myPos) / DBL(mySlide));
    for(size_t k = 0; k < 5; ++k)
    {
      myRegs.hz[k] = myFrom.hz[k] + p * (myTo.hz[k] - myFrom.hz[k]);
      myRegs.volume[k] = myFrom.volume[k] + p * (myTo.volume[k] - myFrom.volume[k]);
    }
    myRegs.distortion = myFrom.distortion + p * (myTo.distortion - myFrom.distortion);
    myRegs.noiseHalf = myTo.noiseHalf;
    myRegs.enveloped = true;
    myRegs.envelopeHz = f0;

    // A run ends faded, the chip having read ahead to see it end
    double ending = 1.0;
    const auto fadeLength = SZT(END_FADE_MS * SpeakJetSynth::RATE / 1000.0);
    const size_t left = s.samples - myPos;
    if(s.last && left < fadeLength)
    {
      if(!myEndKnown)
      {
        myEnds = !runsOn();
        myEndKnown = true;
      }
      if(myEnds)
        ending = DBL(left) / DBL(fadeLength);
    }
    myRegs.master = s.master * ending;
    y = synth(myRegs) * SpeakJetTables::SPEECH_GAIN;
    myRunMs += 1000.0 / SpeakJetSynth::RATE;
  }
  else
  {
    myLive = false;
    const double pos = DBL(myPos), length = DBL(s.samples);
    if(s.kind == Kind::Noise)
    {
      // It ramps in and out, but where it hands over to voicing
      const double ramp = NOISE_RAMP_MS * SpeakJetSynth::RATE / 1000.0;
      const double fade = NOISE_END_MS * SpeakJetSynth::RATE / 1000.0;
      const double out = s.handsOver ? 1.0 : std::min(1.0, (length - pos) / fade);
      myRegs.envelopeHz = myF0;
      myRegs.master = s.master * std::min(1.0, (pos + 1.0) / ramp) * out;
      y = synth(myRegs) * SpeakJetTables::SPEECH_GAIN;
    }
    else if(s.kind == Kind::Effect)
    {
      // The program a step each tick, its last frame on the plain tick; while
      // enveloped, at the Pitch and its offset, a grain keeps its registers
      const Effect& fx = EFFECTS[SZT(s.code - SpeakJet::LAST_ALLOPHONE - 1)];
      const double head = DBL(fx.lastFrame) * s.tick;
      const double tick = pos < head ? std::floor(pos / s.tick)
                        : DBL(fx.lastFrame) + std::floor((pos - head) / s.lastTick);
      double envelope = 0.0;
      const Registers r = effectRegisters(fx, tick, s.bend, envelope);
      const double f0 = myF0 + envelope;
      if(!r.enveloped || !myRegs.enveloped || mySynth.startsPeriod(f0))
        myRegs = r;
      myRegs.envelopeHz = f0;
      myRegs.master = s.master;
      y = synth(myRegs) * SpeakJetTables::SPEECH_GAIN;
    }
    else if(s.kind == Kind::Thump)
    {
      // Oscillator 1 a plain sine at the measured frequency under the code's
      // level curve, beside the code's noise
      myRegs = s.target;
      myRegs.enveloped = false;
      myRegs.hz[0] = THUMP_HZ / CLOCK;
      myRegs.volume[0] = THUMP_LEVEL * PLAIN_VOLUME * std::sin(BSPF::PI_d * pos / length);
      myRegs.envelopeHz = myF0;
      myRegs.master = s.master;
      y = synth(myRegs) * SpeakJetTables::SPEECH_GAIN;
    }
    else
      y = still() * SpeakJetTables::SPEECH_GAIN;
  }

  if(++myPos >= s.samples)
  {
    // Past the sound's end, its join can no longer open
    if(mySegments.size() == myJoin.after + 1)
      myJoin.open = false;
    if(s.kind == Kind::Voiced && s.last && myEndKnown && myEnds)
      myLive = false;
    myLastKind = s.kind;
    mySegments.pop_front();
    myPos = 0;
  }
  return y;
}
