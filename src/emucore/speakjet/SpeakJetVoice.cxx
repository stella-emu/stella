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
#include "SpeakJetSamples.hxx"
#include "SpeakJetTables.hxx"
#include "SpeakJetVoice.hxx"

namespace {
  using namespace SpeakJetChip;

  // The recordings' own fundamental at Pitch 88, where every residual is taken
  constexpr double RESIDUAL_F0 = 87.83;

  // How far a residual period may be moved to meet the synthesised one, at 48kHz
  constexpr double RESIDUAL_SEARCH_48K = 12.0;

  // Where in a recording each state is taken, as a share of its length: a
  // glide's onset, then the start and end states
  constexpr double ONSET_AT = 0.15, START_AT = 0.3, END_AT = 0.75;

  // Under this share of its state's loudest, an oscillator is all but silent
  constexpr double QUIET_LEVEL = 0.1;

  // VV, ZZ and ZH: made as the continuants are, plus their noise
  constexpr int FIRST_VOICED_FRICATIVE = 166, LAST_VOICED_FRICATIVE = 168;

  // DH and the voiced stops: recorded, but a made sound after one slides out of it
  constexpr int FIRST_CARRIED = 169, LAST_CARRIED = 181;

  double grainAt(const std::array<double, GRAIN_POINTS>& curve, double tau)
  {
    const double u = tau / GRAIN_SECONDS * DBL(GRAIN_POINTS - 1);
    if(u < 0.0 || u >= DBL(GRAIN_POINTS - 1))
      return 0.0;
    const auto i = SZT(u);
    return curve[i] + (u - DBL(i)) * (curve[i + 1] - curve[i]);
  }

  void advance(const std::array<double, 4>& e, const std::array<double, 2>& g,
               std::array<double, 2>& x, double u)
  {
    const double x0 = e[0] * x[0] + e[1] * x[1] + g[0] * u;
    x[1] = e[2] * x[0] + e[3] * x[1] + g[1] * u;
    x[0] = x0;
  }

  // A 4th-order Butterworth low-pass as two biquads, run both ways so it
  // shifts nothing in time
  void lowpass(vector<double>& y, uInt32 rate, double hz)
  {
    const size_t n = y.size();
    const double k = 2.0 * DBL(rate), w = k * std::tan(BSPF::PI_d * hz / DBL(rate));

    for(const double q: {0.5411961, 1.3065630})
      for(int pass = 0; pass < 2; ++pass)
      {
        const double a0 = k * k + k * w / q + w * w;
        const double b0 = w * w / a0, a1 = (2.0 * w * w - 2.0 * k * k) / a0;
        const double a2 = (k * k - k * w / q + w * w) / a0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        for(size_t j = 0; j < n; ++j)
        {
          double& v = y[pass == 0 ? j : n - 1 - j];
          const double l = b0 * (v + 2.0 * x1 + x2) - a1 * y1 - a2 * y2;
          x2 = x1; x1 = v; y2 = y1; y1 = l;
          v = l;
        }
      }
  }

  // Where each voicing period starts: above 15 percent of the peak after 1ms
  // below it, low-passed under a voiced fricative's ripple
  vector<size_t> voicingStarts(const vector<double>& x, uInt32 rate)
  {
    const size_t n = x.size();
    vector<double> y{x};
    lowpass(y, rate, 1200.0);

    double peak = 0.0;
    for(const double v: y)
      peak = std::max(peak, std::abs(v));

    const size_t quiet = rate / 1000;
    vector<size_t> starts;
    size_t below = 0;
    for(size_t i = 0; i < n; ++i)
    {
      const bool on = std::abs(y[i]) > 0.15 * peak;
      if(on && below >= quiet && i >= 2 * quiet)
        starts.push_back(i);
      below = on ? 0 : below + 1;
    }
    return starts;
  }

  // A state as Bend moves it: one offset on every sounding oscillator,
  // relative to the default Bend the states are given at
  State bent(State s, uInt8 bend)
  {
    const double offset = BEND_HZ[bend] - BEND_HZ[SpeakJet::DEFAULT_BEND];
    for(double& hz: s.hz)
      if(hz > 0.0)
        hz += offset;
    return s;
  }

  bool quiet(const State& s, size_t k)
  {
    return s.level[k] < QUIET_LEVEL * std::max({s.level[0], s.level[1], s.level[2]});
  }

  // One state, a silent oscillator given the other side's frequency so it does
  // not slew; an all but silent one is left out, its share is in the residual
  State settle(State s, const State& other)
  {
    for(size_t k = 0; k < 3; ++k)
      if(s.hz[k] < 0.0 || quiet(s, k))
      {
        s.hz[k] = other.hz[k] > 0.0 ? other.hz[k] : s.hz[k] > 0.0 ? s.hz[k] : 500.0;
        s.level[k] = 0.0;
      }
    return s;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetVoice::SpeakJetVoice(SpeakJetSamples& samples)
  : mySamples{samples}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetVoice::makes(int code)
{
  return ((code >= 128 && SpeakJet::isContinuant(U8(code))) ||
          (code >= FIRST_VOICED_FRICATIVE && code <= LAST_VOICED_FRICATIVE)) &&
         SOUNDS[SZT(code - 128)].voiced;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetVoice::carries(int code)
{
  return code >= FIRST_CARRIED && code <= LAST_CARRIED &&
         SOUNDS[SZT(code - 128)].voiced;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetVoice::Samples SpeakJetVoice::speak(int code, const vector<Part>& parts,
    uInt8 bend, uInt8 speed, bool cold, bool ends, int into)
{
  Voicing& v = myVoicing;
  const Sound& sound = SOUNDS[SZT(code - 128)];
  const Noise& noise = sound.noise;
  const auto rate = DBL(mySamples.rate());
  const size_t lead = mySamples.rate() / 1000;

  // Its residuals come from its own Bend's recordings
  mySamples.setFor(bend);

  // Voicing that did not run straight in starts afresh
  const bool fresh = !v.live;
  if(fresh)
  {
    const double f0 = v.f0;
    v = Voicing{};
    v.f0 = f0;
    v.live = true;
    v.cold = cold;
    v.delay.assign(lead, 0.0);
    const State& first = sound.glide ? sound.onset : sound.start;
    const State s = settle(bent(first, bend), bent(first, bend));
    v.hz = s.hz;
    v.level = s.level;
    v.pedestal = s.pedestal;
    v.resCode = code;
    v.resAt = sound.glide ? Point::Onset : Point::Start;
    v.resBend = bend;

    // Out of a recorded voiced stop the chip is already voicing at the stop's
    // state, and the sound slides in from it as from any other
    if(myCarryCode >= 0)
    {
      const State& end = SOUNDS[SZT(myCarryCode - 128)].end;
      State c = settle(bent(end, bend), s);

      // A stop whose release state is known hands on from there instead
      for(const auto& rel: RELEASES)
        if(rel.code == myCarryCode)
          c.hz = bent({rel.hz, c.level, c.pedestal}, bend).hz;
      v.hz = c.hz;
      v.level = c.level;
      v.pedestal = c.pedestal;
      v.resCode = myCarryCode;
      v.resAt = Point::End;
    }
  }
  myCarryCode = -1;

  size_t total = 0;
  vector<size_t> partEnds;
  for(const Part& part: parts)
  {
    total += SZT(std::max(0.0, part.targetMs) * rate / 1000.0);
    partEnds.push_back(total);
  }

  // Speed scales the slides exactly as it scales durations
  double scale = SPEED_SLEW.back()[1];
  for(size_t i = 1; i < SPEED_SLEW.size(); ++i)
    if(DBL(speed) <= SPEED_SLEW[i][0])
    {
      const auto& lo = SPEED_SLEW[i - 1];
      const auto& hi = SPEED_SLEW[i];
      scale = lo[1] + (DBL(speed) - lo[0]) / (hi[0] - lo[0]) * (hi[1] - lo[1]);
      break;
    }

  // Each state is reached by one synchronised slide from wherever the
  // oscillators are; inside a glide a slide spans the points it lies between
  struct Segment {
    size_t from;
    State target;
    Point at;
    size_t span;
  };
  const auto share = [total](double f) { return SZT(f * DBL(total)); };
  vector<Segment> segments;
  if(sound.glide)
  {
    segments.push_back({0, bent(sound.onset, bend), Point::Onset, 0});
    segments.push_back({share(ONSET_AT), bent(sound.start, bend), Point::Start,
                        share(START_AT - ONSET_AT)});
  }
  else
    segments.push_back({0, bent(sound.start, bend), Point::Start, 0});
  segments.push_back({share(END_STATE_AT), bent(sound.end, bend), Point::End,
                      share(END_AT - END_STATE_AT)});

  // Running straight into a recorded stop, the oscillators slide at the
  // ordinary rate to that stop's entry state, arriving at the sound's end
  for(const auto& entry: ENTRIES)
    if(entry.code == into)
    {
      const State end = bent(sound.end, bend);
      const State target = bent({entry.hz, end.level, end.pedestal}, bend);
      double ms = 0.0;
      for(size_t o = 0; o < 3; ++o)
        if(end.hz[o] > 0.0 && end.level[o] > 0.0)
          ms = std::max(ms, std::abs(target.hz[o] - end.hz[o]) / (SLEW_HZ_PER_MS[o] * scale));
      const size_t from = total - std::min(total, SZT(ms * rate / 1000.0));
      if(from > segments.back().from)
        segments.push_back({from, target, Point::End, 0});
    }
  size_t seg = 0;
  State was{}, to{};
  size_t slide = 1;
  int prevCode = v.resCode;
  Point prevAt = v.resAt;
  uInt8 prevBend = v.resBend;

  const auto pitchHz = [](uInt8 setting) {
    const double p = BSPF::clamp(DBL(setting), PITCH_HZ.front()[0], PITCH_HZ.back()[0]);
    for(size_t i = 1; i < PITCH_HZ.size(); ++i)
      if(p <= PITCH_HZ[i][0])
        return PITCH_HZ[i - 1][1] + (p - PITCH_HZ[i - 1][0]) /
               (PITCH_HZ[i][0] - PITCH_HZ[i - 1][0]) * (PITCH_HZ[i][1] - PITCH_HZ[i - 1][1]);
    return PITCH_HZ.back()[1];
  };

  const Path path = pathFor(rate);
  const size_t fadeLength = SZT(END_FADE_MS * rate / 1000.0);

  Samples out;
  out.reserve(total + lead);
  size_t part = 0;

  // The residual follows the fade sample by sample, so it ends with the run
  const auto emit = [&](double raw, double fade, double ending, double gain) {
    const double back = v.delay[v.head];
    v.delay[v.head] = raw;
    v.head = (v.head + 1) % lead;
    double r = 0.0;
    if(v.at - v.activeAt < v.active.size())
      r = v.active[SZT(v.at - v.activeAt)] * fade;

    // Held at the chip's own rate, then the output stage
    const double y = stageStep(path, v.stage, v.at, back + r) * ending;

    // The delay line is still filling at the start of a run
    if(!v.warm && v.at >= lead)
      v.warm = true;
    if(v.warm)
      out.push_back(SpeakJetDSP::limit(y * 32768.0 * gain));
  };

  for(size_t i = 0; i < total; ++i)
  {
    while(part + 1 < parts.size() && i >= partEnds[part])
      ++part;
    const Part& now = parts[part];
    const double gain = SpeakJetTables::volumeFactor(now.volume) * SpeakJetTables::SPEECH_GAIN;

    // A new target: slide from the oscillators as they are to it
    if(seg < segments.size() && i == segments[seg].from)
    {
      was = {v.hz, v.level, v.pedestal};
      to = settle(segments[seg].target, was);
      for(size_t o = 0; o < 3; ++o)
        if(was.level[o] <= 0.0)
          was.hz[o] = to.hz[o];
      double ms = 0.0;
      for(size_t o = 0; o < 3; ++o)
        ms = std::max(ms, std::abs(to.hz[o] - was.hz[o]) / (SLEW_HZ_PER_MS[o] * scale));
      slide = std::max(size_t{1}, SZT(ms * rate / 1000.0));

      // A glide moves between its states over the span they lie apart; a
      // steady sound keeps the ordinary rate
      if(sound.glide)
        slide = std::max(slide, segments[seg].span);
      if(seg > 0 || !fresh)
      {
        prevCode = seg > 0 ? code : v.resCode;
        prevAt = seg > 0 ? segments[seg - 1].at : v.resAt;
        prevBend = seg > 0 ? bend : v.resBend;
      }
      v.resCode = code;
      v.resAt = segments[seg].at;
      v.resBend = bend;
      v.slideStart = i;
      ++seg;
    }
    const double p = std::min(1.0, DBL(i - v.slideStart) / DBL(slide));
    for(size_t o = 0; o < 3; ++o)
    {
      v.hz[o] = was.hz[o] + p * (to.hz[o] - was.hz[o]);
      v.level[o] = was.level[o] + p * (to.level[o] - was.level[o]);
    }
    v.pedestal = was.pedestal + p * (to.pedestal - was.pedestal);

    // Pitch glides to each setting; from nothing it also rises into it
    const double want = pitchHz(now.setting);
    const double step = PITCH_GLIDE_HZ_PER_MS * 1000.0 / rate;
    v.f0 = v.f0 <= 0.0 ? want : v.f0 + BSPF::clamp(want - v.f0, -step, step);
    const double since = v.runMs;
    const double f0 = v.f0 * (v.cold ? COLD_GLIDE_FROM + (1.0 - COLD_GLIDE_FROM) *
                                       std::min(1.0, since / COLD_GLIDE_MS) : 1.0);
    const double fade = v.cold ? std::min(1.0, since / COLD_FADE_MS) : 1.0;

    // A run ends faded at the output, where fading leaves no step behind
    const double ending = ends && total - i < fadeLength
                        ? DBL(total - i) / DBL(fadeLength) : 1.0;

    // Each period starts the grain again, with its residual moving from the
    // last state's to this one's with the slide
    v.cycle += f0 / rate;
    if(v.since < 0.0 || v.cycle >= 1.0)
    {
      if(v.since >= 0.0)
        v.cycle -= 1.0;
      v.phase = {};
      v.since = 0.0;
      const vector<double>& ra = residual(prevCode, prevAt, prevBend);
      const vector<double>& rb = residual(code, v.resAt, bend);
      v.active.clear();
      if(!ra.empty() && !rb.empty())
      {
        v.active.resize(ra.size());
        for(size_t j = 0; j < ra.size(); ++j)
          v.active[j] = (1.0 - p) * ra[j] + p * rb[j];
      }
      v.activeAt = v.at;
    }

    const double tau = v.since / rate;
    double sum = 0.0;
    for(size_t o = 0; o < 3; ++o)
    {
      sum += v.level[o] * std::sin(v.phase[o]);
      v.phase[o] += 2.0 * BSPF::PI_d * v.hz[o] * CLOCK / rate;
    }
    double raw = (-grainAt(ENVELOPE, tau) * sum + v.pedestal * grainAt(PEDESTAL, tau)) * fade;
    if(noise.level > 0.0)
      raw += noise.level * voicedNoise(noise, tau, rate) * fade;
    v.since += 1.0;
    emit(raw, fade, ending, gain);
    ++v.at;
    v.runMs += 1000.0 / rate;
  }

  // Voicing stops here: what is still in the delay line goes out
  if(ends)
  {
    const double gain = parts.empty()
        ? SpeakJetTables::SPEECH_GAIN
        : SpeakJetTables::volumeFactor(parts.back().volume) * SpeakJetTables::SPEECH_GAIN;
    for(size_t i = 0; i < lead; ++i)
    {
      emit(0.0, 0.0, 0.0, gain);
      ++v.at;
    }
    v.live = false;
  }
  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const vector<double>& SpeakJetVoice::residual(int code, Point which, uInt8 bend) const
{
  const auto index = SZT(code - 128);
  const size_t slot = SZT(bend) * 72 + index;
  const auto point = SZT(which);
  vector<double>& r = myResidual[slot][point];
  if(myResidualDone[slot][point])
    return r;
  myResidualDone[slot][point] = true;

  // That Bend's own recording, against the state as that Bend moves it
  const uInt32 rate = mySamples.rate();
  const SpeakJetSamples::Clip& clip = mySamples.recorded(bend)[index];
  const Sound& sound = SOUNDS[index];
  const State st = bent(which == Point::End ? sound.end
                        : which == Point::Onset ? sound.onset : sound.start, bend);
  const auto period = SZT(std::round(DBL(rate) / RESIDUAL_F0));
  const size_t lead = rate / 1000;
  if(clip.body.size() < 4 * period)
    return r;

  vector<double> x(clip.body.size());
  for(size_t i = 0; i < x.size(); ++i)
    x[i] = DBL(clip.body[i]) / 32768.0;

  // The triggers lie on one grid at a steady Pitch, so fitting it skips a
  // second pulse inside a period and a missed start
  const vector<size_t> starts = voicingStarts(x, rate);
  const double per = DBL(rate) / RESIDUAL_F0;
  size_t support = 0;
  double phase = 0.0;
  for(const size_t a: starts)
  {
    size_t n = 0;
    double sum = 0.0;
    for(const size_t s: starts)
    {
      const double k = std::round((DBL(s) - DBL(a)) / per);
      if(std::abs(DBL(s) - DBL(a) - k * per) <= 3.0)
      {
        ++n;
        sum += DBL(s) - k * per;
      }
    }
    if(n > support)
    {
      support = n;
      phase = sum / DBL(n);
    }
  }
  const double want = (which == Point::End ? END_AT : which == Point::Onset ? ONSET_AT : START_AT) *
                     DBL(x.size());
  double k = std::round((want - phase) / per);
  while(k > 0.0 && phase + k * per + DBL(period) > DBL(x.size()))
    k -= 1.0;
  while(phase + k * per < DBL(lead))
    k += 1.0;

  // A voiced fricative's ripple hides its period starts, so its period is
  // the one at the point, met to the model's by a search over half a period
  const bool noisy = sound.noise.level > 0.0;
  const auto at = noisy
      ? BSPF::clamp(SZT(std::lround(want)), lead + period, x.size() - 2 * period)
      : SZT(std::lround(phase + k * per));
  if((!noisy && support < 3) || at + period > x.size())
    return r;

  // Its noise does not repeat, so the period is the mean of every one through
  // the middle of the sound, each aligned to this one by correlation
  if(noisy)
  {
    vector<double> mean(period, 0.0);
    size_t n = 0;
    const size_t half = period / 2;
    const double first = DBL(at) - std::floor(DBL(at) / per) * per;
    const double last = 0.8 * DBL(x.size()) - DBL(period + half);
    for(size_t p = 0; first + DBL(p) * per <= last; ++p)
    {
      const double g = first + DBL(p) * per;
      if(g < 0.2 * DBL(x.size()) || g < DBL(lead + half))
        continue;
      const auto a = SZT(std::lround(g));
      size_t best = a;
      double most = -1e30;
      for(size_t c = a - half; c <= a + half; ++c)
      {
        double dot = 0.0, norm = 0.0;
        for(size_t i = 0; i < period; ++i)
        {
          dot += x[at - lead + i] * x[c - lead + i];
          norm += x[c - lead + i] * x[c - lead + i];
        }
        const double score = dot / (std::sqrt(norm) + 1e-12);
        if(score > most)
        {
          most = score;
          best = c;
        }
      }
      for(size_t i = 0; i < period; ++i)
        mean[i] += x[best - lead + i];
      ++n;
    }
    if(n > 0)
      for(size_t i = 0; i < period; ++i)
        x[at - lead + i] = mean[i] / DBL(n);
  }

  // The synthesised period for this state, a few periods in so the filter
  // has settled
  vector<double> raw(4 * period);
  for(size_t i = 0; i < raw.size(); ++i)
  {
    const double tau = std::fmod(DBL(i) / DBL(rate), 1.0 / RESIDUAL_F0);
    double sum = 0.0;
    for(size_t o = 0; o < 3; ++o)
      if(st.hz[o] > 0.0 && !quiet(st, o))
        sum += st.level[o] * std::sin(2.0 * BSPF::PI_d * st.hz[o] * CLOCK * tau);
    raw[i] = -grainAt(ENVELOPE, tau) * sum + st.pedestal * grainAt(PEDESTAL, tau);
  }
  const vector<double> model = chipOutput(raw, rate);

  const auto k0 = SZT(std::ceil(2.0 * DBL(rate) / RESIDUAL_F0));
  const auto reach = noisy ? period / 2
                           : SZT(std::round(RESIDUAL_SEARCH_48K * DBL(rate) / 48000.0));
  size_t best = k0 - lead;
  double least = 1e30;
  for(size_t off = k0 - lead - reach; off <= k0 - lead + reach; ++off)
  {
    double e = 0.0;
    for(size_t i = 0; i < period; ++i)
      e += (x[at - lead + i] - model[off + i]) * (x[at - lead + i] - model[off + i]);
    if(e < least)
    {
      least = e;
      best = off;
    }
  }

  // Registered to 1ms before the trigger, which is where it is added back
  r.assign(period, 0.0);
  double rr = 0.0, gg = 0.0;
  for(size_t i = 0; i < period; ++i)
  {
    const double d = x[at - lead + i] - model[best + i];
    const auto j = I64(i) + I64(best) - I64(k0 - lead);
    if(j >= 0 && j < I64(period))
      r[SZT(j)] = d;
    rr += d * d;
    gg += x[at - lead + i] * x[at - lead + i];
  }

  // Only what the chip computes: added before the hold, it makes the hold's
  // images afresh as the chip does
  toChip(r, rate);

  // A residual larger than the period itself means a bad alignment
  if(rr >= gg)
    r.clear();
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetVoice::voicedNoise(const Noise& noise, double tau, double rate)
{
  Voicing& v = myVoicing;

  // Each oscillator's jitter is drawn afresh every NOISE_HOLD chip samples
  v.noiseHeld += 8192.0 * CLOCK / rate;
  if(v.noiseHeld >= DBL(NOISE_HOLD))
  {
    v.noiseHeld -= DBL(NOISE_HOLD);
    for(double& u: v.noiseJitter)
    {
      v.noiseSeed = v.noiseSeed * 1664525U + 1013904223U;
      u = DBL(v.noiseSeed) / 4294967296.0 * 2.0 - 1.0;
    }
  }

  double y = std::sin(v.noisePhase[0]) + noise.ratio * std::sin(v.noisePhase[1]);
  const std::array<double, 2> hz{noise.hz4, noise.hz5};
  for(size_t o = 0; o < 2; ++o)
  {
    v.noisePhase[o] += 2.0 * BSPF::PI_d * hz[o] * CLOCK *
                       (1.0 + NOISE_K * noise.distortion * v.noiseJitter[o]) / rate;
    v.noisePhase[o] = std::fmod(v.noisePhase[o], 2.0 * BSPF::PI_d);
  }

  // Half enveloped: half constant, half under the grain's envelope
  if(noise.half)
    y *= 0.5 + 0.5 * grainAt(ENVELOPE, tau);
  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetVoice::Path SpeakJetVoice::pathFor(double rate)
{
  Path p;
  p.rate = rate;
  p.dt = 1.0 / rate;
  const double w = 2.0 * BSPF::PI_d * LOWPASS_HZ;
  p.w2 = w * w;
  p.wq = w / LOWPASS_Q;
  const auto root = std::sqrt(std::complex<double>(p.wq * p.wq - 4.0 * p.w2));
  p.p1 = (-p.wq + root) / 2.0;
  p.p2 = (-p.wq - root) / 2.0;
  lowpassOver(p, p.dt, p.e, p.g);
  const double k = 2.0 * rate, wc = k * std::tan(BSPF::PI_d * HIGHPASS_HZ / rate);
  p.h0 = k / (k + wc); p.h1 = (wc - k) / (k + wc);
  return p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetVoice::lowpassOver(const Path& p, double t, std::array<double, 4>& e,
                                std::array<double, 2>& g)
{
  // A = [0 1; -w2 -wq]: e^At = c0 I + c1 A by Sylvester's formula, and the
  // held input's part is [1 - c0, w2 c1]
  std::complex<double> c0, c1;
  if(std::abs(p.p1 - p.p2) < 1e-9 * std::abs(p.p1))
  {
    const auto x = std::exp(p.p1 * t);
    c0 = x * (1.0 - p.p1 * t);
    c1 = x * t;
  }
  else
  {
    const auto x1 = std::exp(p.p1 * t), x2 = std::exp(p.p2 * t);
    c0 = (p.p1 * x2 - p.p2 * x1) / (p.p1 - p.p2);
    c1 = (x1 - x2) / (p.p1 - p.p2);
  }
  e = { c0.real(), c1.real(), -p.w2 * c1.real(), c0.real() - p.wq * c1.real() };
  g = { 1.0 - c0.real(), p.w2 * c1.real() };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetVoice::stageStep(const Path& p, Stage& s, uInt64 at, double in)
{
  const auto now = U64(std::floor(DBL(at) * CHIP_RATE / p.rate));
  if(now == s.tick)
    advance(p.e, p.g, s.x, s.held);
  else
  {
    // A hold edge since the last sample: the old value up to it, then the
    // value the chip computes there, between this sample's and the last
    const double into = BSPF::clamp(DBL(now) / CHIP_RATE - DBL(at - 1) * p.dt, 0.0, p.dt);
    std::array<double, 4> e{};
    std::array<double, 2> g{};
    lowpassOver(p, into, e, g);
    advance(e, g, s.x, s.held);
    s.held = s.last + (in - s.last) * into / p.dt;
    lowpassOver(p, p.dt - into, e, g);
    advance(e, g, s.x, s.held);
    s.tick = now;
  }
  s.last = in;

  const double h = p.h0 * (s.x[0] - s.lowLast) - p.h1 * s.highLast;
  s.lowLast = s.x[0];
  s.highLast = h;
  return h;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
vector<double> SpeakJetVoice::chipOutput(const vector<double>& raw, uInt32 rate)
{
  const Path p = pathFor(DBL(rate));
  Stage stage;
  vector<double> y(raw.size());
  for(size_t i = 0; i < raw.size(); ++i)
    y[i] = stageStep(p, stage, i, raw[i]);
  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::complex<double> SpeakJetVoice::pathGain(const Path& p, double hz)
{
  const double om = 2.0 * BSPF::PI_d * hz;
  const auto lp = p.w2 / std::complex<double>(p.w2 - om * om, om * p.wq);
  const auto z1 = std::polar(1.0, -om / p.rate);
  const auto hp = p.h0 * (1.0 - z1) / (1.0 + p.h1 * z1);
  const double u = BSPF::PI_d * hz / CHIP_RATE;

  // Each held value is taken between two samples, which on average loses this
  const double between = std::sqrt(1.0 - (1.0 - std::cos(om / p.rate)) / 3.0);
  return lp * hp * std::polar(between * (u > 0.0 ? std::sin(u) / u : 1.0), -u);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetVoice::toChip(vector<double>& y, uInt32 rate)
{
  const size_t n = y.size();
  const Path p = pathFor(DBL(rate));
  vector<std::complex<double>> spec(n / 2 + 1);
  for(size_t k = 1; k < spec.size(); ++k)
  {
    const double hz = DBL(k) * DBL(rate) / DBL(n);
    if(hz >= CHIP_RATE / 2.0)
      break;
    std::complex<double> s{};
    for(size_t i = 0; i < n; ++i)
      s += y[i] * std::polar(1.0, -2.0 * BSPF::PI_d * DBL(k * i % n) / DBL(n));
    spec[k] = s / pathGain(p, hz);
  }
  for(size_t i = 0; i < n; ++i)
  {
    double v = 0.0;
    for(size_t k = 1; k < spec.size(); ++k)
      v += (spec[k] * std::polar(1.0, 2.0 * BSPF::PI_d * DBL(k * i % n) / DBL(n))).real();
    y[i] = 2.0 * v / DBL(n);
  }
}
