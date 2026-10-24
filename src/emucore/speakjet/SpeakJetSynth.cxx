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
#include "SpeakJetSynth.hxx"

namespace {
  using namespace SpeakJetChip;

  // Envelope shapes 1 to 3 'cycle' of the way into a period, as measured
  double shapeAt(uInt8 shape, double cycle)
  {
    switch(shape)
    {
      case 1:   return 0.5 + 0.5 * std::cos(2.0 * BSPF::PI_d * cycle);
      case 2:   return cycle;
      default:  return cycle < 0.5 ? 1.0 : 0.0;
    }
  }

  // The grain envelope or pedestal 'tau' seconds into a period
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
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSynth::SpeakJetSynth()
{
  Path& p = myPath;
  const double rate = DBL(RATE);
  p.dt = 1.0 / rate;
  const double w = 2.0 * BSPF::PI_d * LOWPASS_HZ;
  p.w2 = w * w;
  p.wq = w / LOWPASS_Q;
  const auto root = std::sqrt(std::complex<double>(p.wq * p.wq - 4.0 * p.w2));
  p.p1 = (-p.wq + root) / 2.0;
  p.p2 = (-p.wq - root) / 2.0;
  lowpassOver(p, p.dt, p.e, p.g);
  const double k = 2.0 * rate, wc = k * std::tan(BSPF::PI_d * HIGHPASS_HZ / rate);
  p.h0 = k / (k + wc);
  p.h1 = (wc - k) / (k + wc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSynth::reset()
{
  myPhase = {};
  myCycle = 0.0;
  mySince = -1.0;
  myNoisePhase = {};
  myNoiseJitter = {};
  myNoiseHeld = 0.0;
  myNoiseSeed = 1;
  myAt = 0;
  myTick = ~uInt64{0};
  myHeld = myLast = 0.0;
  myLow = {};
  myLowLast = myHighLast = 0.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSynth::restart()
{
  myPhase = {};
  myCycle = 0.0;
  mySince = -1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetSynth::sample(const Registers& regs)
{
  const double rate = DBL(RATE);
  const double f0 = regs.envelopeHz;

  // Each period of the speech grain restarts the formant oscillators; under
  // the other shapes they run on
  if(f0 > 0.0)
  {
    myCycle += f0 / rate;
    if(mySince < 0.0 || myCycle >= 1.0)
    {
      if(mySince >= 0.0)
        myCycle -= 1.0;
      if(regs.enveloped && regs.shape == 0)
        myPhase = {};
      mySince = 0.0;
    }
  }

  // One volume step's amplitude at this master volume
  const double step = VOLUME_STEP * regs.master / 127.0;
  const double tau = mySince / rate;
  double sum = 0.0;
  for(size_t o = 0; o < 3; ++o)
  {
    sum += regs.volume[o] * std::sin(myPhase[o]);
    myPhase[o] += 2.0 * BSPF::PI_d * regs.hz[o] * CLOCK / rate;
  }
  sum *= step;

  // A sounding grain has the chip's pedestal beneath it
  const bool sounding = regs.volume[0] > 0.0 || regs.volume[1] > 0.0 || regs.volume[2] > 0.0;
  double raw = !regs.enveloped ? sum
      : regs.shape != 0 ? shapeAt(regs.shape, myCycle) * sum
      : -grainAt(ENVELOPE, tau) / ENVELOPE_PEAK * sum +
        (sounding ? PEDESTAL_LEVEL * regs.master / 127.0 * grainAt(PEDESTAL, tau) : 0.0);
  if(regs.volume[3] > 0.0 || regs.volume[4] > 0.0)
    raw += NOISE_SHARE * step * noise(regs, tau);
  if(mySince >= 0.0)
    mySince += 1.0;

  return stageStep(raw);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSynth::startsPeriod(double f0) const
{
  return f0 > 0.0 && (mySince < 0.0 || myCycle + f0 / DBL(RATE) >= 1.0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SpeakJetSynth::noise(const Registers& regs, double tau)
{
  // Each oscillator's jitter is drawn afresh every NOISE_HOLD chip samples
  myNoiseHeld += 8192.0 * CLOCK / DBL(RATE);
  if(myNoiseHeld >= DBL(NOISE_HOLD))
  {
    myNoiseHeld -= DBL(NOISE_HOLD);
    for(double& u: myNoiseJitter)
    {
      myNoiseSeed = myNoiseSeed * 1664525U + 1013904223U;
      u = DBL(myNoiseSeed) / 4294967296.0 * 2.0 - 1.0;
    }
  }

  double y = regs.volume[3] * std::sin(myNoisePhase[0]) +
             regs.volume[4] * std::sin(myNoisePhase[1]);
  for(size_t o = 0; o < 2; ++o)
  {
    myNoisePhase[o] += 2.0 * BSPF::PI_d * regs.hz[3 + o] * CLOCK *
                       (1.0 + NOISE_K * regs.distortion * myNoiseJitter[o]) / DBL(RATE);
    myNoisePhase[o] = std::fmod(myNoisePhase[o], 2.0 * BSPF::PI_d);
  }

  // Half enveloped, the level swings with the grain's envelope
  if(regs.noiseHalf)
    y *= HALF_BASE + HALF_DEPTH * grainAt(ENVELOPE, tau) / ENVELOPE_PEAK;
  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSynth::lowpassOver(const Path& p, double t, std::array<double, 4>& e,
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
double SpeakJetSynth::stageStep(double in)
{
  const Path& p = myPath;
  const uInt64 at = myAt++;
  const auto now = U64(std::floor(DBL(at) * CHIP_RATE / DBL(RATE)));
  if(now == myTick)
    advance(p.e, p.g, myLow, myHeld);
  else
  {
    // A hold edge since the last sample: the old value up to it, then the
    // value the chip computes there, between this sample's and the last
    const double into = BSPF::clamp(DBL(now) / CHIP_RATE - DBL(at - 1) * p.dt, 0.0, p.dt);
    std::array<double, 4> e{};
    std::array<double, 2> g{};
    lowpassOver(p, into, e, g);
    advance(e, g, myLow, myHeld);
    myHeld = myLast + (in - myLast) * into / p.dt;
    lowpassOver(p, p.dt - into, e, g);
    advance(e, g, myLow, myHeld);
    myTick = now;
  }
  myLast = in;

  const double h = p.h0 * (myLow[0] - myLowLast) - p.h1 * myHighLast;
  myLowLast = myLow[0];
  myHighLast = h;
  return h;
}
