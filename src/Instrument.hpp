#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP
#include "Articulator.hpp"
#include "ExpressionParser.hpp"
#include "Region.hpp"
#include <cstdint>
#include <riffcpp.hpp>

namespace DLSynth {
struct Uuid;
class Info;

/// A DLS instrument, part of a DLS Collection
class Instrument final {
  struct impl;
  impl *m_pimpl;

public:
  Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
             bool isDrumInstrument,
             const std::vector<ConnectionBlock> &connectionBlocks,
             const std::vector<Region> &regions) noexcept;
  Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
             bool isDrumInstrument,
             const std::vector<ConnectionBlock> &connectionBlocks,
             const std::vector<Region> &regions, const Uuid &dlid) noexcept;
  Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
             bool isDrumInstrument,
             const std::vector<ConnectionBlock> &connectionBlocks,
             const std::vector<Region> &regions, const Info &info) noexcept;
  Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
             bool isDrumInstrument,
             const std::vector<ConnectionBlock> &connectionBlocks,
             const std::vector<Region> &regions, const Uuid &dlid,
             const Info &info) noexcept;
  Instrument(Instrument &&instr) noexcept;
  Instrument(const Instrument &instr) noexcept;
  ~Instrument();

  Instrument &operator=(const Instrument &) noexcept;

  /// Returns the \ref Uuid of this instrument, if it exists
  const Uuid *dlid() const noexcept;

  /// Returns the MIDI bank location of this instrument
  std::uint32_t midiBank() const noexcept;

  /// Returns the MIDI Program Change of this instrument
  std::uint32_t midiInstrument() const noexcept;

  /// Returns whether this is a drum or melodic instrument
  bool isDrumInstrument() const noexcept;

  /// Returns the connection blocks defined for this instrument
  const std::vector<ConnectionBlock> &connectionBlocks() const noexcept;

  /// Returns the regions defined for this instrument
  const std::vector<Region> &regions() const noexcept;

  /// Returns the contents of the INFO chunk, if it exists
  const Info *info() const noexcept;

  static Instrument readChunk(riffcpp::Chunk &chunk,
                              const ExpressionParser &exprParser);
};
} // namespace DLSynth

#endif