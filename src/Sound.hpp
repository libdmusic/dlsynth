#ifndef SOUND_HPP
#define SOUND_HPP
#include "ExpressionParser.hpp"
#include "Instrument.hpp"
#include "Wave.hpp"
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
struct Uuid;
class Info;
/// A DLS Collection of instruments
class Sound final {
  struct impl;

  impl *m_pimpl;

public:
  Sound(riffcpp::Chunk &chunk, std::uint32_t sampleRate);
  Sound(Sound &&snd);
  ~Sound();

  Sound &operator=(const Sound &) = delete;

  /// Returns the \ref Uuid of the sound, if it exists
  const Uuid *dlid() const;

  /// Returns the instruments contained in this collection
  const std::vector<Instrument> &instruments() const;

  /// Returns the wavepool of this collection
  const std::vector<Wave> &wavepool() const;

  /// Returns the contents of the INFO chunk, if it exists
  const Info *info() const;
};
} // namespace DLSynth

#endif