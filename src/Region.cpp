#include "Region.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "Structs.hpp"

using namespace DLSynth;

Region::Region(const Range &keyRange, const Range &velocityRange,
               const std::vector<ConnectionBlock> &connectionBlocks,
               std::uint32_t waveIndex, bool selfNonExclusive) noexcept
  : m_keyRange(keyRange)
  , m_velRange(velocityRange)
  , m_blocks(connectionBlocks)
  , m_waveIndex(waveIndex)
  , m_selfNonExclusive(selfNonExclusive) {}

Region::Region(const Range &keyRange, const Range &velocityRange,
               const std::vector<ConnectionBlock> &connectionBlocks,
               std::uint32_t waveIndex, bool selfNonExclusive,
               const Wavesample &wavesample) noexcept
  : m_keyRange(keyRange)
  , m_velRange(velocityRange)
  , m_blocks(connectionBlocks)
  , m_waveIndex(waveIndex)
  , m_selfNonExclusive(selfNonExclusive)
  , m_wavesample(std::make_unique<Wavesample>(wavesample)) {}

Region::Region(Region &&region) noexcept
  : m_keyRange(region.m_keyRange)
  , m_velRange(region.m_velRange)
  , m_blocks(region.m_blocks)
  , m_waveIndex(region.m_waveIndex)
  , m_selfNonExclusive(region.m_selfNonExclusive)
  , m_wavesample(std::move(region.m_wavesample)) {}

Region::Region(const Region &region) noexcept
  : m_keyRange(region.m_keyRange)
  , m_velRange(region.m_velRange)
  , m_blocks(region.m_blocks)
  , m_waveIndex(region.m_waveIndex)
  , m_selfNonExclusive(region.m_selfNonExclusive)
  , m_wavesample(region.m_wavesample
                  ? std::make_unique<Wavesample>(*region.m_wavesample)
                  : nullptr) {}

Region &Region::operator=(const Region &region) noexcept {
  m_keyRange = region.m_keyRange;
  m_velRange = region.m_velRange;
  m_blocks = region.m_blocks;
  m_waveIndex = region.m_waveIndex;
  m_selfNonExclusive = region.m_selfNonExclusive;
  m_wavesample = region.m_wavesample
                  ? std::make_unique<Wavesample>(*region.m_wavesample)
                  : nullptr;

  return *this;
}

const Wavesample *Region::wavesample() const noexcept {
  return m_wavesample.get();
}

const Range &Region::keyRange() const noexcept { return m_keyRange; }

const Range &Region::velocityRange() const noexcept { return m_velRange; }

std::uint16_t Region::keyGroup() const noexcept { return m_keyGroup; }

const std::vector<ConnectionBlock> &Region::connectionBlocks() const noexcept {
  return m_blocks;
}

std::uint32_t Region::waveIndex() const noexcept { return m_waveIndex; }

bool Region::selfNonExclusive() const noexcept { return m_selfNonExclusive; }

Region Region::readChunk(riffcpp::Chunk &chunk,
                         const ExpressionParser &exprParser) {
  bool rgnh_found = false;
  bool wvlnk_found = false;
  bool wsmp_found = false;

  rgnh header;
  Wavesample wavesample(0, 0, 0);
  wlnk wavelink;

  std::vector<ConnectionBlock> m_blocks;

  for (auto child : chunk) {
    if (child.id() == cdl_id) {
      if (!exprParser.execute(child)) {
        throw Error("Condition failed", ErrorCode::CONDITION_FAILED);
      }
    } else if (child.id() == rgnh_id) {
      if (rgnh_found) {
        throw Error("Duplicate region header", ErrorCode::INVALID_FILE);
      }
      rgnh_found = true;
      header = ::DLSynth::readChunk<rgnh>(child);
    } else if (child.id() == wsmp_id) {
      if (wsmp_found) {
        throw Error("Duplicate wavesample", ErrorCode::INVALID_FILE);
      }
      wavesample = Wavesample::readChunk(child);
      wsmp_found = true;
    } else if (child.id() == wlnk_id) {
      if (wvlnk_found) {
        throw Error("Duplicate wavelink", ErrorCode::INVALID_FILE);
      }
      wavelink = ::DLSynth::readChunk<wlnk>(child);
      wvlnk_found = true;
    } else if (child.id() == riffcpp::list_id) {
      if (child.type() == lart_id || child.type() == lar2_id) {
        try {
          Articulator art = Articulator::readChunk(child, exprParser);
          m_blocks.insert(m_blocks.end(), art.connectionBlocks().begin(),
                          art.connectionBlocks().end());
        } catch (const Error &e) {
          if (e.code() != ErrorCode::CONDITION_FAILED) {
            throw e;
          }
        }
      }
    }
  }

  if (!(rgnh_found && wvlnk_found)) {
    throw Error("Incomplete region", ErrorCode::INVALID_FILE);
  }

  if (wsmp_found) {
    return Region(header.RangeKey, header.RangeVelocity, m_blocks,
                  wavelink.ulTableIndex, header.fusOptions & 0x0001,
                  wavesample);
  } else {
    return Region(header.RangeKey, header.RangeVelocity, m_blocks,
                  wavelink.ulTableIndex, header.fusOptions & 0x0001);
  }
}