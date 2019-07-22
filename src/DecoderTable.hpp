#ifndef DECODERTABLE_HPP
#define DECODERTABLE_HPP

#include "Wave.hpp"
#include <unordered_map>

namespace DLSynth {
  class DecoderTable {
    DecoderTable();
    std::unordered_map<std::uint16_t, WaveDecoderFactory *> m_decoders;

  public:
    DecoderTable(const DecoderTable &) = delete;
    void operator=(const DecoderTable &) = delete;
    ~DecoderTable();

    WaveDecoderFactory *getFactory(std::uint16_t formatTag);

    static DecoderTable &getInstance();
  };
} // namespace DLSynth

#endif