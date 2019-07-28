#include "Synthesizer.hpp"
#include "../Articulator.hpp"
#include "Voice.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <vector>

using namespace DLSynth;
using namespace DLSynth::Synth;

struct Synthesizer::impl {
  std::array<float, max_source + 1> m_sources{0.f};
  bool m_sustain = false;
  std::vector<Voice> m_voices;
  const Instrument &m_instrument;
  const Sound &m_collection;

  inline float &getSource(Source src) {
    return m_sources[static_cast<std::uint16_t>(src)];
  }

  impl(const Sound &collection, const Instrument &instr)
    : m_instrument(instr), m_collection(collection) {

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
};

Synthesizer::Synthesizer(const Sound &collection, std::size_t instrumentIndex,
                         std::size_t voiceCount, std::uint32_t sampleRate)
  : pimpl(new impl(collection, collection.instruments()[instrumentIndex])) {
  assert(instrumentIndex < collection.instruments().size());

  for (std::size_t i = 0; i < voiceCount; i++) {
    pimpl->m_voices.emplace_back(pimpl->m_instrument, pimpl->m_sources,
                                 sampleRate);
  }
}

Synthesizer::Synthesizer(Synthesizer &&synth) : pimpl(synth.pimpl) {
  synth.pimpl = nullptr;
}

Synthesizer::~Synthesizer() {
  if (pimpl != nullptr) {
    delete pimpl;
  }
}

void Synthesizer::pressure(std::uint8_t value) {
  pimpl->getSource(Source::ChannelPressure) = static_cast<float>(value) / 128.f;
}

void Synthesizer::pressure(std::uint8_t note, std::uint8_t value) {
  // TODO: Not implemented
}

void Synthesizer::pitchBend(std::uint16_t value) {
  pimpl->getSource(Source::PitchWheel) = static_cast<float>(value) / 16384.f;
}

void Synthesizer::volume(std::uint8_t value) {
  pimpl->getSource(Source::CC7) = static_cast<float>(value) / 128.f;
}

void Synthesizer::pan(std::uint8_t value) {
  pimpl->getSource(Source::CC10) = static_cast<float>(value) / 128.f;
}

void Synthesizer::modulation(std::uint8_t value) {
  pimpl->getSource(Source::CC1) = static_cast<float>(value) / 128.f;
}

void Synthesizer::sustain(bool status) { pimpl->m_sustain = status; }

void Synthesizer::reverb(std::uint8_t value) {
  pimpl->getSource(Source::CC91) = static_cast<float>(value) / 128.f;
}

void Synthesizer::chorus(std::uint8_t value) {
  pimpl->getSource(Source::CC93) = static_cast<float>(value) / 128.f;
}

void Synthesizer::pitchBendRange(std::uint16_t value) {
  pimpl->getSource(Source::RPN0) = static_cast<float>(value) / 16384.f;
}

void Synthesizer::fineTuning(std::uint16_t value) {
  pimpl->getSource(Source::RPN1) = static_cast<float>(value) / 16384.f;
}

void Synthesizer::coarseTuning(std::uint16_t value) {
  pimpl->getSource(Source::RPN2) = static_cast<float>(value) / 16384.f;
}

void Synthesizer::resetControllers() {
  pimpl->getSource(Source::CC7) = 100.f / 128.f;
  pimpl->getSource(Source::CC10) = 64.f / 128.f;
  pimpl->getSource(Source::CC91) = 40.f / 128.f;
  pimpl->getSource(Source::ChannelPressure) = 0.f;
  pimpl->getSource(Source::RPN1) = 0.f;
  pimpl->getSource(Source::CC11) = 127.f / 128.f;
  pimpl->getSource(Source::CC1) = 0.f;
  pimpl->getSource(Source::CC93) = 0.f;
  pimpl->getSource(Source::RPN0) = 2.f / 128.f;
  pimpl->getSource(Source::RPN2) = 0.f;
  pimpl->getSource(Source::None) = 1.f;
  pimpl->m_sustain = false;
}

void Synthesizer::noteOn(std::uint8_t note, std::uint8_t velocity) {
  for (const auto &region : pimpl->m_instrument.regions()) {
    if (region.keyRange().inRange(note) &&
        region.velocityRange().inRange(velocity)) {

      const Wave &sample = pimpl->m_collection.wavepool()[region.waveIndex()];
      const Wavesample *wavesample = region.wavesample() != nullptr
                                      ? region.wavesample()
                                      : sample.wavesample();

      // If the region is self exclusive, we need to check whether a voice is
      // already playing the same note, and steal it if necessary.
      if (!region.selfNonExclusive()) {
        for (auto &voice : pimpl->m_voices) {
          if (voice.playing() && voice.note() == note) {
            voice.noteOn(note, velocity, wavesample, sample);
            return;
          }
        }
      }

      // Otherwise, search for a free voice.
      for (auto &voice : pimpl->m_voices) {
        if (!voice.playing()) {
          voice.noteOn(note, velocity, wavesample, sample);
          return;
        }
      }

      // No free voices, steal the one playing the oldest note.
      auto oldestVoice =
       std::min_element(pimpl->m_voices.begin(), pimpl->m_voices.end(),
                        [](const auto &lhs, const auto &rhs) {
                          return lhs.startTime() < rhs.startTime();
                        });
      oldestVoice->noteOn(note, velocity, wavesample, sample);
    }
  }
}

void Synthesizer::noteOff(std::uint8_t note) {
  if (pimpl->m_sustain) {
    return;
  }

  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note) {
      voice.noteOff();
    }
  }
}

void Synthesizer::render_fill(float *beginLeft, float *endLeft,
                              float *beginRight, float *endRight,
                              std::size_t bufferSkip, float gain) {
  bool isFirst = true;
  for (auto &voice : pimpl->m_voices) {
    if (isFirst) {
      voice.render_fill(beginLeft, endLeft, beginRight, endRight, bufferSkip,
                        gain);
      isFirst = false;
    } else {
      voice.render_mix(beginLeft, endLeft, beginRight, endRight, bufferSkip,
                       gain);
    }
  }
}

void Synthesizer::render_mix(float *beginLeft, float *endLeft,
                             float *beginRight, float *endRight,
                             std::size_t bufferSkip, float gain) {
  for (auto &voice : pimpl->m_voices) {
    voice.render_mix(beginLeft, endLeft, beginRight, endRight, bufferSkip,
                     gain);
  }
}