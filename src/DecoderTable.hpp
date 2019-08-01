#ifndef DECODERTABLE_HPP
#define DECODERTABLE_HPP

#include "Wave.hpp"
#include "WaveFormat.hpp"
#include <memory>
#include <unordered_map>

namespace DLSynth {
class WaveDecoderFactory;
class WaveDecoder;

/// Allows access to the supported WAV decoders
/**
 * Example usage:
 * @code
 * WaveFormat fmt; // Taken from a wave file
 * std::vector<char> waveData; // Audio data
 * auto decoder = DecoderTable::getInstance().getDecoder(fmt, waveData);
 * @endcode
 */
class DecoderTable final {
  DecoderTable();
  std::unordered_map<std::uint16_t, WaveDecoderFactory *> m_decoders;

public:
  DecoderTable(const DecoderTable &) = delete;
  void operator=(const DecoderTable &) = delete;
  ~DecoderTable();

  WaveDecoderFactory *getFactory(std::uint16_t formatTag) const;

  /// Creates a decoder for a WAV file that has a WAVEFORMAT structure and a
  /// `fact` chunk
  std::unique_ptr<WaveDecoder> getDecoder(
   const WaveFormat &format,     ///< [in] The WAVEFORMAT structure of the file
   std::uint32_t fact,           ///< [in] The contents of the `fact` chunk
   const std::vector<char> &data ///< [in] The audio data
   ) const;

  /// Creates a decoder for a WAV file that has a WAVEFORMATEX structure and a
  /// `fact` chunk
  std::unique_ptr<WaveDecoder> getDecoder(
   const WaveFormatEx &format, ///< [in] The WAVEFORMATEX structure of the file
   std::uint32_t fact,         ///< [in] The contents of the `fact` chunk
   const std::vector<char> &data ///< [in] The audio data
   ) const;

  /// Creates a decoder for a WAV file that has a WAVEFORMAT structure and no
  /// `fact` chunk
  std::unique_ptr<WaveDecoder> getDecoder(
   const WaveFormat &format,     ///< [in] The WAVEFORMAT structure of the file
   const std::vector<char> &data ///< [in] The audio data
   ) const;

  /// Creates a decoder for a WAV file that has a WAVEFORMATEX structure and no
  /// `fact` chunk
  std::unique_ptr<WaveDecoder> getDecoder(
   const WaveFormatEx &format, ///< [in] The WAVEFORMATEX structure of the file
   const std::vector<char> &data ///< [in] The audio data
   ) const;

  /// Grants access to the DecoderTable singleton instance
  static DecoderTable &getInstance();
};
} // namespace DLSynth

#endif