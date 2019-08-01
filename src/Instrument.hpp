#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP
#include "Articulator.hpp"
#include "ExpressionParser.hpp"
#include "Region.hpp"
#include <cstdint>
#include <riffcpp.hpp>

namespace DLSynth {
struct Uuid;
/// A DLS instrument, part of a DLS Collection
class Instrument final {
  struct impl;
  impl *m_pimpl;

public:
  Instrument(riffcpp::Chunk &chunk, const ExpressionParser &exprParser);
  Instrument(Instrument &&instr);
  ~Instrument();

  Instrument &operator=(const Instrument &) = delete;

  /// Returns the \ref Uuid of this instrument, if it exists
  const Uuid *dlid() const;

  /// Returns the MIDI bank location of this instrument
  std::uint32_t midiBank() const;

  /// Returns the MIDI Program Change of this instrument
  std::uint32_t midiInstrument() const;

  /// Returns the connection blocks defined for this instrument
  const std::vector<ConnectionBlock> &connectionBlocks() const;

  /// Returns the regions defined for this instrument
  const std::vector<Region> &regions() const;
};
} // namespace DLSynth

#endif