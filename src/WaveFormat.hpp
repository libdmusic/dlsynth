#ifndef WAVEFORMAT_HPP
#define WAVEFORMAT_HPP
#include <cstdint>

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
} // namespace DLSynth

#endif