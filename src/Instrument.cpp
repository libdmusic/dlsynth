#include "Instrument.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "Info.hpp"
#include "Structs.hpp"
#include "Uuid.hpp"
#include <cassert>
#include <memory>

using namespace DLSynth;

struct Instrument::impl {
  std::uint32_t m_midiBank;
  std::uint32_t m_midiInstrument;
  std::vector<ConnectionBlock> m_blocks;
  std::vector<Region> m_regions;
  std::unique_ptr<Uuid> m_dlid;
  std::unique_ptr<Info> m_info;
  bool m_isDrum;

  impl(std::uint32_t midiBank, std::uint32_t midiInstrument,
       const std::vector<ConnectionBlock> &blocks,
       const std::vector<Region> &regions, std::unique_ptr<Uuid> dlid,
       std::unique_ptr<Info> info) noexcept
    : m_midiBank(midiBank)
    , m_midiInstrument(midiInstrument)
    , m_blocks(blocks)
    , m_regions(regions)
    , m_dlid(std::move(dlid))
    , m_info(std::move(info)) {}

  impl(const impl &i) noexcept
    : m_midiBank(i.m_midiBank)
    , m_midiInstrument(i.m_midiInstrument)
    , m_blocks(i.m_blocks)
    , m_regions(i.m_regions)
    , m_dlid(i.m_dlid ? std::make_unique<Uuid>(*i.m_dlid) : nullptr)
    , m_info(i.m_info ? std::make_unique<Info>(*i.m_info) : nullptr) {}
};

Instrument::Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
                       bool isDrumInstrument,
                       const std::vector<ConnectionBlock> &connectionBlocks,
                       const std::vector<Region> &regions) noexcept
  : m_pimpl(new impl(midiBank, midiInstrument, connectionBlocks, regions,
                     nullptr, nullptr)) {}

Instrument::Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
                       bool isDrumInstrument,
                       const std::vector<ConnectionBlock> &connectionBlocks,
                       const std::vector<Region> &regions,
                       const Uuid &dlid) noexcept
  : m_pimpl(new impl(midiBank, midiInstrument, connectionBlocks, regions,
                     std::make_unique<Uuid>(dlid), nullptr)) {}
Instrument::Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
                       bool isDrumInstrument,
                       const std::vector<ConnectionBlock> &connectionBlocks,
                       const std::vector<Region> &regions,
                       const Info &info) noexcept
  : m_pimpl(new impl(midiBank, midiInstrument, connectionBlocks, regions,
                     nullptr, std::make_unique<Info>(info))) {}
Instrument::Instrument(std::uint32_t midiBank, std::uint32_t midiInstrument,
                       bool isDrumInstrument,
                       const std::vector<ConnectionBlock> &connectionBlocks,
                       const std::vector<Region> &regions, const Uuid &dlid,
                       const Info &info) noexcept
  : m_pimpl(new impl(midiBank, midiInstrument, connectionBlocks, regions,
                     std::make_unique<Uuid>(dlid),
                     std::make_unique<Info>(info))) {}

Instrument::Instrument(const Instrument &instr) noexcept
  : m_pimpl(new impl(*instr.m_pimpl)) {}

Instrument::Instrument(Instrument &&instr) noexcept : m_pimpl(instr.m_pimpl) {
  instr.m_pimpl = nullptr;
}

const Instrument &Instrument::operator=(const Instrument &instr) const
 noexcept {
  delete m_pimpl;
  m_pimpl = new impl(*instr.m_pimpl);
  return *this;
}

Instrument::~Instrument() {
  if (m_pimpl != nullptr) {
    delete m_pimpl;
  }
}

const Uuid *Instrument::dlid() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_dlid.get();
}

std::uint32_t Instrument::midiBank() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_midiBank;
}

std::uint32_t Instrument::midiInstrument() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_midiInstrument;
}

const std::vector<Region> &Instrument::regions() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_regions;
}

const std::vector<ConnectionBlock> &Instrument::connectionBlocks() const
 noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_blocks;
}

const Info *Instrument::info() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_info.get();
}

bool Instrument::isDrumInstrument() const noexcept {
  assert(m_pimpl != nullptr);
  return m_pimpl->m_isDrum;
}

static void load_regions(riffcpp::Chunk &chunk,
                         const ExpressionParser &exprParser,
                         std::vector<Region> &m_regions) {
  for (auto child : chunk) {
    if (child.id() == riffcpp::list_id) {
      if (child.type() == rgn_id || child.type() == rgn2_id) {
        m_regions.push_back(Region::readChunk(child, exprParser));
      }
    }
  }
}
static void load_articulators(riffcpp::Chunk &chunk,
                              const ExpressionParser &exprParser,
                              std::vector<ConnectionBlock> &m_blocks) {
  try {
    Articulator art = Articulator::readChunk(chunk, exprParser);
    m_blocks.insert(m_blocks.end(), art.connectionBlocks().begin(),
                    art.connectionBlocks().end());
  } catch (const Error &e) {
    if (e.code() != ErrorCode::CONDITION_FAILED) {
      throw e;
    }
  }
}

Instrument Instrument::readChunk(riffcpp::Chunk &chunk,
                                 const ExpressionParser &exprParser) {
  bool insh_found = false;
  bool lrgn_found = false;
  bool dlid_found = false;
  bool info_found = false;
  insh header;

  Uuid dlid;
  Info info("", "", "", "", "");

  std::vector<Region> regions;
  std::vector<ConnectionBlock> blocks;

  for (auto child : chunk) {
    if (child.id() == dlid_id) {
      if (dlid_found) {
        throw Error("Duplicate DLID", ErrorCode::INVALID_FILE);
      }
      dlid = ::DLSynth::readChunk<Uuid>(child);
      dlid_found = true;
    } else if (child.id() == insh_id) {
      if (insh_found) {
        throw Error("Duplicate instrument header", ErrorCode::INVALID_FILE);
      }

      header = ::DLSynth::readChunk<insh>(child);

      insh_found = true;
    } else if (child.id() == riffcpp::list_id) {
      if (child.type() == lrgn_id) {
        if (lrgn_found) {
          throw Error("Duplicate region list", ErrorCode::INVALID_FILE);
        }

        load_regions(child, exprParser, regions);

        lrgn_found = true;
      } else if (child.type() == lart_id || child.type() == lar2_id) {
        load_articulators(child, exprParser, blocks);
      } else if (child.type() == INFO_id) {
        if (info_found) {
          throw Error("Duplicate INFO chunk", ErrorCode::INVALID_FILE);
        }
        info = Info::readChunk(child);
        info_found = true;
      }
    }
  }

  if (!insh_found) {
    throw Error("Incomplete instrument", ErrorCode::INVALID_FILE);
  }

  std::uint32_t midiBank = header.ulBank & ~(1 << 31);
  std::uint32_t midiInstrument = header.ulInstrument;
  bool isDrum = (header.ulBank & (1 << 31)) != 0;

  if (dlid_found && info_found) {
    return Instrument(midiBank, midiInstrument, isDrum, blocks, regions, dlid,
                      info);
  } else if (dlid_found) {
    return Instrument(midiBank, midiInstrument, isDrum, blocks, regions, dlid);
  } else if (info_found) {
    return Instrument(midiBank, midiInstrument, isDrum, blocks, regions, info);
  } else {
    return Instrument(midiBank, midiInstrument, isDrum, blocks, regions);
  }
}