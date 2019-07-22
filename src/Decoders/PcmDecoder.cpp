#include "../Error.hpp"
#include "../NumericUtils.hpp"
#include "Decoders.hpp"
#include <array>
#include <cassert>
#include <limits>

using namespace DLSynth;
using namespace DLSynth::Decoders;

template <typename T> class PcmDecoder : public WaveDecoder {
  std::vector<T> m_leftData, m_rightData;

  void loadMono(const T *data, std::size_t size) {
    m_leftData.resize(size);
    m_rightData.resize(size);
    std::copy(data, data + size, m_leftData.begin());
    std::copy(data, data + size, m_rightData.begin());
  }

  void loadStereo(const T *data, std::size_t size) {
    m_leftData.reserve(size);
    m_rightData.reserve(size);
    for (std::size_t i = 0; i < size; i += 2) {
      auto valueL = data[i + 0];
      auto valueR = data[i + 1];
      m_leftData.push_back(valueL);
      m_rightData.push_back(valueR);
    }
  }

public:
  PcmDecoder(const WaveFormat &format, const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::Pcm);

    auto samples = reinterpret_cast<const T *>(data.data());
    if (format.NumChannels == 1) {
      loadMono(samples, data.size() / sizeof(T));
    } else if (format.NumChannels == 2) {
      loadStereo(samples, data.size() / sizeof(T));
    } else {
      throw Error("Only mono and stereo files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
    }
  }

  PcmDecoder(const WaveFormat &format, std::uint32_t fact,
             const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::Pcm);

    auto samples = reinterpret_cast<const T *>(data.data());
    if (format.NumChannels == 1) {
      loadMono(samples, fact);
    } else if (format.NumChannels == 2) {
      loadStereo(samples, fact);
    } else {
      throw Error("Only mono and stereo files are supported",
                  ErrorCode::UNSUPPORTED_CODEC);
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

  ~PcmDecoder() override = default;
};

PcmDecoderFactory::PcmDecoderFactory() = default;
PcmDecoderFactory::~PcmDecoderFactory() = default;

std::unique_ptr<WaveDecoder>
PcmDecoderFactory::createDecoder(const WaveFormat &format,
                                 const std::vector<char> &data) {
  switch (format.BlockAlign) {
  case 1:
    return std::make_unique<PcmDecoder<std::uint8_t>>(format, data);
  case 2:
    return std::make_unique<PcmDecoder<std::int16_t>>(format, data);
  case 3:
    return std::make_unique<PcmDecoder<int24_t>>(format, data);
  case 4:
    return std::make_unique<PcmDecoder<std::int32_t>>(format, data);
  default:
    throw Error("Unsupported encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

std::unique_ptr<WaveDecoder>
PcmDecoderFactory::createDecoder(const WaveFormat &format, std::uint32_t fact,
                                 const std::vector<char> &data) {
  switch (format.BlockAlign) {
  case 1:
    return std::make_unique<PcmDecoder<std::uint8_t>>(format, fact, data);
  case 2:
    return std::make_unique<PcmDecoder<std::int16_t>>(format, fact, data);
  case 3:
    return std::make_unique<PcmDecoder<int24_t>>(format, fact, data);
  case 4:
    return std::make_unique<PcmDecoder<std::int32_t>>(format, fact, data);
  default:
    throw Error("Unsupported encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

std::unique_ptr<WaveDecoder>
PcmDecoderFactory::createDecoder(const WaveFormatEx &format,
                                 const std::vector<char> &data) {
  return createDecoder(format.toWaveFormat(), data);
}

std::unique_ptr<WaveDecoder>
PcmDecoderFactory::createDecoder(const WaveFormatEx &format, std::uint32_t fact,
                                 const std::vector<char> &data) {
  return createDecoder(format.toWaveFormat(), fact, data);
}

PcmDecoderFactory *PcmDecoderFactory::getInstance() {
  static PcmDecoderFactory factory;
  return &factory;
}