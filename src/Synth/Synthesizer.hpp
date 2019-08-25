#ifndef SYNTHESIZER_HPP
#define SYNTHESIZER_HPP

#include "../Sound.hpp"

namespace DLSynth {
namespace Synth {
  /// A @ref Synthesizer coordinates a number of @ref Voice objects in order
  /// to produce audio data according to what an @ref Instrument specifies
  class Synthesizer final {
    struct impl;
    impl *pimpl;

  public:
    Synthesizer(std::size_t voiceCount, std::uint32_t sampleRate);
    Synthesizer(Synthesizer &&synth);
    ~Synthesizer();
    Synthesizer &operator=(const Synthesizer &) = delete;

    /// Sends a MIDI Note On event
    void noteOn(const Sound &collection, std::size_t instrumentIndex,
                int channel, std::uint8_t note, std::uint8_t velocity);

    /// Sends a MIDI Note Off event
    void noteOff(int channel, std::uint8_t note);

    /// Sends a MIDI Poly Pressure (Aftertouch) event
    void pressure(int channel, std::uint8_t note, std::uint8_t value);

    /// Sends a MIDI Channel Pressure (Aftertouch) event
    void pressure(int channel, std::uint8_t value);

    /// Sends a MIDI Pitch Bend change event
    void pitchBend(int channel, std::uint16_t value);

    /// Sends a volume change event
    void volume(int channel, std::uint8_t value);

    /// Sends a pan value change event
    void pan(int channel, std::uint8_t value);

    /// Sends a modulation value change event
    void modulation(int channel, std::uint8_t value);

    /// Changes the value of the sustain
    void sustain(int channel, bool status);

    /// Sends a reverb value change event
    void reverb(int channel, std::uint8_t value);

    /// Sends a chorus value change event
    void chorus(int channel, std::uint8_t value);

    /// Sends a pitch bend range change event
    void pitchBendRange(int channel, std::uint16_t value);

    /// Sends a fine tuning change event
    void fineTuning(int channel, std::uint16_t value);

    /// Sends a coarse tuning change event
    void coarseTuning(int channel, std::uint16_t value);

    /// Resets all MIDI Control Change to their default values
    void resetControllers(int channel);

    /// Sends a MIDI All Notes Off event
    void allNotesOff();

    /// Sends a MIDI All Sound Off event
    void allSoundOff();

    /// Overwrites the specified buffers with rendered audio data
    void render_fill(float *beginLeft, float *endLeft, float *beginRight,
                     float *endRight, std::size_t bufferSkip, float gain);

    /// Mixes additional rendered audio data on top of the specified buffers
    void render_mix(float *beginLeft, float *endLeft, float *beginRight,
                    float *endRight, std::size_t bufferSkip, float gain);
  };
} // namespace Synth
} // namespace DLSynth

#endif