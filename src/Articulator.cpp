#include "Articulator.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include <set>

using namespace DLSynth;
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
     static_cast<TransformType>((connectionBlock.usTransform & 0x00F0) >> 4);
    bool ctrlBipolar = connectionBlock.usTransform & (1 << 8);
    bool ctrlInvert = connectionBlock.usTransform & (1 << 9);
    TransformType srcTransType =
     static_cast<TransformType>((connectionBlock.usTransform & 0x0F00) >> 8);
    bool srcBipolar = connectionBlock.usTransform & (1 << 14);
    bool srcInvert = connectionBlock.usTransform & (1 << 15);

    m_blocks.emplace_back(
     connectionBlock.usSource, connectionBlock.usControl,
     connectionBlock.usDestination, connectionBlock.lScale,
     TransformParams(srcInvert, srcBipolar, srcTransType),
     TransformParams(ctrlInvert, ctrlBipolar, ctrlTransType));
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
     TransformParams(invertedSources.find(src) != invertedSources.end(),
                     bipolarSources.find(src) != bipolarSources.end(),
                     transType),
     TransformParams(false, false, transType));
  }
}