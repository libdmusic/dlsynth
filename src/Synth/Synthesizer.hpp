#ifndef SYNTHESIZER_HPP
#define SYNTHESIZER_HPP

#include "../Sound.hpp"

namespace DLSynth {
namespace Synth {
  /// A \ref Synthesizer coordinates a number of \ref Voice objects in order
  /// to produce audio data according to what an \ref Instrument specifies
  class Synthesizer {
    struct impl;
    impl *pimpl;

  public:
    Synthesizer(const Sound &collection, std::size_t instrumentIndex,
                std::size_t voiceCount, std::uint32_t sampleRate);
    Synthesizer(Synthesizer &&synth);
    ~Synthesizer();
    Synthesizer &operator=(const Synthesizer &) = delete;

    /// Sends a MIDI Note On event
    void noteOn(std::uint8_t note, std::uint8_t velocity);

    /// Sends a MIDI Note Off event
    void noteOff(std::uint8_t note, std::uint8_t velocity);

    /// Sends a MIDI Poly Pressure (Aftertouch) event
    void pressure(std::uint8_t note, std::uint8_t value);

    /// Sends a MIDI Channel Pressure (Aftertouch) event
    void pressure(std::uint8_t value);

    /// Sends a MIDI Pitch Bend change event
    void pitchBend(std::uint16_t value);

    /// Sends a volume change event
    void volume(std::uint8_t value);

    /// Sends a pan value change event
    void pan(std::uint8_t value);

    /// Sends a modulation value change event
    void modulation(std::uint8_t value);

    /// Changes the value of the sustain
    void sustain(bool status);

    /// Sends a reverb value change event
    void reverb(std::uint8_t value);

    /// Sends a chorus value change event
    void chorus(std::uint8_t value);

    /// Sends a pitch bend range change event
    void pitchBendRange(std::uint16_t value);

    /// Sends a fine tuning change event
    void fineTuning(std::uint16_t value);

    /// Sends a coarse tuning change event
    void coarseTuning(std::uint16_t value);

    /// Resets all MIDI Control Change to their default values
    void resetControllers();

    /// Sends a MIDI All Notes Off event
    void allNotesOff();

    /// Sends a MIDI All Sound Off event
    void allSoundOff();

    /// Overwrites the specified buffers with rendered audio data
    void render_fill(float *beginLeft, float *endLeft, float *beginRight,
                     float *endRight, float gain);

    /// Mixes additional rendered audio data on top of the specified buffers
    void render_mix(float *beginLeft, float *endLeft, float *beginRight,
                    float *endRight, float gain);
  };
} // namespace Synth
} // namespace DLSynth

#endif