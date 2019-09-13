#ifndef REGION_HPP
#define REGION_HPP

#include "Articulator.hpp"
#include "ExpressionParser.hpp"
#include "Structs/Range.hpp"
#include "Wavesample.hpp"
#include <memory>

namespace DLSynth {

class Region final {
  Range m_keyRange;
  Range m_velRange;
  std::uint16_t m_keyGroup;
  bool m_selfNonExclusive;
  std::vector<ConnectionBlock> m_blocks;
  std::unique_ptr<Wavesample> m_wavesample;
  std::uint32_t m_waveIndex;

public:
  Region(const Range &keyRange, const Range &velocityRange,
         const std::vector<ConnectionBlock> &connectionBlocks,
         std::uint32_t waveIndex, bool selfNonExclusive) noexcept;

  Region(const Range &keyRange, const Range &velocityRange,
         const std::vector<ConnectionBlock> &connectionBlocks,
         std::uint32_t waveIndex, bool selfNonExclusive,
         const Wavesample &wavesample) noexcept;

  Region(Region &&region) noexcept;
  Region(const Region &region) noexcept;
  ~Region() = default;

  Region &operator=(const Region &region) noexcept;

  /// Returns the range of notes for which this region is valid
  const Range &keyRange() const noexcept;

  /// Returns the range of velocities for which this region is valid
  const Range &velocityRange() const noexcept;

  /// Returns the key group to which this region is assigned.
  /**
   * @remarks Only valid if the instrument is a drum instrument
   */
  std::uint16_t keyGroup() const noexcept;

  /// Returns the connection blocks defined for this region
  const std::vector<ConnectionBlock> &connectionBlocks() const noexcept;

  /// Returns the index of the wave associated with this region in the wavepool
  std::uint32_t waveIndex() const noexcept;

  /// Returns wavesample data that overrides the one specified in the wave, if
  /// it exists
  const Wavesample *wavesample() const noexcept;

  /// If true, playing a note while a voice is already playing the same note
  /// spawns a new voice instead of restarting the preexisting one
  bool selfNonExclusive() const noexcept;

  static Region readChunk(riffcpp::Chunk &chunk,
                          const ExpressionParser &exprParser);
};
} // namespace DLSynth

#endif