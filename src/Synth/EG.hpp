#ifndef EG_HPP
#define EG_HPP

namespace DLSynth {
namespace Synth {
  /// A Delay-Attack-Hold-Decay-Sustain-Release Envelope generator
  class EG final {
    float m_currentTime = 0;
    float m_lastValue = 0;
    float m_sampleRate;
    float m_sampleInterval;
    bool m_gate = false;

  public:
    EG(float sampleRate);

    void noteOn();
    void noteOff();

    bool gate() const;

    float nextSample(float delayTime, float attackTime, float holdTime,
                     float decayTime, float sustainLevel, float releaseTime);
  };
} // namespace Synth
} // namespace DLSynth

#endif