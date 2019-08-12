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
  Sound(const std::vector<Instrument> &instruments,
        const std::vector<Wave> &wavepool) noexcept;
  Sound(const std::vector<Instrument> &instruments,
        const std::vector<Wave> &wavepool, const Uuid &dlid) noexcept;
  Sound(const std::vector<Instrument> &instruments,
        const std::vector<Wave> &wavepool, const Info &info) noexcept;
  Sound(const std::vector<Instrument> &instruments,
        const std::vector<Wave> &wavepool, const Uuid &dlid,
        const Info &info) noexcept;
  Sound(Sound &&snd) noexcept;
  Sound(const Sound &snd) noexcept;
  ~Sound();

  Sound &operator=(const Sound &) noexcept;

  /// Returns the @ref Uuid of the sound, if it exists
  const Uuid *dlid() const noexcept;

  /// Returns the instruments contained in this collection
  const std::vector<Instrument> &instruments() const noexcept;

  /// Returns the wavepool of this collection
  const std::vector<Wave> &wavepool() const noexcept;

  /// Returns the contents of the INFO chunk, if it exists
  const Info *info() const noexcept;

  static Sound readChunk(riffcpp::Chunk &chunk, std::uint32_t sampleRate);
};
} // namespace DLSynth

#endif