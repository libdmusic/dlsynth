#ifndef WAVEDECODER_HPP
#define WAVEDECODER_HPP

#include "Wave.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace DLSynth {
class WaveDecoder {
public:
  virtual std::size_t num_frames() = 0;
  virtual void decode(float *leftBuffer, float *rightBuffer,
                      std::size_t bufferSize) = 0;

  virtual ~WaveDecoder();
};

class WaveDecoderFactory {
public:
  using WaveDecoderPtr = std::unique_ptr<WaveDecoder>;

  virtual WaveDecoderPtr createDecoder(const WaveFormat &format,
                                       std::uint32_t fact,
                                       const std::vector<char> &data) = 0;
  virtual WaveDecoderPtr createDecoder(const WaveFormat &format,
                                       const std::vector<char> &data) = 0;
  virtual WaveDecoderPtr createDecoder(const WaveFormatEx &format,
                                       std::uint32_t fact,
                                       const std::vector<char> &data) = 0;
  virtual WaveDecoderPtr createDecoder(const WaveFormatEx &format,
                                       const std::vector<char> &data) = 0;

  virtual ~WaveDecoderFactory();
};
} // namespace DLSynth

#endif