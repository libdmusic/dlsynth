#include "Voice.hpp"
#include "../NumericUtils.hpp"
#include "EG.hpp"
#include "LFO.hpp"
#include <atomic>
#include <map>
#include <mutex>
#include <unordered_set>

using namespace DLSynth;
using namespace DLSynth::Synth;

namespace std {
template <> struct hash<ConnectionBlock> {
  constexpr std::size_t operator()(const ConnectionBlock &s) const noexcept {
    return ((std::size_t)s.source() << 0) | ((std::size_t)s.control() << 16) |
           ((std::size_t)s.destination() << 32);
  }
};

template <> struct equal_to<ConnectionBlock> {
  constexpr bool operator()(const ConnectionBlock &lhs,
                            const ConnectionBlock &rhs) const noexcept {
    return lhs.control() == rhs.control() &&
           lhs.destination() == rhs.destination() &&
           lhs.source() == rhs.source();
  }
};
} // namespace std

#define DLSYNTH_DEFAULT_CONN(src, ctrl, dst, scale, bip, inv, type)            \
  ConnectionBlock(Source::src, Source::ctrl, Destination::dst, scale,          \
                  Transform(inv, bip, TransformType::type),                    \
                  Transform(false, false, TransformType::None))

static float concaveTransform(float value) {
  static float threshold = 1.f - std::pow(10, -12.f / 5.f);
  if (value > threshold) {
    return 1.f;
  } else {
    return -(5.f / 12.f) * std::log10(1.f - value);
  }
}

static float convexTransform(float value) {
  static float threshold = std::pow(10, -12.f / 5.f);
  if (value < threshold) {
    return 0;
  } else {
    return 1.f + (5.f / 12.f) * std::log10(value);
  }
}

static float switchTransform(float value) {
  if (value < .5f) {
    return 0;
  } else {
    return 1;
  }
}

inline float sgn(float x) {
  if (x < 0)
    return -1;
  else
    return 1;
}

static float applyTransform(float input, const Transform &trans) {
  if (trans.invert()) {
    input = 1.f - input;
  }
  switch (trans.type()) {
  case TransformType::None:
    if (trans.bipolar()) {
      return 2.f * input - 1.f;
    } else {
      return input;
    }
  case TransformType::Switch:
    if (trans.bipolar()) {
      float value = switchTransform(input);
      return 2.f * input - 1.f;
    } else {
      return switchTransform(input);
    }
  case TransformType::Concave:
    if (trans.bipolar()) {
      float value = 2.f * input - 1.f;
      return sgn(value) * concaveTransform(std::abs(value));
    } else {
      return concaveTransform(input);
    }
  case TransformType::Convex:
    if (trans.bipolar()) {
      float value = 2.f * input - 1.f;
      return sgn(value) * convexTransform(std::abs(value));
    } else {
      return convexTransform(input);
    }
  }
}

static float getScaleValue(Destination dest, std::int32_t scale) {
  switch (dest) {
  case Destination::FilterCutoff:
  case Destination::LfoFrequency:
  case Destination::Pitch:
  case Destination::VibratoFrequency:
  case Destination::KeyNumber:
    return absolutePitchToCents(scale);
  case Destination::EG1AttackTime:
  case Destination::EG1DecayTime:
  case Destination::EG1DelayTime:
  case Destination::EG1HoldTime:
  case Destination::EG1ReleaseTime:
  case Destination::EG1ShutdownTime:
  case Destination::EG2AttackTime:
  case Destination::EG2DecayTime:
  case Destination::EG2DelayTime:
  case Destination::EG2HoldTime:
  case Destination::EG2ReleaseTime:
  case Destination::LfoStartDelay:
  case Destination::VibratoStartDelay:
    return timeUnitsToCents(scale);
  case Destination::FilterQ:
  case Destination::Gain:
    return relativeGainUnitsToBels(scale);
  case Destination::Chorus:
  case Destination::EG1SustainLevel:
  case Destination::EG2SustainLevel:
  case Destination::Pan:
  case Destination::Reverb:
    return percentUnitsToRatio(scale);
  default:
    return 0;
  }
}

inline float lerp(float v0, float v1, float t) { return (1 - t) * v0 + t * v1; }

static float interpolateSample(float pos, const std::vector<float> samples) {
  std::size_t floor = (std::size_t)std::floor(pos);

  float sample1 = samples[floor];
  float sample2 = samples[floor + 1];

  float t = pos - floor;
  return lerp(sample1, sample2, t);
}

inline float centsToRatio(float cents) { return std::exp2(cents / 1200.f); }

inline float centsToFreq(float cents) {
  return centsToRatio(cents - 6900) * 440.f;
}

inline float belsToGain(float bels) { return std::pow(10, bels / 2); }

inline float centsToSecs(float cents) { return std::exp2(cents / 1200); }

struct VoiceParams {
  std::unordered_set<ConnectionBlock> m_connections;
  std::map<Source, float> m_sources;
  std::map<Destination, float> m_destinations;
  const std::map<Source, float> &m_instrSources;
  bool m_playing;
  std::uint8_t m_note;
  std::uint8_t m_velocity;
  std::chrono::steady_clock::time_point m_startTime;

  const Wave *m_sample = nullptr;
  const Wavesample *m_wavesample = nullptr;
  float m_samplePos = 0;

  LFO m_modLfo, m_vibLfo;
  EG m_volEg, m_filtEg;

  VoiceParams(const std::map<Source, float> &instrSources,
              std::uint32_t sampleRate)
    : m_instrSources(instrSources)
    , m_modLfo(sampleRate)
    , m_vibLfo(sampleRate)
    , m_volEg(sampleRate)
    , m_filtEg(sampleRate) {
    resetConnections();
    resetDestinations();
  }

  float getSource(Source source) {
    if (m_sources.find(source) != m_sources.end()) {
      return m_sources.at(source);
    } else if (m_instrSources.find(source) != m_instrSources.end()) {
      return m_instrSources.at(source);
    } else {
      return 0;
    }
  }

  void calcDestinations() {
    for (const auto &connection : m_connections) {
      if (m_destinations.find(connection.destination()) ==
          m_destinations.end()) {
        continue;
      }
      const auto &srcTrans = connection.sourceTransform();
      const auto &ctrlTrans = connection.controlTransform();

      float source = applyTransform(getSource(connection.source()), srcTrans);
      float control =
       applyTransform(getSource(connection.control()), ctrlTrans);
      float destination = m_destinations.at(connection.destination());

      destination +=
       source * control *
       getScaleValue(connection.destination(), connection.scale());
    }
  }

  void resetConnections() {
    m_connections = {
     DLSYNTH_DEFAULT_CONN(None, None, LfoFrequency, -55791973, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, LfoStartDelay, -522494112, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, VibratoFrequency, -55791973, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, VibratoStartDelay, -522494112, false,
                          false, None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1DelayTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1AttackTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1HoldTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1DecayTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1SustainLevel, 65536000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1ReleaseTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG1ShutdownTime, -476490789, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyOnVelocity, None, EG1AttackTime, 0, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, EG1DecayTime, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, EG1HoldTime, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2DelayTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2AttackTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2HoldTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2DecayTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2SustainLevel, 65536000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, EG2ReleaseTime, 0x80000000, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyOnVelocity, None, EG2AttackTime, 0, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, EG2DecayTime, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, EG2HoldTime, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, KeyNumber, 838860800, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(RPN2, None, KeyNumber, 419430400, true, false, None),
     DLSYNTH_DEFAULT_CONN(None, None, FilterCutoff, 0x7FFFFFFF, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(None, None, FilterQ, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, None, FilterCutoff, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, CC1, FilterCutoff, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, ChannelPressure, FilterCutoff, 0, true, false,
                          None),
     DLSYNTH_DEFAULT_CONN(EG2, None, FilterCutoff, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(KeyOnVelocity, None, FilterCutoff, 0, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, FilterCutoff, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, None, Gain, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, CC1, Gain, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, ChannelPressure, Gain, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(KeyOnVelocity, None, Gain, 0x80000000, false, true,
                          None),
     DLSYNTH_DEFAULT_CONN(CC7, None, Gain, 0x80000000, false, true, None),
     DLSYNTH_DEFAULT_CONN(CC11, None, Gain, 0x80000000, false, true, None),
     DLSYNTH_DEFAULT_CONN(None, None, Pan, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(CC10, None, Pan, 33292288, true, false, None),
     DLSYNTH_DEFAULT_CONN(CC91, None, Reverb, 65536000, false, false, None),
     DLSYNTH_DEFAULT_CONN(None, None, Reverb, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(CC93, None, Chorus, 65536000, false, false, None),
     DLSYNTH_DEFAULT_CONN(None, None, Chorus, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(None, None, Pitch, 0, false, false, None),
     DLSYNTH_DEFAULT_CONN(PitchWheel, RPN0, Pitch, 838860800, true, false,
                          None),
     DLSYNTH_DEFAULT_CONN(KeyNumber, None, Pitch, 838860800, false, false,
                          None),
     DLSYNTH_DEFAULT_CONN(RPN1, None, Pitch, 6553600, false, false, None),
     DLSYNTH_DEFAULT_CONN(Vibrato, None, Pitch, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(Vibrato, CC1, Pitch, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(Vibrato, ChannelPressure, Pitch, 0, true, false,
                          None),
     DLSYNTH_DEFAULT_CONN(LFO, None, Pitch, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, CC1, Pitch, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(LFO, ChannelPressure, Pitch, 0, true, false, None),
     DLSYNTH_DEFAULT_CONN(EG2, None, Pitch, 0, false, false, None),
    };
  }

  void loadConnections(const std::vector<ConnectionBlock> &cblocks) {
    for (const auto &block : cblocks) {
      auto old_elem = m_connections.find(block);
      if (old_elem == m_connections.end()) {
        m_connections.insert(block);
      } else {
        m_connections.erase(old_elem);
        m_connections.insert(block);
      }
    }
  }

  void resetDestinations() {
    m_destinations = {{Destination::Gain, 0.f},
                      {Destination::Pitch, 0.f},
                      {Destination::Pan, 0.f},
                      {Destination::KeyNumber, 0.f},
                      {Destination::LfoFrequency, 0.f},
                      {Destination::LfoStartDelay, 0.f},
                      {Destination::VibratoFrequency, 0.f},
                      {Destination::VibratoStartDelay, 0.f},
                      {Destination::EG1AttackTime, 0.f},
                      {Destination::EG1DecayTime, 0.f},
                      {Destination::EG1Reserved, 0.f},
                      {Destination::EG1ReleaseTime, 0.f},
                      {Destination::EG1SustainLevel, 0.f},
                      {Destination::EG1DelayTime, 0.f},
                      {Destination::EG1HoldTime, 0.f},
                      {Destination::EG1ShutdownTime, 0.f},
                      {Destination::EG2AttackTime, 0.f},
                      {Destination::EG2DecayTime, 0.f},
                      {Destination::EG2Reserved, 0.f},
                      {Destination::EG2ReleaseTime, 0.f},
                      {Destination::EG2SustainLevel, 0.f},
                      {Destination::EG2DelayTime, 0.f},
                      {Destination::EG2HoldTime, 0.f},
                      {Destination::FilterCutoff, 0.f},
                      {Destination::FilterQ, 0.f}};
  }

  void updateModLfo() {
    float freq = centsToFreq(m_destinations.at(Destination::LfoFrequency));
    float startDelay =
     centsToSecs(m_destinations.at(Destination::LfoStartDelay));

    float output = m_modLfo.nextSample(freq, startDelay);

    m_sources.at(Source::LFO) = output;
  }

  void updateVibLfo() {
    float freq = centsToFreq(m_destinations.at(Destination::VibratoFrequency));
    float startDelay =
     centsToSecs(m_destinations.at(Destination::VibratoStartDelay));

    float output = m_vibLfo.nextSample(freq, startDelay);

    m_sources.at(Source::Vibrato) = output;
  }

  void updateVolEg() {
    float delay = centsToSecs(m_destinations.at(Destination::EG1DelayTime));
    float attack = centsToSecs(m_destinations.at(Destination::EG1AttackTime));
    float hold = centsToSecs(m_destinations.at(Destination::EG1HoldTime));
    float decay = centsToSecs(m_destinations.at(Destination::EG1DecayTime));
    float sustain = m_destinations.at(Destination::EG1SustainLevel);
    float release = centsToSecs(m_destinations.at(Destination::EG1ReleaseTime));

    float output = 2 * std::log10(m_volEg.nextSample(delay, attack, hold, decay,
                                                     sustain, release));

    if (!m_volEg.isActive()) {
      m_playing = false;
    }

    m_sources.at(Source::EG1) = output;
  }

  void updateFiltEg() {
    float delay = centsToSecs(m_destinations.at(Destination::EG2DelayTime));
    float attack = centsToSecs(m_destinations.at(Destination::EG2AttackTime));
    float hold = centsToSecs(m_destinations.at(Destination::EG2HoldTime));
    float decay = centsToSecs(m_destinations.at(Destination::EG2DecayTime));
    float sustain = m_destinations.at(Destination::EG2SustainLevel);
    float release = centsToSecs(m_destinations.at(Destination::EG2ReleaseTime));

    float output =
     m_filtEg.nextSample(delay, attack, hold, decay, sustain, release);

    m_sources.at(Source::EG2) = output;
  }

  void render_mix(float *beginLeft, float *endLeft, float *beginRight,
                  float *endRight) {
    float *lp = beginLeft;
    float *rp = beginRight;

    if (m_sample == nullptr || m_wavesample == nullptr || !m_playing) {
      m_playing = false;
      return;
    }

    while (lp < endLeft || rp < endRight) {
      calcDestinations();
      updateVolEg();
      updateFiltEg();
      updateModLfo();
      updateVibLfo();

      float pan = m_destinations.at(Destination::Pan);
      float gain = m_destinations.at(Destination::Gain);

      float leftGain =
       2 * std::log10(std::cos((PI / 2.f) * (pan + 0.5f))) + gain;
      float rightGain =
       2 * std::log10(std::sin((PI / 2.f) * (pan + 0.5f))) + gain;

      float pitch = m_destinations.at(Destination::Pitch);
      float samplePitch = m_wavesample->unityNote() * 100.f;
      float freqRatio = centsToRatio(samplePitch - pitch);

      float lsample = interpolateSample(m_samplePos, m_sample->leftData()) *
                      belsToGain(leftGain);
      float rsample = interpolateSample(m_samplePos, m_sample->rightData()) *
                      belsToGain(rightGain);

      m_samplePos += freqRatio;
      const WavesampleLoop *loop = m_wavesample->loop();

      if (loop != nullptr) {
        float loopStart = loop->start();
        float loopLength = loop->length();
        float loopEnd = loopStart + loopEnd;

        if (loop->type() == LoopType::Forward) {
          if (m_samplePos > loopEnd) {
            m_samplePos -= loopLength;
          }
        } else {
          if (m_playing) {
            if (m_samplePos > loopEnd) {
              m_samplePos -= loopLength;
            }
          } else {
            if (m_samplePos > m_sample->leftData().size()) {
              m_playing = false;
            }
          }
        }
      }

      if (lp < endLeft) {
        *lp = lsample;
        lp++;
      }

      if (rp < endRight) {
        *rp = rsample;
        rp++;
      }
    }
  }

  void render_fill(float *beginLeft, float *endLeft, float *beginRight,
                   float *endRight) {
    std::fill(beginLeft, endLeft, 0.f);
    std::fill(beginRight, endRight, 0.f);
    render_mix(beginLeft, endLeft, beginRight, endRight);
  }

  void noteOff() {
    m_volEg.noteOff();
    m_filtEg.noteOff();
  }
};

struct Voice::impl {
  const Instrument &m_instrument;
  const std::map<Source, float> &m_instrSources;
  std::uint32_t m_sampleRate;

  VoiceParams m_paramSet1;
  VoiceParams m_paramSet2;

  std::mutex m_currentParamsMtx;
  std::atomic<VoiceParams *> m_currentParams;
  std::atomic<VoiceParams *> m_newParams;

  impl(const Instrument &instr, const std::map<Source, float> &sources,
       std::uint32_t sampleRate)
    : m_instrument(instr)
    , m_instrSources(sources)
    , m_sampleRate(sampleRate)
    , m_paramSet1(sources, sampleRate)
    , m_paramSet2(sources, sampleRate)
    , m_currentParams(&m_paramSet1)
    , m_newParams(&m_paramSet2) {}
};

Voice::Voice(const Instrument &instrument,
             const std::map<Source, float> &sources, std::uint32_t sampleRate)
  : pimpl(new impl(instrument, sources, sampleRate)) {}

Voice::Voice(Voice &&voice) : pimpl(voice.pimpl) { voice.pimpl = nullptr; }

Voice::~Voice() {
  if (pimpl != nullptr) {
    delete pimpl;
  }
}

void Voice::noteOn(std::uint8_t note, std::uint8_t velocity,
                   const Wavesample *wavesample, const Wave &sample) {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  while (true) {
    VoiceParams *reusedData = pimpl->m_newParams;
    if (reusedData == nullptr) {
      continue;
    }
    pimpl->m_newParams = nullptr;

    reusedData->m_playing = true;
    reusedData->resetConnections();
    reusedData->resetDestinations();
    reusedData->m_wavesample = wavesample;
    reusedData->m_sample = &sample;
    reusedData->m_samplePos = 0;
    reusedData->m_note = note;
    reusedData->m_velocity = velocity;
    reusedData->m_startTime = std::chrono::steady_clock::now();
    reusedData->m_sources[Source::KeyNumber] = (float)note / 128.f;
    reusedData->m_sources[Source::KeyOnVelocity] = (float)velocity / 128.f;

    pimpl->m_currentParams = reusedData;
  }
}
void Voice::noteOff() {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  VoiceParams *params = pimpl->m_currentParams;
  params->noteOff();
}
void Voice::soundOff() {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  while (true) {
    VoiceParams *reusedData = pimpl->m_newParams;
    if (reusedData == nullptr) {
      continue;
    }
    pimpl->m_newParams = nullptr;

    reusedData->m_playing = false;
    reusedData->m_wavesample = nullptr;
    reusedData->m_sample = nullptr;

    pimpl->m_currentParams = reusedData;
  }
}

bool Voice::playing() const {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  VoiceParams *params = pimpl->m_currentParams;
  return params->m_playing;
}
std::uint8_t Voice::note() const {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  VoiceParams *params = pimpl->m_currentParams;
  return params->m_note;
}
std::chrono::steady_clock::time_point Voice::startTime() const {
  std::lock_guard<std::mutex> guard(pimpl->m_currentParamsMtx);
  VoiceParams *params = pimpl->m_currentParams;
  return params->m_startTime;
}
