#include "Instrument.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "StructUtils.hpp"
#include "Uuid.hpp"
#include <cassert>
#include <memory>

using namespace DLSynth;

struct insh {
  std::uint32_t cRegions;
  std::uint32_t ulBank;
  std::uint32_t ulInstrument;
};

struct Instrument::impl {
  std::unique_ptr<Uuid> m_dlid;
  std::uint32_t m_midiBank;
  std::uint32_t m_midiInstrument;
  std::vector<ConnectionBlock> m_blocks;
  std::vector<Region> m_regions;
  bool m_isDrum;
  void load_regions(riffcpp::Chunk &chunk, const ExpressionParser &exprParser) {
    for (auto child : chunk) {
      if (child.id() == riffcpp::list_id) {
        if (child.type() == rgn_id || child.type() == rgn2_id) {
          m_regions.emplace_back(child, exprParser);
        }
      }
    }
  }
  void load_articulators(riffcpp::Chunk &chunk,
                         const ExpressionParser &exprParser) {
    try {
      Articulator art(chunk, exprParser);
      m_blocks.insert(m_blocks.end(), art.connectionBlocks().begin(),
                      art.connectionBlocks().end());
    } catch (const Error &e) {
      if (e.code() != ErrorCode::CONDITION_FAILED) {
        throw e;
      }
    }
  }

  impl(riffcpp::Chunk &chunk, const ExpressionParser &exprParser) {
    assert(chunk.id() == riffcpp::list_id && chunk.type() == ins_id);

    bool insh_found = false;
    bool lrgn_found = false;

    for (auto child : chunk) {
      if (child.id() == dlid_id) {
        m_dlid = std::make_unique<Uuid>();
        child.read_data(reinterpret_cast<char *>(m_dlid.get()),
                        sizeof(*m_dlid));
      } else if (child.id() == insh_id) {
        if (insh_found) {
          throw Error("Duplicate instrument header", ErrorCode::INVALID_FILE);
        }

        insh header = readStruct<insh>(child);

        m_midiBank = header.ulBank & ~(1 << 31);
        m_midiInstrument = header.ulInstrument;
        m_isDrum = (header.ulBank & (1 << 31)) != 0;

        insh_found = true;
      } else if (child.id() == riffcpp::list_id) {
        if (child.type() == lrgn_id) {
          if (lrgn_found) {
            throw Error("Duplicate region list", ErrorCode::INVALID_FILE);
          }

          load_regions(child, exprParser);

          lrgn_found = true;
        } else if (child.type() == lart_id || child.type() == lar2_id) {
          load_articulators(child, exprParser);
        }
      }
    }

    if (!insh_found) {
      throw Error("Incomplete instrument", ErrorCode::INVALID_FILE);
    }
  }
};

Instrument::Instrument(riffcpp::Chunk &chunk,
                       const ExpressionParser &exprParser)
  : m_pimpl(new impl(chunk, exprParser)) {}
Instrument::Instrument(Instrument &&instr) : m_pimpl(instr.m_pimpl) {
  instr.m_pimpl = nullptr;
}

Instrument::~Instrument() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}

const Uuid *Instrument::dlid() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_dlid.get();
}

std::uint32_t Instrument::midiBank() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_midiBank;
}

std::uint32_t Instrument::midiInstrument() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_midiInstrument;
}

const std::vector<Region> &Instrument::regions() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_regions;
}

const std::vector<ConnectionBlock> &Instrument::connectionBlocks() const {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_blocks;
}