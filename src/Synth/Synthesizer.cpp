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
  std::map<Source, float> m_sources = {
   {Source::CC7, 100 / 128.f}, {Source::CC10, 64 / 128.f},
   {Source::CC91, 40 / 128.f}, {Source::ChannelPressure, 0},
   {Source::RPN1, 0},          {Source::CC11, 127 / 128.f},
   {Source::CC1, 0},           {Source::CC93, 0},
   {Source::RPN0, 2 / 128.f},  {Source::RPN2, 0},
   {Source::None, 1}};
  bool m_sustain = false;
  std::vector<Voice> m_voices;
  const Instrument &m_instrument;

  impl(const Instrument &instr) : m_instrument(instr) {}
};

Synthesizer::Synthesizer(const Sound &collection, std::size_t instrumentIndex,
                         std::size_t voiceCount, std::uint32_t sampleRate)
  : pimpl(new impl(collection.instruments()[instrumentIndex])) {
  assert(instrumentIndex < collection.instruments().size());

  for (std::size_t i = 0; i < voiceCount; i++) {
    pimpl->m_voices.emplace_back(pimpl->m_instrument, pimpl->m_voices,
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
  pimpl->m_sources[Source::ChannelPressure] = value;
}

void Synthesizer::pitchBend(std::uint16_t value) {
  pimpl->m_sources[Source::PitchWheel] = value;
}

void Synthesizer::volume(std::uint8_t value) {
  pimpl->m_sources[Source::CC7] = value;
}

void Synthesizer::pan(std::uint8_t value) {
  pimpl->m_sources[Source::CC10] = value;
}

void Synthesizer::modulation(std::uint8_t value) {
  pimpl->m_sources[Source::CC1] = value;
}

void Synthesizer::sustain(bool status) { pimpl->m_sustain = status; }

void Synthesizer::reverb(std::uint8_t value) {
  pimpl->m_sources[Source::CC91] = value;
}

void Synthesizer::chorus(std::uint8_t value) {
  pimpl->m_sources[Source::CC93] = value;
}

void Synthesizer::pitchBendRange(std::uint16_t value) {
  pimpl->m_sources[Source::RPN0] = value;
}

void Synthesizer::fineTuning(std::uint16_t value) {
  pimpl->m_sources[Source::RPN1] = value;
}

void Synthesizer::coarseTuning(std::uint16_t value) {
  pimpl->m_sources[Source::RPN2] = value;
}

void Synthesizer::resetControllers() {
  pimpl->m_sources = {{Source::CC7, 100}, {Source::CC10, 64},
                      {Source::CC91, 40}, {Source::ChannelPressure, 0},
                      {Source::RPN1, 0},  {Source::CC11, 127},
                      {Source::CC1, 0},   {Source::CC93, 0},
                      {Source::RPN0, 2},  {Source::RPN2, 0}};
  pimpl->m_sustain = false;
}

void Synthesizer::noteOn(std::uint8_t note, std::uint8_t velocity) {
  for (const auto &region : pimpl->m_instrument.regions()) {
    if (region.keyRange().inRange(note) &&
        region.velocityRange().inRange(velocity)) {

      // If the region is self exclusive, we need to check whether a voice is
      // already playing the same note, and steal it if necessary.
      if (!region.selfNonExclusive()) {
        for (auto &voice : pimpl->m_voices) {
          if (voice.playing() && voice.note() == note) {
            voice.noteOn(note, velocity);
            return;
          }
        }
      }

      // Otherwise, search for a free voice.
      for (auto &voice : pimpl->m_voices) {
        if (!voice.playing()) {
          voice.noteOn(note, velocity);
          return;
        }
      }

      // No free voices, steal the one playing the oldest note.
      auto oldestVoice = std::min_element(
       pimpl->m_voices.begin(), pimpl->m_voices.end(),
       [](auto lhs, auto rhs) { return lhs.startTime() < rhs.startTime(); });
      oldestVoice->noteOn(note, velocity);
    }
  }
}

void Synthesizer::noteOff(std::uint8_t note, std::uint8_t velocity) {
  if (pimpl->m_sustain) {
    return;
  }

  for (auto &voice : pimpl->m_voices) {
    if (voice.playing() && voice.note() == note) {
      voice.noteOff(velocity);
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