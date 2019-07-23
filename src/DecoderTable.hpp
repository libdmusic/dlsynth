#ifndef DECODERTABLE_HPP
#define DECODERTABLE_HPP

#include "Wave.hpp"
#include <memory>
#include <unordered_map>

namespace DLSynth {
class WaveDecoderFactory;
class WaveDecoder;
class DecoderTable {
  DecoderTable();
  std::unordered_map<std::uint16_t, WaveDecoderFactory *> m_decoders;

public:
  DecoderTable(const DecoderTable &) = delete;
  void operator=(const DecoderTable &) = delete;
  ~DecoderTable();

  WaveDecoderFactory *getFactory(std::uint16_t formatTag) const;
  std::unique_ptr<WaveDecoder> getDecoder(const WaveFormat &format,
                                          std::uint32_t fact,
                                          const std::vector<char> &data) const;
  std::unique_ptr<WaveDecoder> getDecoder(const WaveFormatEx &format,
                                          std::uint32_t fact,
                                          const std::vector<char> &data) const;
  std::unique_ptr<WaveDecoder> getDecoder(const WaveFormat &format,
                                          const std::vector<char> &data) const;
  std::unique_ptr<WaveDecoder> getDecoder(const WaveFormatEx &format,
                                          const std::vector<char> &data) const;

  static DecoderTable &getInstance();
};
} // namespace DLSynth

#endif