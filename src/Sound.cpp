#include "Sound.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "Uuid.hpp"
#include <cassert>

using namespace DLSynth;

struct Sound::impl {
  ExpressionParser m_exprParser;
  std::unique_ptr<Uuid> m_dlid;
  std::vector<Wave> m_wavepool;
  std::vector<Instrument> m_instruments;

  void load_wavepool(riffcpp::Chunk &chunk) {
    assert(chunk.id() == riffcpp::list_id && chunk.type() == wvpl_id);

    for (auto wave : chunk) {
      if (wave.id() == riffcpp::list_id && wave.type() == wave_id) {
        m_wavepool.emplace_back(wave);
      }
    }
  }

  void load_instruments(riffcpp::Chunk &chunk) {
    assert(chunk.id() == riffcpp::list_id && chunk.type() == lins_id);

    for (auto ins : chunk) {
      if (ins.id() == riffcpp::list_id && ins.type() == ins_id) {
        try {
          m_instruments.emplace_back(ins, m_exprParser);
        } catch (const Error &e) {
          if (e.code() != ErrorCode::CONDITION_FAILED) {
            throw e;
          }
        }
      }
    }
  }
  impl(riffcpp::Chunk &chunk, std::uint32_t sampleRate)
    : m_exprParser(sampleRate, UINT32_MAX) {
    bool lins_found = false;
    bool wvpl_found = false;

    for (auto child : chunk) {
      if (child.id() == cdl_id) {
        if (!m_exprParser.execute(child)) {
          throw Error("Condition failed", ErrorCode::CONDITION_FAILED);
        }
      } else if (child.id() == dlid_id) {
        m_dlid = std::make_unique<Uuid>();
        child.read_data(reinterpret_cast<char *>(m_dlid.get()),
                        sizeof(*m_dlid));
      } else if (child.id() == riffcpp::list_id) {
        if (child.type() == wvpl_id) {
          if (wvpl_found)
            throw Error("Duplicate wavepool", ErrorCode::INVALID_FILE);

          load_wavepool(child);
          wvpl_found = true;
        } else if (child.type() == lins_id) {
          if (lins_found)
            throw Error("Duplicate instrument list", ErrorCode::INVALID_FILE);

          load_instruments(child);
          lins_found = true;
        }
      }
    }

    if (!(lins_found && wvpl_found)) {
      throw Error("Incomplete file", ErrorCode::INVALID_FILE);
    }
  }

  ~impl() = default;
};
Sound::Sound(riffcpp::Chunk &chunk, std::uint32_t sampleRate)
  : m_pimpl(new impl(chunk, sampleRate)) {}

Sound::Sound(Sound &&snd) : m_pimpl(snd.m_pimpl) { snd.m_pimpl = nullptr; }

Sound::~Sound() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}

const Uuid *Sound::dlid() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_dlid.get();
}

const std::vector<Instrument> &Sound::instruments() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_instruments;
}

const std::vector<Wave> &Sound::wavepool() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_wavepool;
}