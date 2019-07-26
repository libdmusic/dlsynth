#ifndef EG_HPP
#define EG_HPP

namespace DLSynth {
namespace Synth {
  class EG {
    float m_currentTime = 0;
    float m_lastValue = 0;
    float m_sampleRate;
    bool m_gate = false;

  public:
    EG(float sampleRate);

    void noteOn();
    void noteOff();

    bool isActive() const;

    float nextSample(float delayTime, float attackTime, float holdTime,
                     float decayTime, float sustainLevel, float releaseTime);
  };
} // namespace Synth
} // namespace DLSynth

#endif