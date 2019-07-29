#ifndef LFO_HPP
#define LFO_HPP

namespace DLSynth {
namespace Synth {
  /// A low-frequency oscillator with variable start delay
  class LFO {
    float m_phase = 0;
    float m_currentTime = 0;
    float m_sampleRate;

  public:
    LFO(float sampleRate);

    /// Computes the next sample
    float nextSample(float freq, ///< [in] Frequency of the oscillator in Hz
                     float startDelay ///< [in] Start delay in seconds
    );

    /// Resets the oscillator to its initial state
    void reset();
  };
} // namespace Synth
} // namespace DLSynth

#endif