#ifndef WAVEDECODER_HPP
#define WAVEDECODER_HPP

#include "Wave.hpp"
#include "WaveFormat.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace DLSynth {
/// Used to convert from the audio format stored in a WAV file to a floating
/// point representation
class WaveDecoder {
public:
  /// Returns the number of frames stored in the WAV file
  virtual std::size_t num_frames() = 0;

  /// Decodes the specified amount of data into output buffers
  virtual void
  decode(float *leftBuffer,     ///< [out] Left output buffer
         float *rightBuffer,    ///< [out] Right output buffer
         std::size_t bufferSize ///< [in] Size of the buffers in samples
         ) = 0;

  virtual ~WaveDecoder();
};

/// Creates a \ref WaveDecoder according to the WAV file parameters
class WaveDecoderFactory {
public:
  using WaveDecoderPtr = std::unique_ptr<WaveDecoder>;

  /// \see DecoderTable::getDecoder
  virtual WaveDecoderPtr createDecoder(const WaveFormat &format,
                                       std::uint32_t fact,
                                       const std::vector<char> &data) = 0;

  /// \see DecoderTable::getDecoder
  virtual WaveDecoderPtr createDecoder(const WaveFormat &format,
                                       const std::vector<char> &data) = 0;

  /// \see DecoderTable::getDecoder
  virtual WaveDecoderPtr createDecoder(const WaveFormatEx &format,
                                       std::uint32_t fact,
                                       const std::vector<char> &data) = 0;

  /// \see DecoderTable::getDecoder
  virtual WaveDecoderPtr createDecoder(const WaveFormatEx &format,
                                       const std::vector<char> &data) = 0;

  virtual ~WaveDecoderFactory();
};
} // namespace DLSynth

#endif