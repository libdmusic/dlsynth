#include "Synthesizer.hpp"
#include "../Articulator.hpp"
#include "DisableDenormals.hpp"
#include "Voice.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <mutex>
#include <vector>

using namespace DLSynth;
using namespace DLSynth::Synth;

struct Synthesizer::impl {
  std::mutex mutex;
  std::vector<Voice> m_voices;

  void controlChange(int channel, Source source, float value) {
    for (auto &voice : m_voices) {
      if (voice.channel() != channel) {
        continue;
      }
      voice.controlChange(source, value);
    }
  }

  impl() {}
};

Synthesizer::Synthesizer(std::size_t voiceCount, std::uint32_t sampleRate)
  : pimpl(new impl()) {
  for (std::size_t i = 0; i < voiceCount; i++) {
    pimpl->m_voices.emplace_back(sampleRate);
  }
}

Synthesizer::Synthesizer(Synthesizer &&synth) : pimpl(std::move(synth.pimpl)) {}

Synthesizer::~Synthesizer() = default;

void Synthesizer::pressure(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::ChannelPressure,
                       static_cast<float>(value) / 128.f);
}

void Synthesizer::pressure(int channel, std::uint8_t note, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note && voice.channel() == channel) {
      voice.controlChange(Source::PolyPressure,
                          static_cast<float>(value) / 128.f);
    }
  }
}

void Synthesizer::pitchBend(int channel, std::uint16_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::PitchWheel,
                       static_cast<float>(value) / 16384.f);
}

void Synthesizer::volume(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::CC7, static_cast<float>(value) / 128.f);
}

void Synthesizer::pan(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::CC10,
                       static_cast<float>(value) / 128.f);
}

void Synthesizer::modulation(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::CC1, static_cast<float>(value) / 128.f);
}

void Synthesizer::sustain(int channel, bool status) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    if (voice.channel() != channel) {
      continue;
    }
    voice.sustain(status);
  }
}

void Synthesizer::reverb(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::CC91,
                       static_cast<float>(value) / 128.f);
}

void Synthesizer::chorus(int channel, std::uint8_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::CC93,
                       static_cast<float>(value) / 128.f);
}

void Synthesizer::pitchBendRange(int channel, std::uint16_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::RPN0,
                       static_cast<float>(value) / 16384.f);
}

void Synthesizer::fineTuning(int channel, std::uint16_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::RPN1,
                       static_cast<float>(value) / 16384.f);
}

void Synthesizer::coarseTuning(int channel, std::uint16_t value) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  pimpl->controlChange(channel, Source::RPN2,
                       static_cast<float>(value) / 16384.f);
}

void Synthesizer::resetControllers(int channel) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    if (voice.channel() != channel) {
      continue;
    }
    voice.resetControllers();
    voice.sustain(false);
  }
}

void Synthesizer::noteOn(const Sound &collection, std::size_t index,
                         int channel, int priority, std::uint8_t note,
                         std::uint8_t velocity) {
  assert(index < collection.instruments().size());

  const auto &instrument = collection.instruments()[index];
  bool isDrum = instrument.isDrumInstrument();

  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (const auto &region : instrument.regions()) {
    if (region.keyRange().inRange(note) &&
        region.velocityRange().inRange(velocity)) {
      std::vector<ConnectionBlock> connectionBlocks =
       instrument.connectionBlocks();

      connectionBlocks.insert(std::end(connectionBlocks),
                              std::begin(region.connectionBlocks()),
                              std::end(region.connectionBlocks()));

      const Wave &sample = collection.wavepool()[region.waveIndex()];
      const Wavesample *wavesample = region.wavesample() != nullptr
                                      ? region.wavesample()
                                      : sample.wavesample();

      // If the region is self exclusive, we need to check whether a voice is
      // already playing the same note, and steal it if necessary.
      if (!region.selfNonExclusive()) {
        for (auto &voice : pimpl->m_voices) {
          if (voice.playing() && voice.note() == note) {
            voice.noteOn(channel, priority, note, velocity, isDrum, wavesample,
                         sample, connectionBlocks);
            return;
          }
        }
      }

      // Otherwise, search for a free voice.
      for (auto &voice : pimpl->m_voices) {
        if (!voice.playing()) {
          voice.noteOn(channel, priority, note, velocity, isDrum, wavesample,
                       sample, connectionBlocks);
          return;
        }
      }

      Voice *suitable_voice = nullptr;
      for (auto &voice : pimpl->m_voices) {
        if (voice.priority() <= priority &&
            (suitable_voice == nullptr ||
             suitable_voice->startTime() < voice.startTime())) {
          suitable_voice = &voice;
        }
      }

      if (suitable_voice == nullptr) {
        return;
      }

      suitable_voice->noteOn(channel, priority, note, velocity, isDrum,
                             wavesample, sample, connectionBlocks);
    }
  }
}

void Synthesizer::noteOff(int channel, std::uint8_t note) {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note && voice.channel() == channel) {
      voice.noteOff();
    }
  }
}

void Synthesizer::allNotesOff() {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    voice.noteOff();
  }
}

void Synthesizer::allSoundOff() {
  std::lock_guard<std::mutex> lock(pimpl->mutex);
  for (auto &voice : pimpl->m_voices) {
    voice.soundOff();
  }
}

void Synthesizer::render_fill(float *beginLeft, float *endLeft,
                              float *beginRight, float *endRight,
                              std::size_t bufferSkip, float gain) {
  DisableDenormals no_denormals;
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
  DisableDenormals no_denormals;
  for (auto &voice : pimpl->m_voices) {
    voice.render_mix(beginLeft, endLeft, beginRight, endRight, bufferSkip,
                     gain);
  }
}