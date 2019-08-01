#ifndef WAVE_HPP
#define WAVE_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
class Wavesample;
struct Uuid;

class Wave final {
  struct impl;

  impl *m_pimpl;

public:
  Wave(riffcpp::Chunk &chunk);
  Wave(Wave &&wave);
  ~Wave();

  Wave &operator=(const Wave &) = delete;

  /// Returns the left channel audio data of this file
  const std::vector<float> &leftData() const;

  /// Returns the right channel audio data of this file
  const std::vector<float> &rightData() const;

  /// Returns the \ref Uuid of this file, if it exists
  const Uuid *guid() const;

  /// Returns the associated \ref Wavesample object, if it exists
  const Wavesample *wavesample() const;

  /// Returns the sample rate of the WAV file
  int sampleRate() const;
};
} // namespace DLSynth

#endif