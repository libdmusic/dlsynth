#ifndef WAVE_HPP
#define WAVE_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <riffcpp.hpp>
#include <vector>

struct Uuid;

namespace DLSynth {
class Wavesample;
class Info;

class Wave final {
  struct impl;

  std::unique_ptr<impl> m_pimpl;

public:
  Wave(const std::vector<float> &leftData, const std::vector<float> &rightData,
       int sampleRate, const Wavesample *wavesample, const Info *info,
       const Uuid *guid) noexcept;
  Wave(Wave &&wave) noexcept;
  Wave(const Wave &wave) noexcept;
  ~Wave();

  Wave &operator=(const Wave &) noexcept;

  /// Returns the left channel audio data of this file
  const std::vector<float> &leftData() const;

  /// Returns the right channel audio data of this file
  const std::vector<float> &rightData() const;

  /// Returns the Uuid of this file, if it exists
  const Uuid *guid() const;

  /// Returns the associated @ref Wavesample object, if it exists
  const Wavesample *wavesample() const;

  /// Returns the sample rate of the WAV file
  int sampleRate() const;

  /// Returns the contents of the INFO chunk, if it exists
  const Info *info() const;

  static Wave readChunk(riffcpp::Chunk &chunk);
};
} // namespace DLSynth

#endif