#ifndef REGION_HPP
#define REGION_HPP

#include "Articulator.hpp"
#include "ExpressionParser.hpp"
#include "Wavesample.hpp"
#include <memory>

namespace DLSynth {

/// Represents a range of values
struct Range {
  std::uint16_t low;  ///< Lower bound
  std::uint16_t high; ///< Higher bound

  /// Returns `true` if \p value falls within the range
  constexpr bool inRange(std::uint16_t value) const {
    return value <= high && value >= low;
  }
};

class Region final {
  Range m_keyRange;
  Range m_velRange;
  std::uint16_t m_keyGroup;
  bool m_selfNonExclusive;
  std::vector<ConnectionBlock> m_blocks;
  std::unique_ptr<Wavesample> m_wavesample;
  std::uint32_t m_waveIndex;

  void load_header(riffcpp::Chunk &chunk);
  void load_wavelink(riffcpp::Chunk &chunk);

public:
  Region(riffcpp::Chunk &chunk, const ExpressionParser &exprParser);

  /// Returns the range of notes for which this region is valid
  const Range &keyRange() const;

  /// Returns the range of velocities for which this region is valid
  const Range &velocityRange() const;

  /// Returns the key group to which this region is assigned.
  /**
   * @remarks Only valid if the instrument is a drum instrument
   */
  std::uint16_t keyGroup() const;

  /// Returns the connection blocks defined for this region
  const std::vector<ConnectionBlock> &connectionBlocks() const;

  /// Returns the index of the wave associated with this region in the wavepool
  std::uint32_t waveIndex() const;

  /// Returns wavesample data that overrides the one specified in the wave, if
  /// it exists
  const Wavesample *wavesample() const;

  /// If true, playing a note while a voice is already playing the same note
  /// spawns a new voice instead of restarting the preexisting one
  bool selfNonExclusive() const;
};
} // namespace DLSynth

#endif