#ifndef VOICEMESSAGE_HPP
#define VOICEMESSAGE_HPP

#include "../Articulator.hpp"
#include "../Wave.hpp"
#include "../Wavesample.hpp"
#include <cstdint>

namespace DLSynth {
namespace Synth {
  class VoiceMessageExecutor;

  class VoiceMessage {
  public:
    virtual ~VoiceMessage();
    virtual void accept(VoiceMessageExecutor *executor) = 0;
  };

  class NoteOnMessage final : public VoiceMessage {
    std::uint8_t m_note, m_velocity;
    const Wavesample *m_wavesample;
    const Wave &m_sample;
    std::vector<ConnectionBlock> m_connectionBlocks;

  public:
    NoteOnMessage(std::uint8_t note, std::uint8_t velocity,
                  const Wavesample *wavesample, const Wave &sample,
                  const std::vector<ConnectionBlock> &m_connectionBlocks);
    ~NoteOnMessage() override;
    void accept(VoiceMessageExecutor *executor) override;

    std::uint8_t note() const;
    std::uint8_t velocity() const;

    const Wavesample *wavesample() const;
    const Wave &sample() const;
    const std::vector<ConnectionBlock> &connectionBlocks() const;
  };

  class NoteOffMessage final : public VoiceMessage {

  public:
    NoteOffMessage();
    ~NoteOffMessage() override;
    void accept(VoiceMessageExecutor *executor) override;
  };

  class SoundOffMessage final : public VoiceMessage {
  public:
    SoundOffMessage();
    ~SoundOffMessage() override;
    void accept(VoiceMessageExecutor *executor) override;
  };

  class ControlChangeMessage final : public VoiceMessage {
    Source m_source;
    float m_value;

  public:
    ControlChangeMessage(Source source, float value);
    ~ControlChangeMessage() override;

    void accept(VoiceMessageExecutor *executor) override;

    Source source() const;
    float value() const;
  };

  class ResetControllersMessage final : public VoiceMessage {
  public:
    ResetControllersMessage();
    ~ResetControllersMessage() override;

    void accept(VoiceMessageExecutor *executor) override;
  };

  class VoiceMessageExecutor {
  public:
    virtual ~VoiceMessageExecutor();
    virtual void execute(const NoteOnMessage &message) = 0;
    virtual void execute(const NoteOffMessage &message) = 0;
    virtual void execute(const SoundOffMessage &message) = 0;
    virtual void execute(const ControlChangeMessage &message) = 0;
    virtual void execute(const ResetControllersMessage &message) = 0;
  };
} // namespace Synth
} // namespace DLSynth

#endif