#include "../Error.hpp"
#include "../NumericUtils.hpp"
#include "Decoders.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>

using namespace DLSynth;
using namespace DLSynth::Decoders;

// Code adapted from dr_wav.h by David Reid
// https://github.com/mackron/dr_libs/blob/master/dr_wav.h

constexpr std::array<std::int16_t, 256> aLawTable = {
 -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,  -7552,  -7296,
 -8064,  -7808,  -6528,  -6272,  -7040,  -6784,  -2752,  -2624,  -3008,  -2880,
 -2240,  -2112,  -2496,  -2368,  -3776,  -3648,  -4032,  -3904,  -3264,  -3136,
 -3520,  -3392,  -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
 -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136, -11008, -10496,
 -12032, -11520, -8960,  -8448,  -9984,  -9472,  -15104, -14592, -16128, -15616,
 -13056, -12544, -14080, -13568, -344,   -328,   -376,   -360,   -280,   -264,
 -312,   -296,   -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
 -88,    -72,    -120,   -104,   -24,    -8,     -56,    -40,    -216,   -200,
 -248,   -232,   -152,   -136,   -184,   -168,   -1376,  -1312,  -1504,  -1440,
 -1120,  -1056,  -1248,  -1184,  -1888,  -1824,  -2016,  -1952,  -1632,  -1568,
 -1760,  -1696,  -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
 -944,   -912,   -1008,  -976,   -816,   -784,   -880,   -848,   5504,   5248,
 6016,   5760,   4480,   4224,   4992,   4736,   7552,   7296,   8064,   7808,
 6528,   6272,   7040,   6784,   2752,   2624,   3008,   2880,   2240,   2112,
 2496,   2368,   3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
 22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,  30208,  29184,
 32256,  31232,  26112,  25088,  28160,  27136,  11008,  10496,  12032,  11520,
 8960,   8448,   9984,   9472,   15104,  14592,  16128,  15616,  13056,  12544,
 14080,  13568,  344,    328,    376,    360,    280,    264,    312,    296,
 472,    456,    504,    488,    408,    392,    440,    424,    88,     72,
 120,    104,    24,     8,      56,     40,     216,    200,    248,    232,
 152,    136,    184,    168,    1376,   1312,   1504,   1440,   1120,   1056,
 1248,   1184,   1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
 688,    656,    752,    720,    560,    528,    624,    592,    944,    912,
 1008,   976,    816,    784,    880,    848};

constexpr std::array<std::int16_t, 256> muLawTable = {
 -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956, -23932, -22908,
 -21884, -20860, -19836, -18812, -17788, -16764, -15996, -15484, -14972, -14460,
 -13948, -13436, -12924, -12412, -11900, -11388, -10876, -10364, -9852,  -9340,
 -8828,  -8316,  -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
 -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,  -3900,  -3772,
 -3644,  -3516,  -3388,  -3260,  -3132,  -3004,  -2876,  -2748,  -2620,  -2492,
 -2364,  -2236,  -2108,  -1980,  -1884,  -1820,  -1756,  -1692,  -1628,  -1564,
 -1500,  -1436,  -1372,  -1308,  -1244,  -1180,  -1116,  -1052,  -988,   -924,
 -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,   -620,   -588,
 -556,   -524,   -492,   -460,   -428,   -396,   -372,   -356,   -340,   -324,
 -308,   -292,   -276,   -260,   -244,   -228,   -212,   -196,   -180,   -164,
 -148,   -132,   -120,   -112,   -104,   -96,    -88,    -80,    -72,    -64,
 -56,    -48,    -40,    -32,    -24,    -16,    -8,     0,      32124,  31100,
 30076,  29052,  28028,  27004,  25980,  24956,  23932,  22908,  21884,  20860,
 19836,  18812,  17788,  16764,  15996,  15484,  14972,  14460,  13948,  13436,
 12924,  12412,  11900,  11388,  10876,  10364,  9852,   9340,   8828,   8316,
 7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,   5884,   5628,
 5372,   5116,   4860,   4604,   4348,   4092,   3900,   3772,   3644,   3516,
 3388,   3260,   3132,   3004,   2876,   2748,   2620,   2492,   2364,   2236,
 2108,   1980,   1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
 1372,   1308,   1244,   1180,   1116,   1052,   988,    924,    876,    844,
 812,    780,    748,    716,    684,    652,    620,    588,    556,    524,
 492,    460,    428,    396,    372,    356,    340,    324,    308,    292,
 276,    260,    244,    228,    212,    196,    180,    164,    148,    132,
 120,    112,    104,    96,     88,     80,     72,     64,     56,     48,
 40,     32,     24,     16,     8,      0};

class LogarithmicDecoder : public WaveDecoder {
  std::vector<std::int16_t> m_leftData, m_rightData;

  void convertMono(const std::vector<char> &data,
                   const std::array<std::int16_t, 256> &table,
                   std::size_t frames) {

    const std::uint8_t *samples =
     reinterpret_cast<const std::uint8_t *>(data.data());

    m_leftData.resize(frames);
    m_rightData.resize(frames);

    std::transform(samples, samples + frames, m_leftData.begin(),
                   [&table](auto x) { return table[x]; });
    std::copy(m_leftData.begin(), m_leftData.end(), m_rightData.begin());
  }

  void convertStereo(const std::vector<char> &data,
                     const std::array<std::int16_t, 256> &table,
                     std::size_t frames) {

    const std::uint8_t *samples =
     reinterpret_cast<const std::uint8_t *>(data.data());

    m_leftData.reserve(frames);
    m_rightData.reserve(frames);

    for (std::size_t i = 0; i < frames; i++) {
      std::int16_t lval = table[samples[i * 2 + 0]];
      std::int16_t rval = table[samples[i * 2 + 1]];
      m_leftData.push_back(lval);
      m_rightData.push_back(rval);
    }
  }

public:
  LogarithmicDecoder(const WaveFormat &format, const std::vector<char> &data,
                     const std::array<std::int16_t, 256> &table) {
    assert(format.FormatTag == WaveFormat::ALaw ||
           format.FormatTag == WaveFormat::MuLaw);

    const std::uint8_t *samples =
     reinterpret_cast<const std::uint8_t *>(data.data());
    if (format.NumChannels == 1) {
      convertMono(data, table, data.size());
    } else if (format.NumChannels == 2) {
      convertStereo(data, table, data.size() / 2);
    } else {
      throw Error("Invalid number of channels", ErrorCode::UNSUPPORTED_CODEC);
    }
  }
  LogarithmicDecoder(const WaveFormat &format, std::uint32_t fact,
                     const std::vector<char> &data,
                     const std::array<std::int16_t, 256> &table) {
    assert(format.FormatTag == WaveFormat::ALaw ||
           format.FormatTag == WaveFormat::MuLaw);

    const std::uint8_t *samples =
     reinterpret_cast<const std::uint8_t *>(data.data());
    if (format.NumChannels == 1) {
      convertMono(data, table, fact);
    } else if (format.NumChannels == 2) {
      convertStereo(data, table, fact);
    } else {
      throw Error("Invalid number of channels", ErrorCode::UNSUPPORTED_CODEC);
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

  ~LogarithmicDecoder() override = default;
};

ALawDecoderFactory::ALawDecoderFactory() = default;
ALawDecoderFactory::~ALawDecoderFactory() = default;

std::unique_ptr<WaveDecoder>
ALawDecoderFactory::createDecoder(const WaveFormat &format,
                                  const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format, data, aLawTable);
}

std::unique_ptr<WaveDecoder>
ALawDecoderFactory::createDecoder(const WaveFormat &format, std::uint32_t fact,
                                  const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format, fact, data, aLawTable);
}

std::unique_ptr<WaveDecoder>
ALawDecoderFactory::createDecoder(const WaveFormatEx &format,
                                  const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format.toWaveFormat(), data,
                                              aLawTable);
}

std::unique_ptr<WaveDecoder>
ALawDecoderFactory::createDecoder(const WaveFormatEx &format,
                                  std::uint32_t fact,
                                  const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format.toWaveFormat(), fact, data,
                                              aLawTable);
}

ALawDecoderFactory *ALawDecoderFactory::getInstance() {
  static ALawDecoderFactory factory;
  return &factory;
}

MuLawDecoderFactory::MuLawDecoderFactory() = default;
MuLawDecoderFactory::~MuLawDecoderFactory() = default;

std::unique_ptr<WaveDecoder>
MuLawDecoderFactory::createDecoder(const WaveFormat &format,
                                   const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format, data, muLawTable);
}

std::unique_ptr<WaveDecoder>
MuLawDecoderFactory::createDecoder(const WaveFormat &format, std::uint32_t fact,
                                   const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format, fact, data, muLawTable);
}

std::unique_ptr<WaveDecoder>
MuLawDecoderFactory::createDecoder(const WaveFormatEx &format,
                                   const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format.toWaveFormat(), data,
                                              muLawTable);
}

std::unique_ptr<WaveDecoder>
MuLawDecoderFactory::createDecoder(const WaveFormatEx &format,
                                   std::uint32_t fact,
                                   const std::vector<char> &data) {
  return std::make_unique<LogarithmicDecoder>(format.toWaveFormat(), fact, data,
                                              muLawTable);
}

MuLawDecoderFactory *MuLawDecoderFactory::getInstance() {
  static MuLawDecoderFactory factory;
  return &factory;
}