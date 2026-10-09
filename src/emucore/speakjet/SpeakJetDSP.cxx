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
#include <limits>

#include "SpeakJetDSP.hxx"

namespace {
  // The most a crossfade may be corrected for the level it would lose
  constexpr double FADE_MAX_GAIN = 2.0;

  // How long two parts of one sustained sound overlap when spliced
  constexpr uInt32 SUSTAIN_XFADE_MS = 15;

  // Above this fraction of full scale, peaks ease into it rather than clip
  constexpr double LIMIT_KNEE = 0.8;
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetDSP::Samples SpeakJetDSP::pitchShift(sShortSpan in, SpanOf<Span> spans,
                                             uInt32 rate, double sourceF0)
{
  const size_t n = in.size();
  const size_t period = SZT(DBL(rate) / sourceF0);
  const auto moves = [](const Span& span) {
    return std::abs(span.pitch - 1.0) >= PITCH_DEADBAND;
  };

  // Needs a few periods to work with, and a shift worth making
  if(period < 16 || n < period * 3 || std::ranges::none_of(spans, moves))
    return Samples{in.begin(), in.end()};

  // One mark per period, pulled onto the local energy peak so grains are cut
  // at the same phase every time; a mark in the wrong place is heard as buzz
  vector<size_t> marks;
  const size_t reach = period / 4;

  for(size_t at = period; at + period < n; )
  {
    const size_t lo = (at > reach) ? at - reach : 0;
    const size_t hi = std::min(at + reach, n - 1);
    size_t best = lo;
    Int32 loudest = -1;

    for(size_t i = lo; i <= hi; ++i)
      if(std::abs(Int32{in[i]}) > loudest)
      {
        loudest = std::abs(Int32{in[i]});
        best = i;
      }

    marks.push_back(best);
    at = best + period;
  }

  if(marks.size() < 2)
    return Samples{in.begin(), in.end()};

  // Lay the same grains back down at the new spacing; only how often a
  // period repeats changes, so the duration stays
  const size_t half = period;
  vector<float> acc(n, 0.F), weight(n, 0.F);
  size_t span = 0;

  for(size_t centre = half; centre < n; )
  {
    // The grain from wherever this moment sits in the original
    size_t pick = 0;
    size_t nearest = std::numeric_limits<size_t>::max();
    for(size_t i = 0; i < marks.size(); ++i)
    {
      const size_t d = (marks[i] > centre) ? marks[i] - centre : centre - marks[i];
      if(d < nearest)
      {
        nearest = d;
        pick = i;
      }
    }

    const size_t src = marks[pick];
    for(size_t j = 0; j < half * 2; ++j)
    {
      const size_t from = src + j - std::min(src, half);
      const size_t to = centre + j - half;

      if(from >= n || to >= n)
        continue;

      const float w = 0.5F - 0.5F * std::cos(2.F * BSPF::PI_f * FLT(j) /
                                             FLT(half * 2 - 1));
      acc[to] += FLT(in[from]) * w;
      weight[to] += w;
    }

    // The Pitch in force here sets the next grain's spacing, and the lattice
    // carries straight across a change rather than restarting
    while(span + 1 < spans.size() && centre >= spans[span].end)
      ++span;
    centre += std::max<size_t>(8, SZT(DBL(period) / spans[span].pitch));
  }

  // Between grains is silence, as between the chip's own pulses at a low
  // Pitch; only beyond the first and last grain does the input stand in
  Samples out{in.begin(), in.end()};
  size_t firstAt = n, lastAt = 0;

  for(size_t i = 0; i < n; ++i)
    if(weight[i] > 0.01F)
    {
      firstAt = std::min(firstAt, i);
      lastAt = i;
    }

  for(size_t i = firstAt; i <= lastAt; ++i)
    out[i] = weight[i] > 0.01F
      ? I16(BSPF::clamp(acc[i] / weight[i], -32768.F, 32767.F))
      : 0;

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetDSP::Samples SpeakJetDSP::timeScale(sShortSpan in, double factor,
                                            uInt32 rate, bool pinEnd,
                                            double sourceF0)
{
  const size_t n = in.size();
  const auto want = SZT(DBL(n) * factor);

  // Frame short enough for brief stops, long enough to carry pitch
  const auto frame = BSPF::clamp<size_t>(std::min(n, want) / 4, rate * 5 / 1000,
                                         rate * 30 / 1000);
  if(frame < 32 || std::min(n, want) < frame * 2)
    return Samples{in.begin(), in.end()};

  // The largest synthesis hop
  const size_t hop = frame / 2;

  // Wide enough to reach the matching phase, which is half a period of this
  // sound; any wider and the search can settle on the wrong period
  const size_t search = std::min(hop, SZT(DBL(rate) / sourceF0 / 2.0) + 1);

  // The final frame, in 'in' and in the output
  const size_t last = n - frame;
  const size_t end = want - frame;

  vector<float> window(frame);
  for(size_t i = 0; i < frame; ++i)
    window[i] = 0.5F - 0.5F * std::cos(2.F * BSPF::PI_f * FLT(i) / FLT(frame - 1));

  vector<float> acc(want, 0.F), weight(want, 0.F);

  // Slide within the search window to whatever best continues the previous
  // frame; this is what keeps the pitch intact instead of phasing
  const auto bestMatch = [&](size_t lo, size_t hi, size_t cont) {
    double best = -1e30;
    size_t bestAt = lo;

    for(size_t c = lo; c <= hi; ++c)
    {
      // Normalised, or a louder candidate wins over the true continuation
      double dot = 0.0, energy = 1.0;
      for(size_t i = 0; i < hop; i += 4)
      {
        dot += DBL(in[c + i]) * DBL(in[cont + i]);
        energy += DBL(in[c + i]) * DBL(in[c + i]);
      }
      const double score = dot / std::sqrt(energy);
      if(score > best)
      {
        best = score;
        bestAt = c;
      }
    }
    return bestAt;
  };
  const auto place = [&](size_t take, size_t at) {
    for(size_t i = 0; i < frame; ++i)
    {
      acc[at + i] += FLT(in[take + i]) * window[i];
      weight[at + i] += window[i];
    }
  };

  // Frames spaced evenly from first to last, so nothing is left uncovered;
  // the last is pinned only when a decay must follow exactly where it ends
  const size_t count = (end + hop - 1) / hop;
  size_t prevTake = 0, prevAt = 0;

  for(size_t k = 0; k <= count; ++k)
  {
    const size_t at = end * k / count;
    size_t take = last * k / count;

    if(k > 0 && (k < count || !pinEnd))
      take = bestMatch(take > search ? take - search : 0,
                       std::min(take + search, last),
                       prevTake + (at - prevAt));
    place(take, at);
    prevTake = take;
    prevAt = at;
  }

  // A window's very edge carries no weight, and there the input stands in
  Samples out(want);
  for(size_t i = 0; i < want; ++i)
    out[i] = weight[i] > 1e-6F
      ? I16(BSPF::clamp(acc[i] / weight[i], -32768.F, 32767.F))
      : in[std::min(n - 1, i * n / want)];

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetDSP::Samples SpeakJetDSP::crossfade(sShortSpan a, sShortSpan b)
{
  const size_t n = a.size();
  Samples out(n, 0);
  double ab = 0.0, aa = 0.0, bb = 0.0;

  for(size_t i = 0; i < n; ++i)
  {
    ab += DBL(a[i]) * DBL(b[i]);
    aa += DBL(a[i]) * DBL(a[i]);
    bb += DBL(b[i]) * DBL(b[i]);
  }

  // Two copies of one sound keep their level through a fade; two unrelated
  // ones lose 3dB in the middle of it, and anything between loses between
  const double rho = BSPF::clamp(ab / (std::sqrt(aa * bb) + 1.0), -1.0, 1.0);
  const double floor = 1.0 / (FADE_MAX_GAIN * FADE_MAX_GAIN);

  for(size_t i = 0; i < n; ++i)
  {
    const double w = DBL(i) / DBL(n);
    const double keeps = (1.0 - w) * (1.0 - w) + w * w + 2.0 * rho * w * (1.0 - w);

    out[i] = limit(((1.0 - w) * DBL(a[i]) + w * DBL(b[i])) /
                   std::sqrt(std::max(keeps, floor)));
  }
  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t SpeakJetDSP::alignPhase(sShortSpan tail, sShortSpan piece, size_t over,
                               uInt32 rate, double f0)
{
  // Half a period reaches the matching phase; joining at any other puts a
  // step in the waveform that no crossfade hides
  const size_t reach = std::min(piece.size() > over ? piece.size() - over : 0,
                                SZT(DBL(rate) / f0 / 2.0) + 1);
  size_t at = 0;
  double best = -1e30;

  for(size_t off = 0; off <= reach; ++off)
  {
    double dot = 0.0, energy = 1.0;

    for(size_t i = 0; i < over; i += 2)
    {
      dot += DBL(tail[i]) * DBL(piece[off + i]);
      energy += DBL(piece[off + i]) * DBL(piece[off + i]);
    }

    const double score = dot / std::sqrt(energy);
    if(score > best)
    {
      best = score;
      at = off;
    }
  }
  return at;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetDSP::sustain(Samples& out, sShortSpan piece, uInt32 rate, double f0)
{
  // Both sides are mid-sound, so a plain crossfade joins them without
  // anything that sounds like the sound starting again
  const size_t over = std::min({size_t{rate * SUSTAIN_XFADE_MS / 1000},
                                out.size() / 2, piece.size() / 3});

  const sShortSpan tail{out.end() - I32(over), over};
  const size_t at = alignPhase(tail, piece, over, rate, f0);
  const Samples mixed = crossfade(tail, {piece.begin() + I32(at), over});

  std::ranges::copy(mixed, out.end() - I32(over));
  out.insert(out.end(), piece.begin() + I32(at + over), piece.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetDSP::resample(Samples& samples, uInt32 from, uInt32 to)
{
  if(from == 0 || to == 0 || from == to || samples.empty())
    return;

  const size_t count = samples.size() * to / from;
  Samples out(count, 0);

  for(size_t i = 0; i < count; ++i)
  {
    const double at = DBL(i) * from / to;
    const auto j = SZT(at);
    const double frac = at - DBL(j);

    out[i] = (j + 1 < samples.size())
      ? I16(samples[j] * (1.0 - frac) + samples[j + 1] * frac)
      : samples.back();
  }

  samples = std::move(out);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int16 SpeakJetDSP::limit(double v)
{
  constexpr double FS = 32767.0;
  constexpr double knee = LIMIT_KNEE * FS;
  const double mag = std::abs(v);

  if(mag <= knee)
    return I16(v);

  const double room = FS - knee;
  return I16(std::copysign(knee + room * std::tanh((mag - knee) / room), v));
}
