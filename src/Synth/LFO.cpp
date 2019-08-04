#include "LFO.hpp"
#include "../NumericUtils.hpp"
#include <cmath>

using namespace DLSynth::Synth;

LFO::LFO(float sampleRate)
  : m_sampleRate(sampleRate), m_sampleInterval(1.f / sampleRate) {}

void LFO::reset() {
  m_phase = 0;
  m_currentTime = 0;
}

float LFO::nextSample(float freq, float startDelay) {
  float out = 0.5f;
  if (m_currentTime > startDelay) {
    float sample = std::sin(m_phase * 2.f * PI);
    m_phase += freq / m_sampleRate;
    if (m_phase > 1.f) {
      m_phase -= 1.f;
    }

    out = (sample + 1.f) / 2.f;
  }

  m_currentTime += m_sampleInterval;

  return out;
}