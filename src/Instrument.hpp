#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP
#include "ExpressionParser.hpp"
#include <cstdint>
#include <riffcpp.hpp>

namespace DLSynth {
struct Uuid;
class Instrument {
  struct impl;
  impl *m_pimpl;

public:
  Instrument(riffcpp::Chunk &chunk, const ExpressionParser &exprParser);
  Instrument(Instrument &&instr);
  ~Instrument();

  Instrument &operator=(const Instrument &) = delete;

  const Uuid *dlid() const;
  std::uint32_t midiBank() const;
  std::uint32_t midiInstrument() const;
};
} // namespace DLSynth

#endif