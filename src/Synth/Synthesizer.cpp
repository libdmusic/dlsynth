#include "Synthesizer.hpp"
#include "../Articulator.hpp"
#include "Voice.hpp"
#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

using namespace DLSynth;
using namespace DLSynth::Synth;

struct Synthesizer::impl {
  std::map<Source, float> m_sources{
   {Source::CC7, 100.f / 128.f}, {Source::CC10, 64.f / 128.f},
   {Source::CC91, 40.f / 128.f}, {Source::ChannelPressure, 0.f},
   {Source::RPN1, 0.f},          {Source::CC11, 127.f / 128.f},
   {Source::CC1, 0.f},           {Source::CC93, 0.f},
   {Source::RPN0, 2.f / 128.f},  {Source::RPN2, 0.f},
   {Source::None, 1.f}};
  bool m_sustain = false;
  std::vector<Voice> m_voices;
  const Instrument &m_instrument;
  const Sound &m_collection;

  impl(const Sound &collection, const Instrument &instr)
    : m_instrument(instr), m_collection(collection) {}
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
  pimpl->m_sources[Source::ChannelPressure] = (float)value / 128.f;
}

void Synthesizer::pitchBend(std::uint16_t value) {
  pimpl->m_sources[Source::PitchWheel] = (float)value / 16384.f;
}

void Synthesizer::volume(std::uint8_t value) {
  pimpl->m_sources[Source::CC7] = (float)value / 128.f;
}

void Synthesizer::pan(std::uint8_t value) {
  pimpl->m_sources[Source::CC10] = (float)value / 128.f;
}

void Synthesizer::modulation(std::uint8_t value) {
  pimpl->m_sources[Source::CC1] = (float)value / 128.f;
}

void Synthesizer::sustain(bool status) { pimpl->m_sustain = status; }

void Synthesizer::reverb(std::uint8_t value) {
  pimpl->m_sources[Source::CC91] = (float)value / 128.f;
}

void Synthesizer::chorus(std::uint8_t value) {
  pimpl->m_sources[Source::CC93] = (float)value / 128.f;
}

void Synthesizer::pitchBendRange(std::uint16_t value) {
  pimpl->m_sources[Source::RPN0] = (float)value / 16384.f;
}

void Synthesizer::fineTuning(std::uint16_t value) {
  pimpl->m_sources[Source::RPN1] = (float)value / 16384.f;
}

void Synthesizer::coarseTuning(std::uint16_t value) {
  pimpl->m_sources[Source::RPN2] = (float)value / 16384.f;
}

void Synthesizer::resetControllers() {
  pimpl->m_sources = {
   {Source::CC7, 100.f / 128.f}, {Source::CC10, 64.f / 128.f},
   {Source::CC91, 40.f / 128.f}, {Source::ChannelPressure, 0.f},
   {Source::RPN1, 0.f},          {Source::CC11, 127 / 128.f},
   {Source::CC1, 0.f},           {Source::CC93, 0.f},
   {Source::RPN0, 2.f / 128.f},  {Source::RPN2, 0.f}};
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
                              float *beginRight, float *endRight, float gain) {
  bool isFirst = true;
  for (auto &voice : pimpl->m_voices) {
    if (isFirst) {
      voice.render_fill(beginLeft, endLeft, beginRight, endRight, gain);
      isFirst = false;
    } else {
      voice.render_mix(beginLeft, endLeft, beginRight, endRight, gain);
    }
  }
}

void Synthesizer::render_mix(float *beginLeft, float *endLeft,
                             float *beginRight, float *endRight, float gain) {
  for (auto &voice : pimpl->m_voices) {
    voice.render_mix(beginLeft, endLeft, beginRight, endRight, gain);
  }
}