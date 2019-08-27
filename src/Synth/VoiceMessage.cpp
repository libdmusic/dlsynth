#include "VoiceMessage.hpp"

using namespace DLSynth;
using namespace DLSynth::Synth;

NoteOnMessage::NoteOnMessage(
 int channel, int priority, std::uint8_t note, std::uint8_t velocity,
 bool isDrum, const Wavesample *wavesample, const Wave &sample,
 const std::vector<ConnectionBlock> &connectionBlocks)
  : m_channel(channel)
  , m_note(note)
  , m_velocity(velocity)
  , m_wavesample(wavesample)
  , m_sample(sample)
  , m_connectionBlocks(connectionBlocks)
  , m_isDrum(isDrum)
  , m_priority(priority) {}

NoteOnMessage::~NoteOnMessage() = default;

void NoteOnMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

std::uint8_t NoteOnMessage::note() const { return m_note; }

std::uint8_t NoteOnMessage::velocity() const { return m_velocity; }

int NoteOnMessage::channel() const { return m_channel; }

const Wavesample *NoteOnMessage::wavesample() const { return m_wavesample; }

const Wave &NoteOnMessage::sample() const { return m_sample; }

const std::vector<ConnectionBlock> &NoteOnMessage::connectionBlocks() const {
  return m_connectionBlocks;
}

bool NoteOnMessage::isDrum() const { return m_isDrum; }

int NoteOnMessage::priority() const { return m_priority; }

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

ControlChangeMessage::ControlChangeMessage(Source source, float value)
  : m_source(source), m_value(value) {}
ControlChangeMessage::~ControlChangeMessage() = default;

void ControlChangeMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

Source ControlChangeMessage::source() const { return m_source; }
float ControlChangeMessage::value() const { return m_value; }

ResetControllersMessage::ResetControllersMessage() = default;
ResetControllersMessage::~ResetControllersMessage() = default;

void ResetControllersMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

SustainChangeMessage::SustainChangeMessage(bool value) : m_value(value) {}
SustainChangeMessage::~SustainChangeMessage() = default;

bool SustainChangeMessage::value() const { return m_value; }

void SustainChangeMessage::accept(VoiceMessageExecutor *executor) {
  executor->execute(*this);
}

VoiceMessage::~VoiceMessage() = default;
VoiceMessageExecutor::~VoiceMessageExecutor() = default;