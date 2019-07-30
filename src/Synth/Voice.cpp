#include "Voice.hpp"
#include "../NumericUtils.hpp"
#include "EG.hpp"
#include "LFO.hpp"
#include "SPSCQueue.hpp"
#include "VoiceMessage.hpp"
#include <unordered_set>

using namespace DLSynth;
using namespace DLSynth::Synth;

namespace std {
template <> struct hash<ConnectionBlock> {
  constexpr std::size_t operator()(const ConnectionBlock &s) const noexcept {
    return (static_cast<std::size_t>(s.source()) << 0) |
           (static_cast<std::size_t>(s.control()) << 16) |
           (static_cast<std::size_t>(s.destination()) << 32);
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
  static float threshold = 1.f - std::pow(10.f, -12.f / 5.f);
  if (value > threshold) {
    return 1.f;
  } else {
    return -(5.f / 12.f) * std::log10(1.f - value);
  }
}

static float convexTransform(float value) {
  static float threshold = std::pow(10.f, -12.f / 5.f);
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
  default:
    return input;
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

static float interpolateSample(float pos, const std::vector<float> &samples) {
  std::size_t floor = static_cast<std::size_t>(std::floor(pos));
  std::size_t ceil = static_cast<std::size_t>(std::ceil(pos));

  float sample1 = samples[floor];
  float sample2 = samples[ceil % samples.size()];

  float t = pos - floor;
  return lerp(sample1, sample2, t);
}

inline float centsToRatio(float cents) { return std::exp2(cents / 1200.f); }

inline float centsToFreq(float cents) {
  return centsToRatio(cents - 6900) * 440.f;
}

inline float belsToGain(float bels) { return std::pow(10.f, bels); }

inline float centsToSecs(float cents) { return std::exp2(cents / 1200); }

using SourceArray = std::array<float, DLSynth::max_source + 1>;
using DestinationArray = std::array<float, DLSynth::max_destination + 1>;

constexpr auto globalSources = {Source::CC1,
                                Source::CC10,
                                Source::CC11,
                                Source::CC7,
                                Source::CC91,
                                Source::CC93,
                                Source::ChannelPressure,
                                Source::None,
                                Source::PitchWheel,
                                Source::RPN0,
                                Source::RPN1,
                                Source::RPN2};

constexpr std::size_t messageQueueSize = 128;

struct Voice::impl : public VoiceMessageExecutor {
  impl(const Instrument &instr, const SourceArray &sources,
       std::uint32_t sampleRate)
    : m_instrument(instr)
    , m_instrSources(sources)
    , m_sampleRate(sampleRate)
    , m_modLfo(static_cast<float>(sampleRate))
    , m_vibLfo(static_cast<float>(sampleRate))
    , m_volEg(static_cast<float>(sampleRate))
    , m_filtEg(static_cast<float>(sampleRate))
    , m_messageQueue(messageQueueSize) {
    for (std::uint16_t i = 0; i < max_source + 1; i++) {
      m_sourceMap[i] = &(m_sources[i]);
    }

    for (Source src : globalSources) {
      m_sourceMap[static_cast<std::uint16_t>(src)] = &getGlobalSource(src);
    }

    resetConnections();
    resetDestinations();

    getInternalSource(Source::EG1) = 0.f;
    getInternalSource(Source::EG2) = 0.f;
    getInternalSource(Source::LFO) = 0.5f;
    getInternalSource(Source::Vibrato) = 0.5f;
  }

  ~impl() override = default;

  const Instrument &m_instrument;
  const SourceArray &m_instrSources;
  std::uint32_t m_sampleRate;
  rigtorp::SPSCQueue<std::unique_ptr<VoiceMessage>> m_messageQueue;

  std::unordered_set<ConnectionBlock> m_connections;
  SourceArray m_sources{0};
  DestinationArray m_destinations;
  std::array<const float *, DLSynth::max_source + 1> m_sourceMap;
  bool m_playing;
  std::uint8_t m_note;
  std::uint8_t m_velocity;
  std::chrono::steady_clock::time_point m_startTime;

  const Wave *m_sample = nullptr;
  const Wavesample *m_wavesample = nullptr;
  float m_samplePos = 0;

  LFO m_modLfo, m_vibLfo;
  EG m_volEg, m_filtEg;

  inline float getSource(Source source) {
    return *(m_sourceMap[static_cast<std::uint16_t>(source)]);
  }

  inline const float &getGlobalSource(Source source) {
    return m_instrSources[static_cast<std::uint16_t>(source)];
  }

  inline float &getInternalSource(Source source) {
    return m_sources[static_cast<std::uint16_t>(source)];
  }

  inline float &getDestination(Destination dest) {
    return m_destinations[static_cast<std::uint16_t>(dest)];
  }

  void calcDestinations() {
    for (const auto &connection : m_connections) {
      Source src = connection.source();
      Source ctrl = connection.control();
      Destination dst = connection.destination();

      const auto &srcTrans = connection.sourceTransform();
      const auto &ctrlTrans = connection.controlTransform();

      float source = applyTransform(getSource(src), srcTrans);
      float control = applyTransform(getSource(ctrl), ctrlTrans);
      float scaleValue = getScaleValue(dst, connection.scale());

      getDestination(dst) += source * control * scaleValue;
      continue;
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
     DLSYNTH_DEFAULT_CONN(KeyOnVelocity, None, Gain, -62914560, false, true,
                          Concave),
     DLSYNTH_DEFAULT_CONN(CC7, None, Gain, -62914560, false, true, Concave),
     DLSYNTH_DEFAULT_CONN(CC11, None, Gain, -62914560, false, true, Concave),
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
     DLSYNTH_DEFAULT_CONN(EG1, None, Gain, -62914560, false, true, None),
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
    std::fill(std::begin(m_destinations), std::end(m_destinations), 0.f);
  }

  void updateModLfo() {
    float freq = centsToFreq(getDestination(Destination::LfoFrequency));
    float startDelay = centsToSecs(getDestination(Destination::LfoStartDelay));

    float output = m_modLfo.nextSample(freq, startDelay);

    getInternalSource(Source::LFO) = output;
  }

  void updateVibLfo() {
    float freq = centsToFreq(getDestination(Destination::VibratoFrequency));
    float startDelay =
     centsToSecs(getDestination(Destination::VibratoStartDelay));

    float output = m_vibLfo.nextSample(freq, startDelay);

    getInternalSource(Source::Vibrato) = output;
  }

  void updateVolEg() {
    float delay = centsToSecs(getDestination(Destination::EG1DelayTime));
    float attack = centsToSecs(getDestination(Destination::EG1AttackTime));
    float hold = centsToSecs(getDestination(Destination::EG1HoldTime));
    float decay = centsToSecs(getDestination(Destination::EG1DecayTime));
    float sustain = getDestination(Destination::EG1SustainLevel);
    float release = centsToSecs(getDestination(Destination::EG1ReleaseTime));

    float output =
     m_volEg.nextSample(delay, attack, hold, decay, sustain, release);

    bool gate = m_volEg.gate();
    if (!gate && output == 0.f) {
      m_playing = false;
    }

    getInternalSource(Source::EG1) = output;
  }

  void updateFiltEg() {
    float delay = centsToSecs(getDestination(Destination::EG2DelayTime));
    float attack = centsToSecs(getDestination(Destination::EG2AttackTime));
    float hold = centsToSecs(getDestination(Destination::EG2HoldTime));
    float decay = centsToSecs(getDestination(Destination::EG2DecayTime));
    float sustain = getDestination(Destination::EG2SustainLevel);
    float release = centsToSecs(getDestination(Destination::EG2ReleaseTime));

    float output =
     m_filtEg.nextSample(delay, attack, hold, decay, sustain, release);

    getInternalSource(Source::EG2) = output;
  }

  void execute(const NoteOnMessage &message) override {
    m_playing = true;
    resetConnections();
    resetDestinations();
    loadConnections(message.connectionBlocks());
    m_wavesample = message.wavesample();
    m_sample = &message.sample();
    m_samplePos = 0;
    m_note = message.note();
    m_velocity = message.velocity();
    m_startTime = std::chrono::steady_clock::now();
    getInternalSource(Source::KeyNumber) = static_cast<float>(m_note) / 128.f;
    getInternalSource(Source::KeyOnVelocity) =
     static_cast<float>(m_velocity) / 128.f;
    m_filtEg.noteOn();
    m_modLfo.reset();
    m_vibLfo.reset();
    m_volEg.noteOn();
  }

  void execute(const NoteOffMessage &message) override {
    m_volEg.noteOff();
    m_filtEg.noteOff();
  }

  void execute(const SoundOffMessage &message) override {
    m_wavesample = nullptr;
    m_sample = nullptr;
    m_playing = false;
  }

  void execute(const PolyPressureMessage &message) override {
    getInternalSource(Source::PolyPressure) =
     static_cast<float>(message.value()) / 128.f;
  }

  void render(float *beginLeft, float *endLeft, float *beginRight,
              float *endRight, float outGain, bool fill,
              std::size_t bufferSkip) {
    float *lp = beginLeft;
    float *rp = beginRight;

    /*
    Calculating destinations is expensive, so it cannot be done for every
    sample. Instead, `destinationCalcInterval` specifies how many samples are
    generated using the same destination values, to reduce CPU load.
     */
    int count = 0;
    constexpr int destinationCalcInterval = 16;

    while (lp < endLeft || rp < endRight) {
      if (lp < endLeft && fill) {
        *lp = 0.f;
      }

      if (rp < endRight && fill) {
        *rp = 0.f;
      }

      while (m_messageQueue.front() != nullptr) {
        auto msg = std::move(*m_messageQueue.front());
        m_messageQueue.pop();

        msg->accept(this);
        count = 0;
      }

      if (m_sample != nullptr && m_wavesample != nullptr && m_playing) {
        if (count == 0) {
          resetDestinations();
          calcDestinations();
        }

        count = (count + 1) % destinationCalcInterval;

        updateVolEg();
        updateFiltEg();
        updateModLfo();
        updateVibLfo();

        float pan = getDestination(Destination::Pan);
        float gain = getDestination(Destination::Gain) + m_wavesample->gain();

        float panParameter = (PI / 2.f) * (pan + 0.5f);
        float leftGain = std::log10(std::cos(panParameter)) + gain;
        float rightGain = std::log10(std::sin(panParameter)) + gain;

        float pitch = getDestination(Destination::Pitch);
        float samplePitch = m_wavesample->unityNote() * 100.f;
        float freqRatio = centsToRatio(pitch - samplePitch);

        float lcoef = belsToGain(leftGain);
        float rcoef = belsToGain(rightGain);
        float lsample =
         interpolateSample(m_samplePos, m_sample->leftData()) * lcoef;
        float rsample =
         interpolateSample(m_samplePos, m_sample->rightData()) * rcoef;

        m_samplePos += freqRatio;
        const WavesampleLoop *loop = m_wavesample->loop();

        if (loop != nullptr) {
          float loopStart = static_cast<float>(loop->start());
          float loopLength = static_cast<float>(loop->length());
          float loopEnd = loopStart + loopLength;

          if (loop->type() == LoopType::Forward) {
            if (m_samplePos >= loopEnd) {
              m_samplePos -= loopLength;
            }
          } else {
            if (m_playing) {
              if (m_samplePos >= loopEnd) {
                m_samplePos -= loopLength;
              }
            } else {
              if (m_samplePos >= m_sample->leftData().size()) {
                m_playing = false;
              }
            }
          }
        } else {
          if (m_samplePos >= m_sample->leftData().size()) {
            m_playing = false;
          }
        }

        if (lp < endLeft) {
          *lp += lsample * outGain;
          lp += bufferSkip;
        }

        if (rp < endRight) {
          *rp += rsample * outGain;
          rp += bufferSkip;
        }
      } else {
        m_playing = false;

        if (lp < endLeft) {
          lp += bufferSkip;
        }

        if (rp < endRight) {
          rp += bufferSkip;
        }
      }
    }
  }
};

Voice::Voice(const Instrument &instrument, const SourceArray &sources,
             std::uint32_t sampleRate)
  : pimpl(new impl(instrument, sources, sampleRate)) {}

Voice::Voice(Voice &&voice) : pimpl(voice.pimpl) { voice.pimpl = nullptr; }

Voice::~Voice() {
  if (pimpl != nullptr) {
    delete pimpl;
  }
}

void Voice::noteOn(std::uint8_t note, std::uint8_t velocity,
                   const Wavesample *wavesample, const Wave &sample,
                   const std::vector<ConnectionBlock> &connectionBlocks) {
  pimpl->m_messageQueue.push(std::make_unique<NoteOnMessage>(
   note, velocity, wavesample, sample, connectionBlocks));
}
void Voice::noteOff() {
  pimpl->m_messageQueue.push(std::make_unique<NoteOffMessage>());
}
void Voice::soundOff() {
  pimpl->m_messageQueue.push(std::make_unique<SoundOffMessage>());
}

void Voice::polyPressure(std::uint8_t value) {
  pimpl->m_messageQueue.push(std::make_unique<PolyPressureMessage>(value));
}

bool Voice::playing() const { return pimpl->m_playing; }
std::uint8_t Voice::note() const { return pimpl->m_note; }
std::chrono::steady_clock::time_point Voice::startTime() const {
  return pimpl->m_startTime;
}

void Voice::render_fill(float *beginLeft, float *endLeft, float *beginRight,
                        float *endRight, std::size_t bufferSkip, float gain) {
  pimpl->render(beginLeft, endLeft, beginRight, endRight, gain, true,
                bufferSkip);
}

void Voice::render_mix(float *beginLeft, float *endLeft, float *beginRight,
                       float *endRight, std::size_t bufferSkip, float gain) {
  pimpl->render(beginLeft, endLeft, beginRight, endRight, gain, false,
                bufferSkip);
}