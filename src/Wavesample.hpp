#ifndef WAVESAMPLE_HPP
#define WAVESAMPLE_HPP

#include <cstdint>
#include <memory>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
enum class LoopType : std::uint32_t { Forward = 0x0000, Release = 0x0001 };

class WavesampleLoop final {
  LoopType m_type;
  std::uint32_t m_start;
  std::uint32_t m_length;

public:
  constexpr WavesampleLoop(LoopType type, std::uint32_t start,
                           std::uint32_t length)
    : m_type(type), m_start(start), m_length(length) {}

  constexpr LoopType type() const { return m_type; }
  constexpr std::uint32_t start() const { return m_start; }
  constexpr std::uint32_t length() const { return m_length; }
};

/// Describes the synthesis parameters of a WAV file
class Wavesample final {
  std::uint16_t m_unityNote;
  std::int16_t m_fineTune;
  std::int32_t m_gain;
  std::unique_ptr<WavesampleLoop> m_loop = nullptr;

public:
  Wavesample(riffcpp::Chunk &chunk);

  /// Returns the MIDI note at which the sample will be played at its original
  /// pitch
  std::uint16_t unityNote() const;

  /// Returns the tuning offset from \ref unityNote in cents
  float fineTune() const;

  /// Returns the gain to be applied to the sample in bels
  float gain() const;

  /// Returns the loop of the sample, if it exists
  /**
   * If this returns null, then this Wavesample is one-shot.
   */
  const WavesampleLoop *loop() const;
};
} // namespace DLSynth

#endif