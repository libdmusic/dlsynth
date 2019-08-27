#ifndef VOICE_HPP
#define VOICE_HPP

#include "../Articulator.hpp"
#include "../Instrument.hpp"
#include "../Region.hpp"
#include "../Wave.hpp"
#include "../Wavesample.hpp"
#include <chrono>
#include <cstdint>
#include <vector>

namespace DLSynth {
namespace Synth {
  class Voice final {
    struct impl;
    impl *pimpl;

  public:
    Voice(std::uint32_t sampleRate);
    Voice(Voice &&voice);
    ~Voice();

    Voice &operator=(const Voice &) = delete;

    void noteOn(int channel, int priority, std::uint8_t note,
                std::uint8_t velocity, bool isDrum,
                const Wavesample *wavesample, const Wave &sample,
                const std::vector<ConnectionBlock> &connectionBlocks);
    void noteOff();
    void soundOff();
    void sustain(bool value);

    void controlChange(Source source, float value);

    void resetControllers();

    bool playing() const;
    std::uint8_t note() const;
    int channel() const;
    int priority() const;
    std::chrono::steady_clock::time_point startTime() const;

    void render_fill(float *beginLeft, float *endLeft, float *beginRight,
                     float *endRight, std::size_t bufferSkip, float gain);

    void render_mix(float *beginLeft, float *endLeft, float *beginRight,
                    float *endRight, std::size_t bufferSkip, float gain);
  };
} // namespace Synth
} // namespace DLSynth

#endif