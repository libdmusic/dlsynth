#include "../Error.hpp"
#include "Decoders.hpp"
#include <cassert>

using namespace DLSynth;
using namespace DLSynth::Decoders;

template <typename T> class FloatDecoder : public WaveDecoder {
  std::vector<T> m_leftData, m_rightData;

  void transferMono(const T *samples, std::size_t size) {
    m_leftData.resize(size);
    m_rightData.resize(size);

    std::copy(samples, samples + size, m_leftData.begin());
    std::copy(samples, samples + size, m_rightData.begin());
  }

  void transferStereo(const T *samples, std::size_t size) {
    m_leftData.reserve(size);
    m_rightData.reserve(size);

    for (std::size_t i = 0; i < size; i++) {
      m_leftData.push_back(samples[i * 2 + 0]);
      m_rightData.push_back(samples[i * 2 + 1]);
    }
  }

public:
  FloatDecoder(const WaveFormat &format, const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::Float);

    if (format.NumChannels < 1 || format.NumChannels > 2)
      throw Error("Invalid number of channels", ErrorCode::UNSUPPORTED_CODEC);

    const T *samples = reinterpret_cast<const T *>(data.data());
    if (format.NumChannels == 1) {
      std::size_t size = data.size() / sizeof(T);
      transferMono(samples, size);
    } else if (format.NumChannels == 2) {
      std::size_t size = (data.size() / sizeof(T)) / 2;
      transferStereo(samples, size);
    }
  }

  FloatDecoder(const WaveFormat &format, std::uint32_t fact,
               const std::vector<char> &data) {
    assert(format.FormatTag == WaveFormat::Float);

    if (format.NumChannels < 1 || format.NumChannels > 2)
      throw Error("Invalid number of channels", ErrorCode::UNSUPPORTED_CODEC);

    const T *samples = reinterpret_cast<const T *>(data.data());
    if (format.NumChannels == 1) {
      transferMono(samples, fact);
    } else if (format.NumChannels == 2) {
      transferStereo(samples, fact);
    }
  }

  virtual void decode(float *leftBuffer, float *rightBuffer,
                      std::size_t bufferSize) override {
    std::size_t len = std::min(bufferSize, m_leftData.size());
    std::transform(m_leftData.begin(), m_leftData.begin() + len, leftBuffer,
                   [](auto x) { return static_cast<float>(x); });
    std::transform(m_rightData.begin(), m_rightData.begin() + len, rightBuffer,
                   [](auto x) { return static_cast<float>(x); });
  }

  virtual std::size_t num_frames() override { return m_leftData.size(); }

  ~FloatDecoder() override = default;
};

FloatDecoderFactory::FloatDecoderFactory() = default;
FloatDecoderFactory::~FloatDecoderFactory() = default;

std::unique_ptr<WaveDecoder>
FloatDecoderFactory::createDecoder(const WaveFormat &format,
                                   const std::vector<char> &data) {
  const auto bitsPerSample = (format.AvgBytesPerSec / format.SamplesPerSec) * 8;
  switch (bitsPerSample) {
  case 32:
    return std::make_unique<FloatDecoder<float>>(format, data);
  case 64:
    return std::make_unique<FloatDecoder<double>>(format, data);
  default:
    throw Error("Invalid encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

std::unique_ptr<WaveDecoder>
FloatDecoderFactory::createDecoder(const WaveFormat &format, std::uint32_t fact,
                                   const std::vector<char> &data) {
  const auto bitsPerSample = (format.AvgBytesPerSec / format.SamplesPerSec) * 8;
  switch (bitsPerSample) {
  case 32:
    return std::make_unique<FloatDecoder<float>>(format, fact, data);
  case 64:
    return std::make_unique<FloatDecoder<double>>(format, fact, data);
  default:
    throw Error("Invalid encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

std::unique_ptr<WaveDecoder>
FloatDecoderFactory::createDecoder(const WaveFormatEx &format,
                                   const std::vector<char> &data) {
  switch (format.BitsPerSample) {
  case 32:
    return std::make_unique<FloatDecoder<float>>(format.toWaveFormat(), data);
  case 64:
    return std::make_unique<FloatDecoder<double>>(format.toWaveFormat(), data);
  default:
    throw Error("Invalid encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

std::unique_ptr<WaveDecoder>
FloatDecoderFactory::createDecoder(const WaveFormatEx &format,
                                   std::uint32_t fact,
                                   const std::vector<char> &data) {
  switch (format.BitsPerSample) {
  case 32:
    return std::make_unique<FloatDecoder<float>>(format.toWaveFormat(), fact,
                                                 data);
  case 64:
    return std::make_unique<FloatDecoder<double>>(format.toWaveFormat(), fact,
                                                  data);
  default:
    throw Error("Invalid encoding", ErrorCode::UNSUPPORTED_CODEC);
  }
}

FloatDecoderFactory *FloatDecoderFactory::getInstance() {
  static FloatDecoderFactory factory;
  return &factory;
}