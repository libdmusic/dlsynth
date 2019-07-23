#ifndef SOUND_HPP
#define SOUND_HPP
#include "ExpressionParser.hpp"
#include "Instrument.hpp"
#include "Wave.hpp"
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
struct Uuid;
class Sound {
  struct impl;

  impl *m_pimpl;

public:
  Sound(riffcpp::Chunk &chunk, std::uint32_t sampleRate);
  Sound(Sound &&snd);
  ~Sound();

  Sound &operator=(const Sound &) = delete;

  /// Returns the \ref Uuid of the sound, if it exists
  const Uuid *dlid() const;
};
} // namespace DLSynth

#endif