#include "Voice.hpp"
#include "../NumericUtils.hpp"
#include "EG.hpp"
#include "LFO.hpp"
#include "ModMatrix.hpp"
#include "SPSCQueue.hpp"
#include "VoiceMessage.hpp"
#include <unordered_set>

#if DLSYNTH_USE_SSE >= 3
#include <pmmintrin.h>
using FilterState = __m128;
#else
using FilterState = std::array<float, 4>;
#endif

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

#define DEFAULT_CONN(src, ctrl, dst, scale, bip, inv, type)                    \
  ConnectionBlock(Source::src, Source::ctrl, Destination::dst, scale,          \
                  TransformParams(inv, bip, TransformType::type),              \
                  TransformParams(false, false, TransformType::None))

static float interpolateSample(float pos, const std::vector<float> &samples) {
  std::size_t floor = static_cast<std::size_t>(std::floor(pos));
  std::size_t ceil = static_cast<std::size_t>(std::ceil(pos));

  float sample1 = samples[floor];
  float sample2 = samples[ceil % samples.size()];

  float t = pos - floor;
  return lerp(sample1, sample2, t);
}

using SourceArray = std::array<SignalSource, DLSynth::max_source + 1>;
using DestinationArray =
 std::array<SignalDestination, DLSynth::max_destination + 1>;
constexpr std::size_t messageQueueSize = 128;

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

/// Provides the left and right channel coefficients given global gain and pan
/// values
class StereoGainNode final : public ObservableSignal, public SignalObserver {
  SignalDestination *m_panNode;
  SignalDestination *m_gainNode;

  float m_leftGainAsRatio;
  float m_rightGainAsRatio;

  float m_sampleGain = 0.f;
  bool m_upToDate = false;

  void calc() {
    float pan = m_panNode->value();
    float gain = m_gainNode->value() + m_sampleGain;

    float panParameter = (PI / 2.f) * (pan + .5f);
    float leftGain = std::log10(std::cos(panParameter)) + gain;
    float rightGain = std::log10(std::sin(panParameter)) + gain;

    m_leftGainAsRatio = belsToGain(leftGain);
    m_rightGainAsRatio = belsToGain(rightGain);

    m_upToDate = true;
  }

public:
  StereoGainNode(SignalDestination *pan, SignalDestination *gain)
    : m_panNode(pan), m_gainNode(gain) {
    pan->subscribe(this);
    gain->subscribe(this);
  }

  void sampleGain(float value) {
    m_sampleGain = value;
    m_upToDate = false;
    valueChanged();
  }

  float leftGain() {
    if (!m_upToDate) {
      calc();
    }

    return m_leftGainAsRatio;
  }

  float rightGain() {
    if (!m_upToDate) {
      calc();
    }

    return m_rightGainAsRatio;
  }

protected:
  void sourceChanged() override {
    m_upToDate = false;
    valueChanged();
  }
};

/// Calculates the ratio between the target note frequency and unity sample
/// frequency
class PitchNode final : public ObservableSignal, public SignalObserver {
  SignalDestination *m_pitchNode;
  float m_samplePitch = 0.f;
  float m_sampleFineTune = 0.f;
  bool m_upToDate = false;

  float m_freqRatio;

public:
  PitchNode(SignalDestination *pitch) : m_pitchNode(pitch) {
    pitch->subscribe(this);
  }

  void samplePitch(float value) {
    m_samplePitch = value;
    m_upToDate = false;
    valueChanged();
  }

  void sampleFineTune(float value) {
    m_sampleFineTune = value;
    m_upToDate = false;
    valueChanged();
  }

  float frequencyRatio() {
    if (!m_upToDate) {
      m_freqRatio =
       centsToRatio(m_pitchNode->value() - (m_samplePitch + m_sampleFineTune));
      m_upToDate = true;
    }

    return m_freqRatio;
  }

protected:
  void sourceChanged() override {
    m_upToDate = false;
    valueChanged();
  }
};

/// Biquad filter coefficients
struct FilterCoeffs {
  float a0, a1, a2;
  float b1, b2;

#if DLSYNTH_USE_SSE >= 3
  __m128 coeffs;
#endif
};

/// Calculates the low-pass filter coefficients from the cutoff frequency and Q
/// values
class FilterNode final : public ObservableSignal, public SignalObserver {
  bool m_upToDate = false;
  float m_sampleRate;
  SignalDestination *m_frequency;
  SignalDestination *m_resonance;
  FilterCoeffs m_coeffs;

public:
  FilterNode(float sampleRate, SignalDestination *frequency,
             SignalDestination *resonance)
    : m_sampleRate(sampleRate), m_frequency(frequency), m_resonance(resonance) {
    frequency->subscribe(this);
    resonance->subscribe(this);
  }

  const FilterCoeffs &coeffs() {
    if (!m_upToDate) {
      // https://creatingsound.com/2014/02/dsp-audio-programming-series-part-2/
      float theta = 2.f * PI * (m_frequency->asFreq() / m_sampleRate);
      float d = .5f * (1.f / m_resonance->asGain()) * std::sin(theta);
      float beta = .5f * ((1.f - d) / (1.f + d));
      float gamma = (.5f + beta) * cos(theta);

      m_coeffs.a0 = .5f * (.5f + beta - gamma);
      m_coeffs.a1 = .5f + beta - gamma;
      m_coeffs.a2 = m_coeffs.a0;

      m_coeffs.b1 = -2.f * gamma;
      m_coeffs.b2 = 2.f * beta;

#if DLSYNTH_USE_SSE >= 3
      m_coeffs.coeffs =
       _mm_set_ps(m_coeffs.a1, m_coeffs.a2, m_coeffs.b1, m_coeffs.b2);
#endif
    }

    return m_coeffs;
  }

protected:
  void sourceChanged() override {
    m_upToDate = false;
    valueChanged();
  }
};

struct Voice::impl : public VoiceMessageExecutor {
  impl(std::uint32_t sampleRate)
    : m_sampleRate(sampleRate)
    , m_messageQueue(messageQueueSize)
    , m_gainNode(&getDestination(Destination::Pan),
                 &getDestination(Destination::Gain))
    , m_pitchNode(&getDestination(Destination::Pitch))
    , m_filterNode(static_cast<float>(sampleRate),
                   &getDestination(Destination::FilterCutoff),
                   &getDestination(Destination::FilterQ))
    , m_modLfo(static_cast<float>(sampleRate))
    , m_vibLfo(static_cast<float>(sampleRate))
    , m_volEg(static_cast<float>(sampleRate))
    , m_filtEg(static_cast<float>(sampleRate)) {

    resetControllers();
    getSource(Source::EG1) = 0.f;
    getSource(Source::EG2) = 0.f;
    getSource(Source::LFO) = 0.5f;
    getSource(Source::Vibrato) = 0.5f;
  }

  ~impl() override = default;
  std::uint32_t m_sampleRate;
  rigtorp::SPSCQueue<std::unique_ptr<VoiceMessage>> m_messageQueue;

  std::unordered_set<ConnectionBlock> m_connections;
  SourceArray m_sources;
  DestinationArray m_destinations;
  StereoGainNode m_gainNode;
  PitchNode m_pitchNode;
  FilterNode m_filterNode;
  bool m_playing;
  bool m_sustain;
  bool m_released;
  bool m_isDrum;
  std::uint8_t m_note;
  int m_channel;
  int m_priority;
  std::uint8_t m_velocity;
  std::chrono::steady_clock::time_point m_startTime;

  const Wave *m_sample = nullptr;
  const Wavesample *m_wavesample = nullptr;
  float m_samplePos = 0;

  LFO m_modLfo, m_vibLfo;
  EG m_volEg, m_filtEg;

  inline SignalSource &getSource(Source source) {
    return m_sources[static_cast<std::uint16_t>(source)];
  }

  inline SignalDestination &getDestination(Destination dest) {
    return m_destinations[static_cast<std::uint16_t>(dest)];
  }

  void resetConnections() {
    m_connections = {
     // clang-format off
     //           Source         Control          Destination        Scale       Bip    Inv    TransType
     DEFAULT_CONN(None,          None,            LfoFrequency,      -55791973,  false, false, None),
     DEFAULT_CONN(None,          None,            LfoStartDelay,     -522494112, false, false, None),
     DEFAULT_CONN(None,          None,            VibratoFrequency,  -55791973,  false, false, None),
     DEFAULT_CONN(None,          None,            VibratoStartDelay, -522494112, false, false, None),
     DEFAULT_CONN(None,          None,            EG1DelayTime,      0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG1AttackTime,     0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG1HoldTime,       0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG1DecayTime,      0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG1SustainLevel,   65536000,   false, false, None),
     DEFAULT_CONN(None,          None,            EG1ReleaseTime,    0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG1ShutdownTime,   -476490789, false, false, None),
     DEFAULT_CONN(KeyOnVelocity, None,            EG1AttackTime,     0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            EG1DecayTime,      0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            EG1HoldTime,       0,          false, false, None),
     DEFAULT_CONN(None,          None,            EG2DelayTime,      0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG2AttackTime,     0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG2HoldTime,       0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG2DecayTime,      0x80000000, false, false, None),
     DEFAULT_CONN(None,          None,            EG2SustainLevel,   65536000,   false, false, None),
     DEFAULT_CONN(None,          None,            EG2ReleaseTime,    0x80000000, false, false, None),
     DEFAULT_CONN(KeyOnVelocity, None,            EG2AttackTime,     0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            EG2DecayTime,      0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            EG2HoldTime,       0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            KeyNumber,         838860800,  false, false, None),
     DEFAULT_CONN(RPN2,          None,            KeyNumber,         419430400,  true,  false, None),
     DEFAULT_CONN(None,          None,            FilterCutoff,      0x7FFFFFFF, false, false, None),
     DEFAULT_CONN(None,          None,            FilterQ,           0,          false, false, None),
     DEFAULT_CONN(LFO,           None,            FilterCutoff,      0,          true,  false, None),
     DEFAULT_CONN(LFO,           CC1,             FilterCutoff,      0,          true,  false, None),
     DEFAULT_CONN(LFO,           ChannelPressure, FilterCutoff,      0,          true,  false, None),
     DEFAULT_CONN(EG2,           None,            FilterCutoff,      0,          false, false, None),
     DEFAULT_CONN(KeyOnVelocity, None,            FilterCutoff,      0,          false, false, None),
     DEFAULT_CONN(KeyNumber,     None,            FilterCutoff,      0,          false, false, None),
     DEFAULT_CONN(LFO,           None,            Gain,              0,          true,  false, None),
     DEFAULT_CONN(LFO,           CC1,             Gain,              0,          true,  false, None),
     DEFAULT_CONN(LFO,           ChannelPressure, Gain,              0,          true,  false, None),
     DEFAULT_CONN(KeyOnVelocity, None,            Gain,              -62914560,  false, true,  Concave),
     DEFAULT_CONN(CC7,           None,            Gain,              -62914560,  false, true,  Concave),
     DEFAULT_CONN(CC11,          None,            Gain,              -62914560,  false, true,  Concave),
     DEFAULT_CONN(None,          None,            Pan,               0,          false, false, None),
     DEFAULT_CONN(CC10,          None,            Pan,               33292288,   true,  false, None),
     DEFAULT_CONN(CC91,          None,            Reverb,            65536000,   false, false, None),
     DEFAULT_CONN(None,          None,            Reverb,            0,          false, false, None),
     DEFAULT_CONN(CC93,          None,            Chorus,            65536000,   false, false, None),
     DEFAULT_CONN(None,          None,            Chorus,            0,          false, false, None),
     DEFAULT_CONN(None,          None,            Pitch,             0,          false, false, None),
     DEFAULT_CONN(PitchWheel,    RPN0,            Pitch,             838860800,  true,  false, None),
     DEFAULT_CONN(KeyNumber,     None,            Pitch,             838860800,  false, false, None),
     DEFAULT_CONN(RPN1,          None,            Pitch,             6553600,    false, false, None),
     DEFAULT_CONN(Vibrato,       None,            Pitch,             0,          true,  false, None),
     DEFAULT_CONN(Vibrato,       CC1,             Pitch,             0,          true,  false, None),
     DEFAULT_CONN(Vibrato,       ChannelPressure, Pitch,             0,          true,  false, None),
     DEFAULT_CONN(LFO,           None,            Pitch,             0,          true,  false, None),
     DEFAULT_CONN(LFO,           CC1,             Pitch,             0,          true,  false, None),
     DEFAULT_CONN(LFO,           ChannelPressure, Pitch,             0,          true,  false, None),
     DEFAULT_CONN(EG2,           None,            Pitch,             0,          false, false, None),
     DEFAULT_CONN(EG1,           None,            Gain,              -62914560,  false, true,  None),
     // clang-format on
    };
  }

  void resetControllers() {
    getSource(Source::CC7) = 100.f / 128.f;
    getSource(Source::CC10) = 64.f / 128.f;
    getSource(Source::CC91) = 40.f / 128.f;
    getSource(Source::ChannelPressure) = 0.f;
    getSource(Source::RPN1) = 0.f;
    getSource(Source::CC11) = 127.f / 128.f;
    getSource(Source::CC1) = 0.f;
    getSource(Source::CC93) = 0.f;
    getSource(Source::RPN0) = 2.f / 128.f;
    getSource(Source::RPN2) = 0.f;
    getSource(Source::None) = 1.f;
  }

  void loadConnections(const std::vector<ConnectionBlock> &cblocks) {
    for (auto &source : m_sources) {
      source.resetSubscribers();
    }

    for (auto &destination : m_destinations) {
      destination.resetConnections();
    }

    for (const auto &block : cblocks) {
      auto old_elem = m_connections.find(block);
      if (old_elem == m_connections.end()) {
        m_connections.insert(block);
      } else {
        m_connections.erase(old_elem);
        m_connections.insert(block);
      }
    }

    for (const auto &conn : m_connections) {
      getDestination(conn.destination())
       .addConnection(getSource(conn.source()), conn.sourceTransform(),
                      getSource(conn.control()), conn.controlTransform(),
                      getScaleValue(conn.destination(), conn.scale()));
    }
  }

  void updateModLfo() {
    float freq = getDestination(Destination::LfoFrequency).asFreq();
    float startDelay = getDestination(Destination::LfoStartDelay).asSecs();

    float output = m_modLfo.nextSample(freq, startDelay);

    getSource(Source::LFO) = output;
  }

  void updateVibLfo() {
    float freq = getDestination(Destination::VibratoFrequency).asFreq();
    float startDelay = getDestination(Destination::VibratoStartDelay).asSecs();

    float output = m_vibLfo.nextSample(freq, startDelay);

    getSource(Source::Vibrato) = output;
  }

  void updateVolEg() {
    float delay = getDestination(Destination::EG1DelayTime).asSecs();
    float attack = getDestination(Destination::EG1AttackTime).asSecs();
    float hold = getDestination(Destination::EG1HoldTime).asSecs();
    float decay = getDestination(Destination::EG1DecayTime).asSecs();
    float sustain = getDestination(Destination::EG1SustainLevel);
    float release = getDestination(Destination::EG1ReleaseTime).asSecs();

    float output =
     m_volEg.nextSample(delay, attack, hold, decay, sustain, release);

    bool gate = m_volEg.gate();
    if (!gate && output == 0.f) {
      m_playing = false;
    }

    getSource(Source::EG1) = output;
  }

  void updateFiltEg() {
    float delay = getDestination(Destination::EG2DelayTime).asSecs();
    float attack = getDestination(Destination::EG2AttackTime).asSecs();
    float hold = getDestination(Destination::EG2HoldTime).asSecs();
    float decay = getDestination(Destination::EG2DecayTime).asSecs();
    float sustain = getDestination(Destination::EG2SustainLevel);
    float release = getDestination(Destination::EG2ReleaseTime).asSecs();

    float output =
     m_filtEg.nextSample(delay, attack, hold, decay, sustain, release);

    getSource(Source::EG2) = output;
  }

  void execute(const SustainChangeMessage &message) override {
    m_sustain = message.value();
    if (!m_sustain && m_released) {
      m_volEg.noteOff();
      m_filtEg.noteOff();
    }
  }

  void execute(const NoteOnMessage &message) override {
    m_playing = true;
    m_released = false;
    m_priority = message.priority();
    m_isDrum = message.isDrum();
    resetConnections();
    loadConnections(message.connectionBlocks());
    m_wavesample = message.wavesample();
    m_sample = &message.sample();
    m_pitchNode.samplePitch(m_wavesample->unityNote() * 100.f);
    m_pitchNode.sampleFineTune(m_wavesample->fineTune());
    m_samplePos = 0;
    m_gainNode.sampleGain(m_wavesample->gain());
    m_note = message.note();
    m_velocity = message.velocity();
    m_channel = message.channel();
    m_startTime = std::chrono::steady_clock::now();
    getSource(Source::KeyNumber) = static_cast<float>(m_note) / 128.f;
    getSource(Source::KeyOnVelocity) = static_cast<float>(m_velocity) / 128.f;
    m_filtEg.noteOn();
    m_modLfo.reset();
    m_vibLfo.reset();
    m_volEg.noteOn();
  }

  void execute(const NoteOffMessage &message) override {
    if (m_sustain) {
      m_released = true;
    } else {
      if (!m_isDrum) {
        m_volEg.noteOff();
        m_filtEg.noteOff();
      }
    }
  }

  void execute(const SoundOffMessage &message) override {
    m_wavesample = nullptr;
    m_sample = nullptr;
    m_playing = false;
  }

  void execute(const ControlChangeMessage &message) override {
    getSource(message.source()) = message.value();
  }

  void execute(const ResetControllersMessage &message) override {
    resetControllers();
  }

  float lcoef = 0.f;
  float rcoef = 0.f;
  static constexpr float lowpass_b = 0.01f;

  std::array<FilterState, 2> m_filterState{};

  float computeFilter(int channel, float input) {

    const auto &mCoeffs = m_filterNode.coeffs();

#if DLSYNTH_USE_SSE >= 3
    __m128 zero = _mm_setzero_ps();

    // a = (i0, i1, o0, o1)
    __m128 a = m_filterState[0];

    // b = (a1, a2, b1, b2)
    __m128 b = mCoeffs.coeffs;

    // c = (a1 * i0, a2 * i1, b1 * o0, b2 * o1)
    __m128 c = _mm_mul_ps(a, b);

    // d = (a1 * i0 + a2 * i1, b1 * o0 + b2 * o1, 0, 0)
    __m128 d = _mm_hadd_ps(c, zero);

    // e = (a1 * i0 + a2 * i1 - b1 * o0 - b2 * o1, 0, 0, 0)
    __m128 e = _mm_hsub_ps(d, zero);

    alignas(16) std::array<float, 4> values;
    _mm_store_ps(values.data(), e);

    float output = (mCoeffs.a0 * input) + values[0];
    _mm_store_ps(values.data(), a);

    m_filterState[0] = _mm_set_ps(input, values[0], output, values[2]);
#else
    float output = (mCoeffs.a0 * input) +
                   (mCoeffs.a1 * m_filterState[channel][0]) +
                   (mCoeffs.a2 * m_filterState[channel][1]) -
                   (mCoeffs.b1 * m_filterState[channel][2]) -
                   (mCoeffs.b2 * m_filterState[channel][3]);

    m_filterState[channel][1] = m_filterState[channel][0];
    m_filterState[channel][0] = input;
    m_filterState[channel][3] = m_filterState[channel][2];
    m_filterState[channel][2] = output;
#endif

    return output;
  }

  void render(float *beginLeft, float *endLeft, float *beginRight,
              float *endRight, float outGain, bool fill,
              std::size_t bufferSkip) {
    float *lp = beginLeft;
    float *rp = beginRight;

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
      }

      if (m_sample != nullptr && m_wavesample != nullptr && m_playing) {
        updateVolEg();
        updateFiltEg();
        updateModLfo();
        updateVibLfo();

        float freqRatio =
         m_pitchNode.frequencyRatio() * m_sample->sampleRate() / m_sampleRate;

        lcoef += lowpass_b * (m_gainNode.leftGain() - lcoef);
        rcoef += lowpass_b * (m_gainNode.rightGain() - rcoef);

        float lsample =
         interpolateSample(m_samplePos, m_sample->leftData()) * lcoef;
        float rsample =
         interpolateSample(m_samplePos, m_sample->rightData()) * rcoef;

        const auto &mCoeffs = m_filterNode.coeffs();

        float lout = computeFilter(0, lsample);
        float rout = computeFilter(1, rsample);

        m_samplePos += freqRatio;
        const WavesampleLoop *loop = m_wavesample->loop();

        if (loop != nullptr) {
          float loopStart = static_cast<float>(loop->start());
          float loopLength = static_cast<float>(loop->length());
          float loopEnd = loopStart + loopLength;

          if (loop->type() == LoopType::Forward) {
            if (m_samplePos >= loopEnd ||
                m_samplePos >= m_sample->leftData().size()) {
              m_samplePos -= loopLength;
            }
          } else {
            if (m_playing) {
              if (m_samplePos >= loopEnd ||
                  m_samplePos >= m_sample->leftData().size()) {
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
          *lp += lout * outGain;
          lp += bufferSkip;
        }

        if (rp < endRight) {
          *rp += rout * outGain;
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

Voice::Voice(std::uint32_t sampleRate) : pimpl(new impl(sampleRate)) {}

Voice::Voice(Voice &&voice) : pimpl(voice.pimpl) { voice.pimpl = nullptr; }

Voice::~Voice() {
  if (pimpl != nullptr) {
    delete pimpl;
  }
}

void Voice::noteOn(int channel, int priority, std::uint8_t note,
                   std::uint8_t velocity, bool isDrum,
                   const Wavesample *wavesample, const Wave &sample,
                   const std::vector<ConnectionBlock> &connectionBlocks) {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(
   std::make_unique<NoteOnMessage>(channel, priority, note, velocity, isDrum,
                                   wavesample, sample, connectionBlocks));
}
void Voice::noteOff() {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(std::make_unique<NoteOffMessage>());
}
void Voice::soundOff() {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(std::make_unique<SoundOffMessage>());
}

void Voice::controlChange(Source source, float value) {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(
   std::make_unique<ControlChangeMessage>(source, value));
}

void Voice::resetControllers() {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(std::make_unique<ResetControllersMessage>());
}

void Voice::sustain(bool value) {
  assert(pimpl != nullptr);
  pimpl->m_messageQueue.push(std::make_unique<SustainChangeMessage>(value));
}

bool Voice::playing() const {
  assert(pimpl != nullptr);
  return pimpl->m_playing;
}

int Voice::channel() const {
  assert(pimpl != nullptr);
  return pimpl->m_channel;
}

int Voice::priority() const {
  assert(pimpl != nullptr);
  return pimpl->m_priority;
}

std::uint8_t Voice::note() const {
  assert(pimpl != nullptr);
  return pimpl->m_note;
}
std::chrono::steady_clock::time_point Voice::startTime() const {
  assert(pimpl != nullptr);
  return pimpl->m_startTime;
}

void Voice::render_fill(float *beginLeft, float *endLeft, float *beginRight,
                        float *endRight, std::size_t bufferSkip, float gain) {
  assert(pimpl != nullptr);
  pimpl->render(beginLeft, endLeft, beginRight, endRight, gain, true,
                bufferSkip);
}

void Voice::render_mix(float *beginLeft, float *endLeft, float *beginRight,
                       float *endRight, std::size_t bufferSkip, float gain) {
  assert(pimpl != nullptr);
  pimpl->render(beginLeft, endLeft, beginRight, endRight, gain, false,
                bufferSkip);
}