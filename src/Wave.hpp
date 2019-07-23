#ifndef WAVE_HPP
#define WAVE_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
struct WaveFormat {
  std::uint16_t FormatTag;
  std::uint16_t NumChannels;
  std::uint32_t SamplesPerSec;
  std::uint32_t AvgBytesPerSec;
  std::uint16_t BlockAlign;

  static constexpr std::uint16_t Unknown = 0x0000;
  static constexpr std::uint16_t Pcm = 0x0001;
  static constexpr std::uint16_t MsAdpcm = 0x0002;
  static constexpr std::uint16_t Float = 0x0003;
  static constexpr std::uint16_t ALaw = 0x0006;
  static constexpr std::uint16_t MuLaw = 0x0007;
  static constexpr std::uint16_t DviAdpcm = 0x0011;
  static constexpr std::uint16_t ImaAdpcm = DviAdpcm;
};

struct WaveFormatEx {
  std::uint16_t FormatTag;
  std::uint16_t NumChannels;
  std::uint32_t SamplesPerSec;
  std::uint32_t AvgBytesPerSec;
  std::uint16_t BlockAlign;
  std::uint16_t BitsPerSample;
  std::uint16_t ExtraInfoSize;
  std::uint8_t ExtraData[];

  inline WaveFormat toWaveFormat() const {
    return {FormatTag, NumChannels, SamplesPerSec, AvgBytesPerSec, BlockAlign};
  }
};

class Wavesample;
struct Uuid;

class Wave {
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