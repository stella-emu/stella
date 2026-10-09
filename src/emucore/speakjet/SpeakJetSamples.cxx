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
#include <fstream>

#include "FSNode.hxx"
#include "SpeakJet.hxx"
#include "SpeakJetSamples.hxx"

namespace {
  // The Pitch settings the voiced codes were recorded at, close enough
  // together that anything from 40 to 209 is within PITCH_DEADBAND of one
  constexpr std::array<uInt8, 30> ourPitchSet = {{
     24,  26,  28,  30,  32,  34,  36,  38,
     40,  44,  48,  52,  57,  62,  68,  74,  81,  88,
     96, 105, 114, 124, 136, 148, 161, 176, 192, 209,
    228, 248
  }};

  // The sets outside 40 to 209, used only where they are near and present
  constexpr size_t FIRST_ORDINARY = 8, LAST_ORDINARY = 27;
  static_assert(ourPitchSet[FIRST_ORDINARY] == 40 && ourPitchSet[LAST_ORDINARY] == 209);

  // The most of a sample's ends that can be its rise and fall, so a burst is
  // never mistaken for either
  constexpr uInt32 RAMP_MS = 25;

  // How far under a sample's own plateau (-3dB) its ends are still the ramp
  constexpr double RAMP_DROP = 0.708;

  // Under this share of the plateau (-12dB) for good, a voiced sound has
  // ended and what follows is its decay
  constexpr double DECAY_DROP = 0.251;

  // A shorter stretch under it is the sound's own fall, which stays
  constexpr uInt32 DECAY_MIN_MS = 20;

  // The window and hop the level is followed with
  constexpr uInt32 LEVEL_WINDOW_MS = 20, LEVEL_HOP_MS = 2;

  // Recordings below this rate are refused: the chip itself outputs at about
  // 8kHz, and far lower rates leave the 1ms windows used on a recording with
  // no samples at all
  constexpr uInt32 MIN_RATE = 8000;

  string bendDir(const string& path, size_t bend)
  {
    return path + "bend" + (bend < 10 ? "0" : "") + std::to_string(bend) +
           FSNode::PATH_SEPARATOR;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSamples::SpeakJetSamples(const string& path)
  : myPath{path}
{
  // Without a per-Bend layout one flat directory serves every Bend
  myPerBend = FSNode(myPath + "bend05").isDirectory();
  myPerPitch = FSNode(myPath + "pitch114").isDirectory();
  // Not in the initializer list: loadSet() needs the layout found above
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  myFound = loadSet(SpeakJet::DEFAULT_BEND);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SpeakJetSamples::loadSet(uInt8 bend)
{
  if(bend >= mySamples.size() || myLoaded[bend])
    return 0;

  myLoaded[bend] = true;

  return loadInto(mySamples[bend], myPerBend ? bendDir(myPath, bend) : myPath,
                  SpeakJet::FIRST_SOUND);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SpeakJetSamples::loadInto(SampleSet& set, const string& dir, uInt8 only)
{
  uInt32 found = 0;

  for(uInt32 i = U32(only - SpeakJet::FIRST_SOUND); i < set.size(); ++i)
  {
    uInt32 rate = 0;
    Samples samples;

    const auto code = U8(SpeakJet::FIRST_SOUND + i);
    const string name = dir + string{SpeakJet::nameOf(code)};

    if(!readWav(name + ".wav", samples, rate))
      continue;

    const Samples cut = trimSilence(samples);
    if(samples.empty())
      continue;

    // Whatever the trim took is already decay, as is any the body still holds
    Samples decay = SpeakJet::isContinuant(code) ? decayTail(samples, rate) : Samples{};
    decay.insert(decay.end(), cut.begin(), cut.end());
    Samples rest;
    uInt32 decayRate = 0;

    if(readWav(name + ".decay.wav", rest, decayRate) && decayRate == rate)
      decay.insert(decay.end(), rest.begin(), rest.end());

    // The first sample loaded sets the rate the rest are matched to
    if(myRate == 0)
      myRate = rate;
    else if(rate != myRate)
    {
      SpeakJetDSP::resample(samples, rate, myRate);
      SpeakJetDSP::resample(decay, rate, myRate);
    }

    set[i] = Clip{std::move(samples), std::move(decay)};
    findRamps(set[i], myRate);
    ++found;
  }

  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpeakJetSamples::SampleSet& SpeakJetSamples::setFor(uInt8 bend)
{
  const uInt8 want = std::min<uInt8>(bend, U8(mySamples.size() - 1));

  if(!myLoaded[want])
    loadSet(want);

  return mySamples[want][0].body.empty() && mySamples[want][1].body.empty()
      ? mySamples[SpeakJet::DEFAULT_BEND] : mySamples[want];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpeakJetSamples::Clip* SpeakJetSamples::pitchClip(uInt8 code, uInt8 pitch,
                                                        uInt8 bend, uInt8& atPitch)
{
  if(!myPerPitch)
    return nullptr;

  const size_t atBend = std::min<size_t>(bend, myPitchSamples.size() - 1);

  size_t want = 0;
  for(size_t i = 1; i < ourPitchSet.size(); ++i)
    if(std::abs(DBL(ourPitchSet[i]) / DBL(pitch) - 1.0) <
       std::abs(DBL(ourPitchSet[want]) / DBL(pitch) - 1.0))
      want = i;

  // Outside 40 to 209 the nearest ordinary set is shifted instead, unless a
  // set close enough is there
  if((want < FIRST_ORDINARY || want > LAST_ORDINARY) &&
     (std::abs(DBL(ourPitchSet[want]) / DBL(pitch) - 1.0) > SpeakJetDSP::PITCH_DEADBAND ||
      pitchSetAt(atBend, want)[code - SpeakJet::FIRST_SOUND].body.empty()))
    want = want < FIRST_ORDINARY ? FIRST_ORDINARY : LAST_ORDINARY;

  const Clip& clip = pitchSetAt(atBend, want)[code - SpeakJet::FIRST_SOUND];

  if(clip.body.empty())
    return nullptr;

  atPitch = ourPitchSet[want];
  return &clip;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpeakJetSamples::SampleSet& SpeakJetSamples::pitchSet(uInt8 bend, uInt8 pitch)
{
  // Nothing is recorded at a Pitch that isn't one of the sampled settings
  static const SampleSet none{};

  const auto want = SZT(std::ranges::find(ourPitchSet, pitch) - ourPitchSet.begin());
  if(want == ourPitchSet.size())
    return none;

  return pitchSetAt(std::min<size_t>(bend, myPitchSamples.size() - 1), want);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SpeakJetSamples::SampleSet& SpeakJetSamples::pitchSetAt(size_t bend, size_t want)
{
  if(!myPitchLoaded[bend][want])
  {
    myPitchLoaded[bend][want] = true;
    const string dir = bend == SpeakJet::DEFAULT_BEND ? myPath : bendDir(myPath, bend);
    loadInto(myPitchSamples[bend][want], dir + "pitch" +
             std::to_string(ourPitchSet[want]) + FSNode::PATH_SEPARATOR,
             SpeakJet::FIRST_SOUND);
  }

  return myPitchSamples[bend][want];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSamples::findRamps(Clip& clip, uInt32 rate)
{
  const size_t window = rate * LEVEL_WINDOW_MS / 1000;
  const size_t hop = rate * LEVEL_HOP_MS / 1000;

  clip.head = clip.tail = 0;
  if(hop == 0 || clip.body.size() < window + hop)
    return;

  vector<double> level;
  for(size_t at = 0; at + window <= clip.body.size(); at += hop)
  {
    double sum = 0.0;
    for(size_t i = at; i < at + window; ++i)
      sum += DBL(clip.body[i]) * DBL(clip.body[i]);
    level.push_back(std::sqrt(sum / DBL(window)));
  }

  // The plateau the sound holds at, taken above the middle so a sound that
  // spends half its length arriving does not set it too low
  vector<double> sorted{level};
  std::ranges::sort(sorted);
  const double want = sorted[sorted.size() * 3 / 4] * RAMP_DROP;

  size_t first = 0, last = level.size() - 1;
  while(first < level.size() && level[first] < want)
    ++first;
  while(last > first && level[last] < want)
    --last;

  const size_t most = std::min(size_t{rate * RAMP_MS / 1000},
                               clip.body.size() / RAMP_DIVISOR);
  clip.head = std::min(first * hop, most);
  clip.tail = std::min(clip.body.size() - (last * hop + window), most);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSamples::Samples SpeakJetSamples::trimSilence(Samples& samples)
{
  if(samples.empty())
    return {};

  // Relative to this file's own peak, so a quiet fricative is not erased
  Int16 peak = 0;
  for(const Int16 s: samples)
    peak = std::max(peak, I16(std::abs(s)));

  const Int16 floor = std::max(I16(64), I16(peak / 50));
  size_t first = 0, last = samples.size();

  while(first < samples.size() && std::abs(samples[first]) < floor)
    ++first;
  while(last > first && std::abs(samples[last - 1]) < floor)
    --last;

  Samples cut{samples.begin() + I32(last), samples.end()};
  samples = Samples(samples.begin() + I32(first), samples.begin() + I32(last));

  return cut;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSamples::Samples SpeakJetSamples::decayTail(Samples& samples, uInt32 rate)
{
  const size_t window = rate * LEVEL_WINDOW_MS / 1000;
  const size_t hop = rate * LEVEL_HOP_MS / 1000;
  if(hop == 0 || samples.size() < window + hop)
    return {};

  vector<double> level;
  for(size_t at = 0; at + window <= samples.size(); at += hop)
  {
    double sum = 0.0;
    for(size_t i = at; i < at + window; ++i)
      sum += DBL(samples[i]) * DBL(samples[i]);
    level.push_back(std::sqrt(sum / DBL(window)));
  }

  // The plateau as findRamps() takes it, and the last window still near it
  vector<double> sorted{level};
  std::ranges::sort(sorted);
  const double want = sorted[sorted.size() * 3 / 4] * DECAY_DROP;
  size_t last = level.size() - 1;
  while(last > 0 && level[last] < want)
    --last;

  const size_t end = std::min(samples.size(), last * hop + window);
  if(samples.size() - end < rate * DECAY_MIN_MS / 1000)
    return {};

  Samples tail{samples.begin() + I64(end), samples.end()};
  samples.resize(end);
  return tail;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSamples::readWav(const string& path, Samples& samples, uInt32& rate)
{
  std::ifstream in(path, std::ios::binary);
  if(!in)
    return false;

  const vector<char> buf{std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>()};
  if(buf.size() < 44)
    return false;

  const auto u16 = [&buf](size_t o) {
    return U16(U8(buf[o]) | (U32(U8(buf[o + 1])) << 8U));
  };
  const auto u32 = [&buf](size_t o) {
    return U32(U8(buf[o]) | (U32(U8(buf[o + 1])) << 8U) |
               (U32(U8(buf[o + 2])) << 16U) | (U32(U8(buf[o + 3])) << 24U));
  };

  if(string(buf.data(), 4) != "RIFF" || string(buf.data() + 8, 4) != "WAVE")
    return false;

  size_t pos = 12;
  uInt16 channels = 0, bits = 0;
  bool haveFmt = false;

  while(pos + 8 <= buf.size())
  {
    const string id(buf.data() + pos, 4);
    const size_t size = u32(pos + 4);
    const size_t body = pos + 8;

    if(id == "fmt " && size >= 16 && body + 16 <= buf.size())
    {
      // Uncompressed PCM only
      if(u16(body) != 1)
        return false;

      channels = u16(body + 2);
      rate = u32(body + 4);
      bits = u16(body + 14);
      haveFmt = channels > 0 && rate >= MIN_RATE && (bits == 8 || bits == 16);
    }
    else if(id == "data" && haveFmt)
    {
      const size_t bytes = std::min(size, buf.size() - body);
      const size_t stride = size_t{channels} * (bits / 8);
      const size_t frames = stride ? bytes / stride : 0;

      samples.clear();
      samples.reserve(frames);

      for(size_t f = 0; f < frames; ++f)
      {
        Int32 sum = 0;
        for(uInt16 ch = 0; ch < channels; ++ch)
        {
          const size_t o = body + f * stride + size_t{ch} * (bits / 8);
          sum += (bits == 8) ? (Int32{U8(buf[o])} - 128) * 256
                             : I32(I16(u16(o)));
        }
        samples.push_back(I16(sum / channels));
      }
      return !samples.empty();
    }

    pos = body + size + (size & 1U);
  }

  return false;
}
