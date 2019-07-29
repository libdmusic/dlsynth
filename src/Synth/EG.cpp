#include "EG.hpp"

using namespace DLSynth::Synth;

EG::EG(float sampleRate) : m_sampleRate(sampleRate) {}

void EG::noteOn() {
  m_gate = true;
  m_currentTime = 0;
  m_lastValue = 0;
}

void EG::noteOff() {
  m_gate = false;
  m_currentTime = 0;
}

float EG::nextSample(float delayTime, float attackTime, float holdTime,
                     float decayTime, float sustainLevel, float releaseTime) {
  float time = m_currentTime;
  m_currentTime += 1 / m_sampleRate;
  if (m_gate) {
    if (time < delayTime) {
      m_lastValue = 0;
      return m_lastValue;
    }

    time -= delayTime;
    if (time < attackTime) {
      float diff = 1.f - m_lastValue;
      m_lastValue += diff / (attackTime - time);
      return m_lastValue;
    }

    time -= attackTime;
    if (time < holdTime) {
      m_lastValue = 1;
      return m_lastValue;
    }

    time -= holdTime;
    if (time < decayTime) {
      float diff = m_lastValue - sustainLevel;
      m_lastValue -= diff / (decayTime - time);
      return m_lastValue;
    }

    m_lastValue = 1;
    return m_lastValue;
  } else {
    if (time < releaseTime) {
      float diff = m_lastValue;
      m_lastValue -= diff / (releaseTime - time);
      return m_lastValue;
    }

    m_lastValue = 0;
    return m_lastValue;
  }
}

bool EG::gate() const { return m_gate; }