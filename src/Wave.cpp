#include "Wave.hpp"
#include "CommonFourCCs.hpp"
#include "DecoderTable.hpp"
#include "Error.hpp"
#include "Info.hpp"
#include "Structs.hpp"
#include "Uuid.hpp"
#include "WaveDecoder.hpp"
#include "Wavesample.hpp"
#include <cassert>
#include <vector>

using namespace DLSynth;

WaveDecoder::~WaveDecoder() = default;
WaveDecoderFactory::~WaveDecoderFactory() = default;

struct Wave::impl {
  std::vector<float> m_leftData, m_rightData;
  std::unique_ptr<Uuid> m_guid = nullptr;
  std::unique_ptr<Wavesample> m_wavesample = nullptr;
  std::unique_ptr<Info> m_info = nullptr;
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

        fact = readChunk<std::uint32_t>(child);
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

        m_guid = std::make_unique<Uuid>(readChunk<Uuid>(child));
      } else if (child.id() == wsmp_id) {
        if (m_wavesample != nullptr)
          throw Error("Duplicate Wavesample", ErrorCode::INVALID_FILE);

        m_wavesample = std::make_unique<Wavesample>(child);
      } else if (child.id() == riffcpp::list_id) {
        if (child.type() == INFO_id) {
          if (m_info != nullptr) {
            throw Error("Duplicate INFO chunk", ErrorCode::INVALID_FILE);
          }

          m_info = std::make_unique<Info>(Info::read(child));
        }
      }
    }

    if (!(fmt_found && data_found)) {
      throw Error("Incomplete data", ErrorCode::INVALID_FILE);
    }

    std::unique_ptr<WaveDecoder> decoder;
    const auto &decoderTable = DecoderTable::getInstance();

    if (fmt.size() == sizeof(WaveFormat)) {
      WaveFormat format;
      StructLoader<WaveFormat>::readBuffer(fmt.data(), fmt.data() + fmt.size(),
                                           format);

      if (format.NumChannels == 0 || format.SamplesPerSec == 0 ||
          format.BlockAlign == 0) {
        throw Error("Invalid file", ErrorCode::INVALID_FILE);
      }

      m_sampleRate = format.SamplesPerSec;

      if (fact_found) {
        decoder = decoderTable.getDecoder(format, fact, data);
      } else {
        decoder = decoderTable.getDecoder(format, data);
      }
    } else if (fmt.size() > sizeof(WaveFormat)) {
      WaveFormatEx format;
      StructLoader<WaveFormatEx>::readBuffer(fmt.data(),
                                             fmt.data() + fmt.size(), format);

      if (format.NumChannels == 0 || format.SamplesPerSec == 0 ||
          format.BlockAlign == 0) {
        throw Error("Invalid file", ErrorCode::INVALID_FILE);
      }

      m_sampleRate = format.SamplesPerSec;

      if (fact_found) {
        decoder = decoderTable.getDecoder(format, fact, data);
      } else {
        decoder = decoderTable.getDecoder(format, data);
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

const Info *Wave::info() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_info.get();
}

Wave::~Wave() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}