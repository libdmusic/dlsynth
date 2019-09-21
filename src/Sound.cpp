#include "Sound.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "Info.hpp"
#include "Uuid.hpp"
#include <cassert>

using namespace DLSynth;

struct Sound::impl {
  std::unique_ptr<Uuid> m_dlid;
  std::unique_ptr<Info> m_info;
  std::vector<Wave> m_wavepool;
  std::vector<Instrument> m_instruments;

  impl(const std::vector<Wave> &wavepool,
       const std::vector<Instrument> &instruments, std::unique_ptr<Uuid> dlid,
       std::unique_ptr<Info> info)
    : m_wavepool(wavepool)
    , m_instruments(instruments)
    , m_dlid(std::move(dlid))
    , m_info(std::move(info)) {}

  impl(const impl &i)
    : m_wavepool(i.m_wavepool)
    , m_instruments(i.m_instruments)
    , m_dlid(i.m_dlid ? std::make_unique<Uuid>(*i.m_dlid) : nullptr)
    , m_info(i.m_info ? std::make_unique<Info>(*i.m_info) : nullptr) {}
};

Sound::Sound(const std::vector<Instrument> &instruments,
             const std::vector<Wave> &wavepool) noexcept
  : m_pimpl(new impl(wavepool, instruments, nullptr, nullptr)) {}
Sound::Sound(const std::vector<Instrument> &instruments,
             const std::vector<Wave> &wavepool, const Uuid &dlid) noexcept
  : m_pimpl(
     new impl(wavepool, instruments, std::make_unique<Uuid>(dlid), nullptr)) {}
Sound::Sound(const std::vector<Instrument> &instruments,
             const std::vector<Wave> &wavepool, const Info &info) noexcept
  : m_pimpl(
     new impl(wavepool, instruments, nullptr, std::make_unique<Info>(info))) {}
Sound::Sound(const std::vector<Instrument> &instruments,
             const std::vector<Wave> &wavepool, const Uuid &dlid,
             const Info &info) noexcept
  : m_pimpl(new impl(wavepool, instruments, std::make_unique<Uuid>(dlid),
                     std::make_unique<Info>(info))) {}

Sound::Sound(Sound &&snd) noexcept : m_pimpl(snd.m_pimpl) {
  snd.m_pimpl = nullptr;
}

Sound::Sound(const Sound &snd) noexcept : m_pimpl(new impl(*snd.m_pimpl)) {}

const Sound &Sound::operator=(const Sound &snd) const noexcept {
  delete m_pimpl;
  m_pimpl = new impl(*snd.m_pimpl);
  return *this;
}

Sound::~Sound() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}

const Uuid *Sound::dlid() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_dlid.get();
}

const std::vector<Instrument> &Sound::instruments() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_instruments;
}

const std::vector<Wave> &Sound::wavepool() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_wavepool;
}

const Info *Sound::info() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_info.get();
}

static void load_wavepool(riffcpp::Chunk &chunk,
                          std::vector<Wave> &m_wavepool) {
  assert(chunk.id() == riffcpp::list_id && chunk.type() == wvpl_id);

  for (auto wave : chunk) {
    if (wave.id() == riffcpp::list_id && wave.type() == wave_id) {
      m_wavepool.push_back(Wave::readChunk(wave));
    }
  }
}

static void load_instruments(riffcpp::Chunk &chunk,
                             std::vector<Instrument> &m_instruments,
                             const ExpressionParser &m_exprParser) {
  assert(chunk.id() == riffcpp::list_id && chunk.type() == lins_id);

  for (auto ins : chunk) {
    if (ins.id() == riffcpp::list_id && ins.type() == ins_id) {
      try {
        m_instruments.push_back(Instrument::readChunk(ins, m_exprParser));
      } catch (const Error &e) {
        if (e.code() != ErrorCode::CONDITION_FAILED) {
          throw e;
        }
      }
    }
  }
}

Sound Sound::readChunk(riffcpp::Chunk &chunk, std::uint32_t sampleRate) {
  ExpressionParser exprParser(sampleRate,
                              std::numeric_limits<std::uint32_t>::max());

  bool lins_found = false;
  bool wvpl_found = false;
  bool info_found = false;
  bool dlid_found = false;

  Info info("", "", "", "", "");
  Uuid dlid;

  std::vector<Wave> wavepool;
  std::vector<Instrument> instruments;

  for (auto child : chunk) {
    if (child.id() == cdl_id) {
      if (!exprParser.execute(child)) {
        throw Error("Condition failed", ErrorCode::CONDITION_FAILED);
      }
    } else if (child.id() == dlid_id) {
      if (dlid_found) {
        throw Error("Duplicate DLID", ErrorCode::INVALID_FILE);
      }

      dlid = ::readChunk<Uuid>(child);
    } else if (child.id() == riffcpp::list_id) {
      if (child.type() == wvpl_id) {
        if (wvpl_found)
          throw Error("Duplicate wavepool", ErrorCode::INVALID_FILE);

        load_wavepool(child, wavepool);
        wvpl_found = true;
      } else if (child.type() == lins_id) {
        if (lins_found)
          throw Error("Duplicate instrument list", ErrorCode::INVALID_FILE);

        load_instruments(child, instruments, exprParser);
        lins_found = true;
      } else if (child.type() == INFO_id) {
        if (info_found) {
          throw Error("Duplicate INFO chunk", ErrorCode::INVALID_FILE);
        }

        info = Info::readChunk(child);
      }
    }
  }

  if (!(lins_found && wvpl_found)) {
    throw Error("Incomplete file", ErrorCode::INVALID_FILE);
  }

  if (dlid_found && info_found) {
    return Sound(instruments, wavepool, dlid, info);
  } else if (dlid_found) {
    return Sound(instruments, wavepool, dlid);
  } else if (info_found) {
    return Sound(instruments, wavepool, info);
  } else {
    return Sound(instruments, wavepool);
  }
}