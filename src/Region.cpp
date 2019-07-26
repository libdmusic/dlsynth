#include "Region.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"

using namespace DLSynth;

Region::Region(riffcpp::Chunk &chunk, const ExpressionParser &exprParser) {
  bool rgnh_found = false;
  bool wvlnk_found = false;

  for (auto child : chunk) {
    if (child.id() == cdl_id) {
      if (!exprParser.execute(child)) {
        throw Error("Condition failed", ErrorCode::CONDITION_FAILED);
      }
    } else if (child.id() == rgnh_id) {
      if (rgnh_found) {
        throw Error("Duplicate region header", ErrorCode::INVALID_FILE);
      }
      load_header(child);
      rgnh_found = true;
    } else if (child.id() == wsmp_id) {
      if (m_wavesample != nullptr) {
        throw Error("Duplicate wavesample", ErrorCode::INVALID_FILE);
      }
      m_wavesample = std::make_unique<Wavesample>(child);
    } else if (child.id() == wlnk_id) {
      if (wvlnk_found) {
        throw Error("Duplicate wavelink", ErrorCode::INVALID_FILE);
      }
      load_wavelink(child);
      wvlnk_found = true;
    } else if (child.id() == riffcpp::list_id) {
      if (child.type() == lart_id || child.type() == lar2_id) {
        try {
          Articulator art(child, exprParser);
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
}

struct rgnh {
  Range RangeKey;
  Range RangeVelocity;
  std::uint16_t fusOptions;
  std::uint16_t usKeyGroup;
};

void Region::load_header(riffcpp::Chunk &chunk) {
  rgnh header;
  chunk.read_data(reinterpret_cast<char *>(&header), sizeof(header));
  m_keyGroup = header.usKeyGroup;
  m_keyRange = header.RangeKey;
  m_velRange = header.RangeVelocity;
  m_selfNonExclusive = (header.fusOptions & 0x0001);
}

struct wlnk {
  std::uint16_t fusOptions;
  std::uint16_t usPhaseGroup;
  std::uint32_t ulChannel;
  std::uint32_t ulTableIndex;
};
void Region::load_wavelink(riffcpp::Chunk &chunk) {
  wlnk wavelink;
  chunk.read_data(reinterpret_cast<char *>(&wavelink), sizeof(wavelink));
  m_waveIndex = wavelink.ulTableIndex;
}

const Range &Region::keyRange() const { return m_keyRange; }
const Range &Region::velocityRange() const { return m_velRange; }
std::uint16_t Region::keyGroup() const { return m_keyGroup; }
const std::vector<ConnectionBlock> &Region::connectionBlocks() const {
  return m_blocks;
}
std::uint32_t Region::waveIndex() const { return m_waveIndex; }
const Wavesample *Region::wavesample() const { return m_wavesample.get(); }

bool Region::selfNonExclusive() const { return m_selfNonExclusive; }