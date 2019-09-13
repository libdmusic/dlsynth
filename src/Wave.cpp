#include "Wave.hpp"
#include "CommonFourCCs.hpp"
#include "DecoderTable.hpp"
#include "Error.hpp"
#include "Info.hpp"
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

  impl(const std::vector<float> &lData, const std::vector<float> &rData,
       int sampleRate, std::unique_ptr<Wavesample> wavesample,
       std::unique_ptr<Uuid> guid, std::unique_ptr<Info> info)
    : m_leftData(lData)
    , m_rightData(rData)
    , m_sampleRate(sampleRate)
    , m_wavesample(std::move(wavesample))
    , m_guid(std::move(guid))
    , m_info(std::move(info)) {}

  impl(const impl &i)
    : m_leftData(i.m_leftData)
    , m_rightData(i.m_rightData)
    , m_sampleRate(i.m_sampleRate)
    , m_wavesample(
       i.m_wavesample ? std::make_unique<Wavesample>(*i.m_wavesample) : nullptr)
    , m_guid(i.m_guid ? std::make_unique<Uuid>(*i.m_guid) : nullptr)
    , m_info(i.m_info ? std::make_unique<Info>(*i.m_info) : nullptr) {}
};

Wave::Wave(const std::vector<float> &leftData,
           const std::vector<float> &rightData, int sampleRate,
           const Wavesample *wavesample, const Info *info,
           const Uuid *guid) noexcept
  : m_pimpl(
     new impl(leftData, rightData, sampleRate,
              wavesample ? std::make_unique<Wavesample>(*wavesample) : nullptr,
              guid ? std::make_unique<Uuid>(*guid) : nullptr,
              info ? std::make_unique<Info>(*info) : nullptr)) {}
Wave::Wave(Wave &&wave) noexcept : m_pimpl(wave.m_pimpl) {
  wave.m_pimpl = nullptr;
}

Wave::Wave(const Wave &wave) noexcept : m_pimpl(new impl(*wave.m_pimpl)) {}

Wave &Wave::operator=(const Wave &wave) noexcept {
  delete m_pimpl;
  m_pimpl = new impl(*wave.m_pimpl);
  return *this;
}

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

Wave Wave::readChunk(riffcpp::Chunk &chunk) {
  bool fmt_found = false;
  std::vector<char> fmt;

  bool data_found = false;
  std::vector<char> data;

  bool fact_found = false;
  std::uint32_t fact;

  bool guid_found = false;
  bool info_found = false;
  bool wsmp_found = false;

  Uuid guid;
  Info info("", "", "", "", "");
  Wavesample wavesample(0, 0, 0);

  int sampleRate;

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

      fact = ::readChunk<std::uint32_t>(child);
      fact_found = true;
    } else if (child.id() == data_id) {
      if (data_found)
        throw Error("Duplicate data chunk", ErrorCode::INVALID_FILE);

      data.resize(child.size());
      child.read_data(data.data(), data.size());
      data_found = true;
    } else if (child.id() == guid_id || child.id() == dlid_id) {
      if (guid_found)
        throw Error("Duplicate GUID", ErrorCode::INVALID_FILE);

      guid = ::readChunk<Uuid>(child);
      guid_found = true;
    } else if (child.id() == wsmp_id) {
      if (wsmp_found)
        throw Error("Duplicate Wavesample", ErrorCode::INVALID_FILE);

      wavesample = Wavesample::readChunk(child);
      wsmp_found = true;
    } else if (child.id() == riffcpp::list_id) {
      if (child.type() == INFO_id) {
        if (info_found) {
          throw Error("Duplicate INFO chunk", ErrorCode::INVALID_FILE);
        }

        info = Info::readChunk(child);
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

    sampleRate = format.SamplesPerSec;

    if (fact_found) {
      decoder = decoderTable.getDecoder(format, fact, data);
    } else {
      decoder = decoderTable.getDecoder(format, data);
    }
  } else if (fmt.size() > sizeof(WaveFormat)) {
    WaveFormatEx format;
    StructLoader<WaveFormatEx>::readBuffer(fmt.data(), fmt.data() + fmt.size(),
                                           format);

    if (format.NumChannels == 0 || format.SamplesPerSec == 0 ||
        format.BlockAlign == 0) {
      throw Error("Invalid file", ErrorCode::INVALID_FILE);
    }

    sampleRate = format.SamplesPerSec;

    if (fact_found) {
      decoder = decoderTable.getDecoder(format, fact, data);
    } else {
      decoder = decoderTable.getDecoder(format, data);
    }
  } else {
    throw Error("Invalid format chunk", ErrorCode::INVALID_FILE);
  }

  std::vector<float> leftData, rightData;
  std::size_t numFrames = decoder->num_frames();
  leftData.resize(numFrames);
  rightData.resize(numFrames);
  decoder->decode(leftData.data(), rightData.data(), numFrames);

  return Wave(leftData, rightData, sampleRate,
              wsmp_found ? &wavesample : nullptr, info_found ? &info : nullptr,
              guid_found ? &guid : nullptr);
}