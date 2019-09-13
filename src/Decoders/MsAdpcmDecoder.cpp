#include "../Error.hpp"
#include "../NumericUtils.hpp"
#include "../Structs/AdpcmCoeffs.hpp"
#include "../Structs/AdpcmFormat.hpp"
#include "Decoders.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>

using namespace DLSynth;
using namespace DLSynth::Decoders;

template <typename TIn, typename TOut> constexpr TOut clamp(TIn value) {
  if (value > std::numeric_limits<TOut>::max())
    return std::numeric_limits<TOut>::max();
  else if (value < std::numeric_limits<TOut>::min())
    return std::numeric_limits<TOut>::min();
  else
    return (TOut)value;
}

template <int NumChannels> struct AdpcmHeader {
  std::array<std::uint8_t, NumChannels> predictor;
  std::array<std::uint16_t, NumChannels> iDelta;
  std::array<std::int16_t, NumChannels> samp1;
  std::array<std::int16_t, NumChannels> samp2;

  static AdpcmHeader<NumChannels> read(const char *data) {
    AdpcmHeader<NumChannels> header;

    for (int i = 0; i < NumChannels; i++) {
      header.predictor[i] = *(data++);
    }

    for (int i = 0; i < NumChannels; i++) {
#ifndef DLSYNTH_BIGENDIAN
      std::uint16_t value = *reinterpret_cast<const std::uint16_t *>(data);
#else
      std::array<char, 2> buf = {data[0], data[1]};
      std::reverse(buf.begin(), buf.end());
      std::uint16_t value =
       *reinterpret_cast<const std::uint16_t *>(buf.data());
#endif
      data += sizeof(std::uint16_t);

      header.iDelta[i] = value;
    }

    for (int i = 0; i < NumChannels; i++) {
#ifndef DLSYNTH_BIGENDIAN
      std::int16_t value = *reinterpret_cast<const std::int16_t *>(data);
#else
      std::array<char, 2> buf = {data[0], data[1]};
      std::reverse(buf.begin(), buf.end());
      std::int16_t value = *reinterpret_cast<const std::int16_t *>(buf.data());
#endif
      data += sizeof(std::int16_t);

      header.samp1[i] = value;
    }

    for (int i = 0; i < NumChannels; i++) {
#ifndef DLSYNTH_BIGENDIAN
      std::int16_t value = *reinterpret_cast<const std::int16_t *>(data);
#else
      std::array<char, 2> buf = {data[0], data[1]};
      std::reverse(buf.begin(), buf.end());
      std::int16_t value = *reinterpret_cast<const std::int16_t *>(buf.data());
#endif
      data += sizeof(std::int16_t);

      header.samp2[i] = value;
    }

    return header;
  }
};

constexpr int AdaptationTable[] = {230, 230, 230, 230, 307, 409, 512, 614,
                                   768, 614, 512, 409, 307, 230, 230, 230};

class MsAdpcmDecoder : public WaveDecoder {
  std::vector<std::int16_t> m_leftData, m_rightData;
  AdpcmFormat m_format;

  template <int NumChannels>
  void decodeBlock(const char *data, std::size_t blockSize) {
    if (blockSize < sizeof(AdpcmHeader<NumChannels>))
      return;

    auto header = AdpcmHeader<NumChannels>::read(data);
    auto dataEnd = data + blockSize;
    data += sizeof(header);

    std::array<std::int16_t, NumChannels> samp1 = header.samp1;
    std::array<std::int16_t, NumChannels> samp2 = header.samp2;

    std::array<std::vector<std::int16_t> *, 2> channels = {&m_leftData,
                                                           &m_rightData};

    for (int i = 0; i < NumChannels; i++) {
      channels[i]->push_back(samp2[i]);
      channels[i]->push_back(samp1[i]);
    }

    while (data < dataEnd) {
      std::uint8_t nextByte = *(data++);

      std::array<int, 2> nibbles = {
       ((std::int8_t)(nextByte & 0xF0) >> 4),
       ((std::int8_t)((nextByte & 0x0F) << 4) >> 4)};

      std::array<std::uint8_t, 2> unibbles = {
       (std::uint8_t)(((std::uint8_t)nextByte & 0xF0) >> 4),
       (std::uint8_t)((std::int8_t)nextByte & 0x0F)};

      for (int i = 0; i < NumChannels; i++) {
        for (int j = 0; j < 3 - NumChannels; j++) {
          std::int8_t errorDelta = nibbles[j + i];
          std::uint8_t uErrorDelta = unibbles[j + i];

          auto predictor = header.predictor[i];
          if (predictor >= m_format.coeffs.size()) {
            throw Error("Invalid ADPCM predictor index",
                        ErrorCode::INVALID_FILE);
          }
          const auto &coeffs = m_format.coeffs[predictor];

          std::int32_t predSamp =
           ((samp1[i] * coeffs.iCoef1) + (samp2[i] * coeffs.iCoef2)) >> 8;
          std::int32_t newSamp = predSamp + (header.iDelta[i] * errorDelta);
          std::int16_t calcSamp = clamp<std::int32_t, std::int16_t>(newSamp);

          channels[i]->push_back(calcSamp);

          header.iDelta[i] =
           (header.iDelta[i] * AdaptationTable[uErrorDelta]) >> 8;

          if (header.iDelta[i] < 16)
            header.iDelta[i] = 16;

          samp2[i] = samp1[i];
          samp1[i] = calcSamp;
        }
      }
    }
  }

public:
  MsAdpcmDecoder(const WaveFormatEx &format, std::uint32_t fact,
                 const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::MsAdpcm);

    if (format.BitsPerSample != 4) {
      throw Error("Only 4-bit Microsoft ADPCM files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
    }

    if (format.NumChannels > 2) {
      throw Error("Only mono and stereo files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
    }

    if (format.ExtraInfoSize < 32) {
      throw Error("Invalid MS ADPCM predictor data", ErrorCode::INVALID_FILE);
    }

    m_leftData.reserve(fact);
    m_rightData.reserve(fact);

    m_format = readVector<AdpcmFormat>(format.ExtraData);

    std::size_t numBlocks = data.size() / format.BlockAlign;
    if (data.size() % format.BlockAlign)
      numBlocks++;

    const char *blockData = data.data();
    const char *blockDataEnd = blockData + data.size();
    for (std::size_t i = 0; i < numBlocks; i++) {
      auto remainingSize = blockDataEnd - blockData;
      auto blockSize =
       remainingSize < format.BlockAlign ? remainingSize : format.BlockAlign;
      if (format.NumChannels == 1) {
        decodeBlock<1>(blockData, blockSize);
      } else {
        decodeBlock<2>(blockData, blockSize);
      }
      blockData = blockData + format.BlockAlign;
    }

    if (format.NumChannels == 1) {
      m_rightData.resize(m_leftData.size());
      std::copy(m_leftData.begin(), m_leftData.end(), m_rightData.begin());

      m_leftData.resize(fact);
      m_rightData.resize(fact);
    }
  }

  MsAdpcmDecoder(const WaveFormatEx &format, const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::MsAdpcm);

    if (format.BitsPerSample != 4) {
      throw Error("Only 4-bit Microsoft ADPCM files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
    }

    if (format.NumChannels > 2) {
      throw Error("Only mono and stereo files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
    }

    if (format.ExtraInfoSize < 32) {
      throw Error("Invalid MS ADPCM predictor data", ErrorCode::INVALID_FILE);
    }

    m_format = readVector<AdpcmFormat>(format.ExtraData);

    std::size_t numBlocks = data.size() / format.BlockAlign;
    if (data.size() % format.BlockAlign)
      numBlocks++;

    m_leftData.reserve(numBlocks * m_format.samplesPerBlock);
    m_rightData.reserve(numBlocks * m_format.samplesPerBlock);

    const char *blockData = data.data();
    const char *blockDataEnd = blockData + data.size();
    for (std::size_t i = 0; i < numBlocks; i++) {
      auto remainingSize = blockDataEnd - blockData;
      auto blockSize =
       remainingSize < format.BlockAlign ? remainingSize : format.BlockAlign;
      if (format.NumChannels == 1) {
        decodeBlock<1>(blockData, blockSize);
      } else {
        decodeBlock<2>(blockData, blockSize);
      }
      blockData = blockData + format.BlockAlign;
    }

    if (format.NumChannels == 1) {
      m_rightData.resize(m_leftData.size());
      std::copy(m_leftData.begin(), m_leftData.end(), m_rightData.begin());
    }
  }

  virtual void decode(float *leftBuffer, float *rightBuffer,
                      std::size_t bufferSize) override {
    std::size_t len = std::min(bufferSize, m_leftData.size());
    std::transform(m_leftData.begin(), m_leftData.begin() + len, leftBuffer,
                   [](auto x) { return normalize(x); });
    std::transform(m_rightData.begin(), m_rightData.begin() + len, rightBuffer,
                   [](auto x) { return normalize(x); });
  }

  virtual std::size_t num_frames() override { return m_leftData.size(); }

  ~MsAdpcmDecoder() override = default;
};

MsAdpcmDecoderFactory::MsAdpcmDecoderFactory() = default;
MsAdpcmDecoderFactory::~MsAdpcmDecoderFactory() = default;

std::unique_ptr<WaveDecoder>
MsAdpcmDecoderFactory::createDecoder(const WaveFormat &format,
                                     const std::vector<char> &data) {
  throw Error("Microsoft ADPCM needs WAVEFORMATEX", ErrorCode::INVALID_FILE);
}

std::unique_ptr<WaveDecoder> MsAdpcmDecoderFactory::createDecoder(
 const WaveFormat &format, std::uint32_t fact, const std::vector<char> &data) {
  throw Error("Microsoft ADPCM needs WAVEFORMATEX", ErrorCode::INVALID_FILE);
}

std::unique_ptr<WaveDecoder>
MsAdpcmDecoderFactory::createDecoder(const WaveFormatEx &format,
                                     const std::vector<char> &data) {
  return std::make_unique<MsAdpcmDecoder>(format, data);
}

std::unique_ptr<WaveDecoder>
MsAdpcmDecoderFactory::createDecoder(const WaveFormatEx &format,
                                     std::uint32_t fact,
                                     const std::vector<char> &data) {
  return std::make_unique<MsAdpcmDecoder>(format, fact, data);
}

MsAdpcmDecoderFactory *MsAdpcmDecoderFactory::getInstance() {
  static MsAdpcmDecoderFactory factory;
  return &factory;
}