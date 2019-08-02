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
  bool m_sustain = false;
  std::vector<Voice> m_voices;
  const Instrument &m_instrument;
  const Sound &m_collection;

  void controlChange(Source source, float value) {
    for (auto &voice : m_voices) {
      voice.controlChange(source, value);
    }
  }

  impl(const Sound &collection, const Instrument &instr)
    : m_instrument(instr), m_collection(collection) {}
};

Synthesizer::Synthesizer(const Sound &collection, std::size_t instrumentIndex,
                         std::size_t voiceCount, std::uint32_t sampleRate)
  : pimpl(new impl(collection, collection.instruments()[instrumentIndex])) {
  assert(instrumentIndex < collection.instruments().size());

  for (std::size_t i = 0; i < voiceCount; i++) {
    pimpl->m_voices.emplace_back(sampleRate);
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
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::ChannelPressure,
                       static_cast<float>(value) / 128.f);
}

void Synthesizer::pressure(std::uint8_t note, std::uint8_t value) {
  assert(pimpl != nullptr);
  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note) {
      voice.controlChange(Source::PolyPressure,
                          static_cast<float>(value) / 128.f);
    }
  }
}

void Synthesizer::pitchBend(std::uint16_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::PitchWheel, static_cast<float>(value) / 16384.f);
}

void Synthesizer::volume(std::uint8_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::CC7, static_cast<float>(value) / 128.f);
}

void Synthesizer::pan(std::uint8_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::CC10, static_cast<float>(value) / 128.f);
}

void Synthesizer::modulation(std::uint8_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::CC1, static_cast<float>(value) / 128.f);
}

void Synthesizer::sustain(bool status) {
  assert(pimpl != nullptr);
  pimpl->m_sustain = status;
}

void Synthesizer::reverb(std::uint8_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::CC91, static_cast<float>(value) / 128.f);
}

void Synthesizer::chorus(std::uint8_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::CC93, static_cast<float>(value) / 128.f);
}

void Synthesizer::pitchBendRange(std::uint16_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::RPN0, static_cast<float>(value) / 16384.f);
}

void Synthesizer::fineTuning(std::uint16_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::RPN1, static_cast<float>(value) / 16384.f);
}

void Synthesizer::coarseTuning(std::uint16_t value) {
  assert(pimpl != nullptr);
  pimpl->controlChange(Source::RPN2, static_cast<float>(value) / 16384.f);
}

void Synthesizer::resetControllers() {
  assert(pimpl != nullptr);
  for (auto &voice : pimpl->m_voices) {
    voice.resetControllers();
  }
  pimpl->m_sustain = false;
}

void Synthesizer::noteOn(std::uint8_t note, std::uint8_t velocity) {
  assert(pimpl != nullptr);

  for (const auto &region : pimpl->m_instrument.regions()) {
    if (region.keyRange().inRange(note) &&
        region.velocityRange().inRange(velocity)) {
      std::vector<ConnectionBlock> connectionBlocks =
       pimpl->m_instrument.connectionBlocks();

      connectionBlocks.insert(std::end(connectionBlocks),
                              std::begin(region.connectionBlocks()),
                              std::end(region.connectionBlocks()));

      const Wave &sample = pimpl->m_collection.wavepool()[region.waveIndex()];
      const Wavesample *wavesample = region.wavesample() != nullptr
                                      ? region.wavesample()
                                      : sample.wavesample();

      // If the region is self exclusive, we need to check whether a voice is
      // already playing the same note, and steal it if necessary.
      if (!region.selfNonExclusive()) {
        for (auto &voice : pimpl->m_voices) {
          if (voice.playing() && voice.note() == note) {
            voice.noteOn(note, velocity, wavesample, sample, connectionBlocks);
            return;
          }
        }
      }

      // Otherwise, search for a free voice.
      for (auto &voice : pimpl->m_voices) {
        if (!voice.playing()) {
          voice.noteOn(note, velocity, wavesample, sample, connectionBlocks);
          return;
        }
      }

      // No free voices, steal the one playing the oldest note.
      auto oldestVoice =
       std::min_element(pimpl->m_voices.begin(), pimpl->m_voices.end(),
                        [](const auto &lhs, const auto &rhs) {
                          return lhs.startTime() < rhs.startTime();
                        });
      oldestVoice->noteOn(note, velocity, wavesample, sample, connectionBlocks);
    }
  }
}

void Synthesizer::noteOff(std::uint8_t note) {
  assert(pimpl != nullptr);
  if (pimpl->m_sustain) {
    return;
  }

  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note) {
      voice.noteOff();
    }
  }
}

void Synthesizer::allNotesOff() {
  assert(pimpl != nullptr);
  if (pimpl->m_sustain) {
    return;
  }

  for (auto &voice : pimpl->m_voices) {
    voice.noteOff();
  }
}

void Synthesizer::allSoundOff() {
  assert(pimpl != nullptr);
  for (auto &voice : pimpl->m_voices) {
    voice.soundOff();
  }
}

void Synthesizer::render_fill(float *beginLeft, float *endLeft,
                              float *beginRight, float *endRight,
                              std::size_t bufferSkip, float gain) {
  assert(pimpl != nullptr);
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
  assert(pimpl != nullptr);
  for (auto &voice : pimpl->m_voices) {
    voice.render_mix(beginLeft, endLeft, beginRight, endRight, bufferSkip,
                     gain);
  }
}