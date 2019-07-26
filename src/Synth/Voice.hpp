#ifndef VOICE_HPP
#define VOICE_HPP

#include "../Articulator.hpp"
#include "../Instrument.hpp"
#include "../Wave.hpp"
#include "../Wavesample.hpp"
#include <chrono>
#include <cstdint>
#include <map>
#include <vector>

namespace DLSynth {
namespace Synth {
  class Voice {
    struct impl;
    impl *pimpl;

  public:
    Voice(const Instrument &instrument, const std::map<Source, float> &sources,
          std::uint32_t sampleRate);
    Voice(Voice &&voice);
    ~Voice();

    Voice &operator=(const Voice &) = delete;

    void noteOn(std::uint8_t note, std::uint8_t velocity);
    void noteOff(std::uint8_t velocity);
    void soundOff();

    bool playing() const;
    std::uint8_t note() const;
    std::chrono::steady_clock::time_point startTime() const;

    void render_fill(float *beginLeft, float *endLeft, float *beginRight,
                     float *endRight, float gain);

    void render_mix(float *beginLeft, float *endLeft, float *beginRight,
                    float *endRight, float gain);
  };
} // namespace Synth
} // namespace DLSynth

#endif