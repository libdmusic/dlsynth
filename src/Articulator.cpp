#include "Articulator.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include <set>

using namespace DLSynth;

ConnectionBlock::ConnectionBlock(Source src, Source ctrl, Destination dest,
                                 std::int32_t scale, const Transform &srcTrans,
                                 const Transform &ctrlTrans)
  : m_source(src)
  , m_control(ctrl)
  , m_destination(dest)
  , m_scale(scale)
  , m_sourceTransform(srcTrans)
  , m_controlTransform(ctrlTrans) {}

Source ConnectionBlock::source() const { return m_source; }
Source ConnectionBlock::control() const { return m_control; }
Destination ConnectionBlock::destination() const { return m_destination; }
std::int32_t ConnectionBlock::scale() const { return m_scale; }
const Transform &ConnectionBlock::sourceTransform() const {
  return m_sourceTransform;
}
const Transform &ConnectionBlock::controlTransform() const {
  return m_controlTransform;
}
Articulator::Articulator(riffcpp::Chunk &chunk,
                         const ExpressionParser &exprParser) {
  for (auto child : chunk) {
    if (child.id() == cdl_id) {
      if (!exprParser.execute(child)) {
        throw Error("Condition failed", ErrorCode::CONDITION_FAILED);
      }
    } else if (child.id() == art1_id) {
      load_art1(child);
    } else if (child.id() == art2_id) {
      load_art2(child);
    }
  }
}

const std::vector<ConnectionBlock> &Articulator::connectionBlocks() const {
  return m_blocks;
}

struct cblock {
  Source usSource;
  Source usControl;
  Destination usDestination;
  std::uint16_t usTransform;
  std::int32_t lScale;
};

struct art {
  std::uint32_t cbSize;
  std::uint32_t cConnectionBlocks;
  cblock connectionBlocks[0];
};

void Articulator::load_art2(riffcpp::Chunk &chunk) {
  std::vector<char> data(chunk.size());
  chunk.read_data(data.data(), data.size());
  art *articulator = reinterpret_cast<art *>(data.data());
  for (std::uint32_t i = 0; i < articulator->cConnectionBlocks; i++) {
    cblock connectionBlock = articulator->connectionBlocks[i];
    TransformType ctrlTransType =
     (TransformType)((connectionBlock.usTransform & 0x00F0) >> 4);
    bool ctrlBipolar = connectionBlock.usTransform & (1 << 8);
    bool ctrlInvert = connectionBlock.usTransform & (1 << 9);
    TransformType srcTransType =
     (TransformType)((connectionBlock.usTransform & 0x0F00) >> 8);
    bool srcBipolar = connectionBlock.usTransform & (1 << 14);
    bool srcInvert = connectionBlock.usTransform & (1 << 15);

    m_blocks.emplace_back(connectionBlock.usSource, connectionBlock.usControl,
                          connectionBlock.usDestination, connectionBlock.lScale,
                          Transform(srcInvert, srcBipolar, srcTransType),
                          Transform(ctrlInvert, ctrlBipolar, ctrlTransType));
  }
}

void Articulator::load_art1(riffcpp::Chunk &chunk) {
  static std::set<Source> bipolarSources{Source::LFO,        Source::CC10,
                                         Source::PitchWheel, Source::RPN1,
                                         Source::RPN2,       Source::Vibrato};
  static std::set<Source> invertedSources{Source::KeyOnVelocity, Source::CC7,
                                          Source::CC11};

  std::vector<char> data(chunk.size());
  chunk.read_data(data.data(), data.size());
  art *articulator = reinterpret_cast<art *>(data.data());

  for (std::uint32_t i = 0; i < articulator->cConnectionBlocks; i++) {
    cblock connectionBlock = articulator->connectionBlocks[i];
    TransformType transType = (TransformType)connectionBlock.usTransform;

    Source src = connectionBlock.usSource;
    Source ctrl = connectionBlock.usControl;
    Destination dst = connectionBlock.usDestination;

    m_blocks.emplace_back(
     src, ctrl, dst, connectionBlock.lScale,
     Transform(invertedSources.find(src) != invertedSources.end(),
               bipolarSources.find(src) != bipolarSources.end(), transType),
     Transform(false, false, transType));
  }
}