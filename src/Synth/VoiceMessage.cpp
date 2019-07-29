#include "VoiceMessage.hpp"

using namespace DLSynth;
using namespace DLSynth::Synth;

NoteOnMessage::NoteOnMessage(
 std::uint8_t note, std::uint8_t velocity, const Wavesample *wavesample,
 const Wave &sample, const std::vector<ConnectionBlock> &connectionBlocks)
  : m_note(note)
  , m_velocity(velocity)
  , m_wavesample(wavesample)
  , m_sample(sample)
  , m_connectionBlocks(connectionBlocks) {}

NoteOnMessage::~NoteOnMessage() = default;

void NoteOnMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

std::uint8_t NoteOnMessage::note() const { return m_note; }

std::uint8_t NoteOnMessage::velocity() const { return m_velocity; }

const Wavesample *NoteOnMessage::wavesample() const { return m_wavesample; }

const Wave &NoteOnMessage::sample() const { return m_sample; }

const std::vector<ConnectionBlock> &NoteOnMessage::connectionBlocks() const {
  return m_connectionBlocks;
}

NoteOffMessage::NoteOffMessage() {}

NoteOffMessage::~NoteOffMessage() = default;

void NoteOffMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

SoundOffMessage::SoundOffMessage() {}

SoundOffMessage::~SoundOffMessage() = default;

void SoundOffMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

PolyPressureMessage::PolyPressureMessage(std::uint8_t value) : m_value(value) {}

PolyPressureMessage::~PolyPressureMessage() = default;

void PolyPressureMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

std::uint8_t PolyPressureMessage::value() const { return m_value; }

VoiceMessage::~VoiceMessage() = default;
VoiceMessageExecutor::~VoiceMessageExecutor() = default;