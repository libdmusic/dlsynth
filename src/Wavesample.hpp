#ifndef WAVESAMPLE_HPP
#define WAVESAMPLE_HPP

#include <cstdint>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
enum class LoopType : std::uint32_t { Forward = 0x0000, Release = 0x0001 };

struct wsmp_loop;

class WavesampleLoop {
  LoopType m_type;
  std::uint32_t m_start;
  std::uint32_t m_length;

public:
  WavesampleLoop(const wsmp_loop *loop);

  LoopType type() const;
  std::uint32_t start() const;
  std::uint32_t length() const;
};

/// Describes the synthesis parameters of a WAV file
class Wavesample {
  std::uint16_t m_unityNote;
  std::int16_t m_fineTune;
  std::int32_t m_gain;
  std::vector<WavesampleLoop> m_loops;

public:
  Wavesample(riffcpp::Chunk &chunk);

  /// Returns the MIDI note at which the sample will be played at its original
  /// pitch
  std::uint16_t unityNote() const;

  /// Returns the tuning offset from \ref unityNote specified in relative pitch
  /// units
  std::int16_t fineTune() const;

  /// Returns the gain to be applied to the sample in absolute gain units
  std::int32_t gain() const;

  /// Returns the loops of the sample
  const std::vector<WavesampleLoop> &loops() const;
};
} // namespace DLSynth

#endif