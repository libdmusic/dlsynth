#include "Wave.hpp"
#include "DecoderTable.hpp"
#include "Error.hpp"
#include "Uuid.hpp"
#include "Wavesample.hpp"
#include <cassert>
#include <vector>

using namespace DLSynth;

constexpr riffcpp::FourCC wave_id = {'w', 'a', 'v', 'e'};
constexpr riffcpp::FourCC WAVE_id = {'W', 'A', 'V', 'E'};
constexpr riffcpp::FourCC fmt_id = {'f', 'm', 't', ' '};
constexpr riffcpp::FourCC fact_id = {'f', 'a', 'c', 't'};
constexpr riffcpp::FourCC data_id = {'d', 'a', 't', 'a'};

constexpr riffcpp::FourCC guid_id = {'g', 'u', 'i', 'd'};
constexpr riffcpp::FourCC dlid_id = {'d', 'l', 'i', 'd'};
constexpr riffcpp::FourCC wsmp_id = {'w', 's', 'm', 'p'};

WaveDecoder::~WaveDecoder() = default;
WaveDecoderFactory::~WaveDecoderFactory() = default;

struct Wave::impl {
  std::vector<float> m_leftData, m_rightData;
  std::unique_ptr<Uuid> m_guid = nullptr;
  std::unique_ptr<Wavesample> m_wavesample = nullptr;
  int m_sampleRate;

  impl(riffcpp::Chunk &chunk) {
    bool fmt_found = false;
    std::vector<char> fmt;

    bool data_found = false;
    std::vector<char> data;

    bool fact_found = false;
    std::uint32_t fact;

    for (auto child : chunk) {
      if (child.id() == fmt_id) {
        if (fmt_found)
          throw Error("Duplicate fmt chunk", ErrorCode::INVALID_FILE);

        fmt.resize(child.size());
        child.read_data(fmt.data(), fmt.size());
        fmt_found = true;
      } else if (child.id() == fact_id) {
        if (fact_found)
          throw Error("Duplicate fact chunk", ErrorCode::INVALID_FILE);

        child.read_data(reinterpret_cast<char *>(&fact), sizeof(fact));
        fact_found = true;
      } else if (child.id() == data_id) {
        if (data_found)
          throw Error("Duplicate data chunk", ErrorCode::INVALID_FILE);

        data.resize(child.size());
        child.read_data(data.data(), data.size());
        data_found = true;
      } else if (child.id() == guid_id || child.id() == dlid_id) {
        if (m_guid != nullptr)
          throw Error("Duplicate GUID", ErrorCode::INVALID_FILE);

        m_guid = std::make_unique<Uuid>();
        child.read_data(reinterpret_cast<char *>(m_guid.get()),
                        sizeof(*m_guid));
      } else if (child.id() == wsmp_id) {
        if (m_wavesample != nullptr)
          throw Error("Duplicate Wavesample", ErrorCode::INVALID_FILE);

        m_wavesample = std::make_unique<Wavesample>(child);
      }
    }

    if (!(fmt_found && data_found)) {
      throw Error("Incomplete data", ErrorCode::INVALID_FILE);
    }

    std::unique_ptr<WaveDecoder> decoder;

    if (fmt.size() == sizeof(WaveFormat)) {
      WaveFormat *format = reinterpret_cast<WaveFormat *>(fmt.data());

      if (format->NumChannels == 0 || format->SamplesPerSec == 0 ||
          format->BlockAlign == 0) {
        throw Error("Invalid file", ErrorCode::INVALID_FILE);
      }

      m_sampleRate = format->SamplesPerSec;
      auto factory = DecoderTable::getInstance().getFactory(format->FormatTag);

      if (fact_found) {
        decoder = factory->createDecoder(*format, fact, data);
      } else {
        decoder = factory->createDecoder(*format, data);
      }
    } else if (fmt.size() > sizeof(WaveFormat)) {
      WaveFormatEx *format = reinterpret_cast<WaveFormatEx *>(fmt.data());

      if (format->NumChannels == 0 || format->SamplesPerSec == 0 ||
          format->BlockAlign == 0) {
        throw Error("Invalid file", ErrorCode::INVALID_FILE);
      }

      m_sampleRate = format->SamplesPerSec;
      auto factory = DecoderTable::getInstance().getFactory(format->FormatTag);

      if (fact_found) {
        decoder = factory->createDecoder(*format, fact, data);
      } else {
        decoder = factory->createDecoder(*format, data);
      }
    } else {
      throw Error("Invalid format chunk", ErrorCode::INVALID_FILE);
    }

    std::size_t numFrames = decoder->num_frames();
    m_leftData.resize(numFrames);
    m_rightData.resize(numFrames);
    decoder->decode(m_leftData.data(), m_rightData.data(), numFrames);
  }
};

Wave::Wave(riffcpp::Chunk &chunk) : m_pimpl(new impl(chunk)) {}
Wave::Wave(Wave &&wave) : m_pimpl(wave.m_pimpl) { wave.m_pimpl = nullptr; }

const std::vector<float> &Wave::leftData() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_leftData;
}

const std::vector<float> &Wave::rightData() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_rightData;
}

const Uuid *Wave::guid() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_guid.get();
};

const Wavesample *Wave::wavesample() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_wavesample.get();
};

int Wave::sampleRate() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_sampleRate;
}

Wave::~Wave() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}