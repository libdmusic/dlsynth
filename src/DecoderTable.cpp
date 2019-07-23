#include "DecoderTable.hpp"
#include "Decoders/Decoders.hpp"
#include "Error.hpp"

using namespace DLSynth;
using namespace DLSynth::Decoders;

DecoderTable &DecoderTable::getInstance() {
  static DecoderTable instance;
  return instance;
}

DecoderTable::DecoderTable() {
  m_decoders[WaveFormat::Pcm] = PcmDecoderFactory::getInstance();
  m_decoders[WaveFormat::Float] = FloatDecoderFactory::getInstance();
  m_decoders[WaveFormat::ALaw] = ALawDecoderFactory::getInstance();
  m_decoders[WaveFormat::MuLaw] = MuLawDecoderFactory::getInstance();
  m_decoders[WaveFormat::MsAdpcm] = MsAdpcmDecoderFactory::getInstance();
}

DecoderTable::~DecoderTable() = default;

WaveDecoderFactory *DecoderTable::getFactory(std::uint16_t formatTag) const {
  if (m_decoders.find(formatTag) == m_decoders.end()) {
    throw Error("Unsupported codec", ErrorCode::UNSUPPORTED_CODEC);
  } else {
    return m_decoders.at(formatTag);
  }
}

std::unique_ptr<WaveDecoder>
DecoderTable::getDecoder(const WaveFormat &format, std::uint32_t fact,
                         const std::vector<char> &data) const {
  return getFactory(format.FormatTag)->createDecoder(format, fact, data);
}

std::unique_ptr<WaveDecoder>
DecoderTable::getDecoder(const WaveFormatEx &format, std::uint32_t fact,
                         const std::vector<char> &data) const {
  return getFactory(format.FormatTag)->createDecoder(format, fact, data);
}

std::unique_ptr<WaveDecoder>
DecoderTable::getDecoder(const WaveFormat &format,
                         const std::vector<char> &data) const {
  return getFactory(format.FormatTag)->createDecoder(format, data);
}

std::unique_ptr<WaveDecoder>
DecoderTable::getDecoder(const WaveFormatEx &format,
                         const std::vector<char> &data) const {
  return getFactory(format.FormatTag)->createDecoder(format, data);
}