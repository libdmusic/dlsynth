#include "Articulator.hpp"
#include "CommonFourCCs.hpp"
#include "Error.hpp"
#include "Structs.hpp"
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

void Articulator::load_art2(riffcpp::Chunk &chunk) {
  art articulator = readChunk<art>(chunk);

  for (const auto &connectionBlock : articulator.blocks) {
    TransformType ctrlTransType =
     static_cast<TransformType>((connectionBlock.usTransform & 0x00F0) >> 4);
    bool ctrlBipolar = connectionBlock.usTransform & (1 << 8);
    bool ctrlInvert = connectionBlock.usTransform & (1 << 9);
    TransformType srcTransType =
     static_cast<TransformType>((connectionBlock.usTransform & 0x0F00) >> 8);
    bool srcBipolar = connectionBlock.usTransform & (1 << 14);
    bool srcInvert = connectionBlock.usTransform & (1 << 15);

    m_blocks.emplace_back(
     static_cast<Source>(connectionBlock.usSource),
     static_cast<Source>(connectionBlock.usControl),
     static_cast<Destination>(connectionBlock.usDestination),
     connectionBlock.lScale,
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
  art articulator = readChunk<art>(chunk);

  for (const auto &connectionBlock : articulator.blocks) {
    TransformType transType = (TransformType)connectionBlock.usTransform;

    Source src = static_cast<Source>(connectionBlock.usSource);
    Source ctrl = static_cast<Source>(connectionBlock.usControl);
    Destination dst = static_cast<Destination>(connectionBlock.usDestination);

    m_blocks.emplace_back(
     src, ctrl, dst, connectionBlock.lScale,
     TransformParams(invertedSources.find(src) != invertedSources.end(),
                     bipolarSources.find(src) != bipolarSources.end(),
                     transType),
     TransformParams(false, false, transType));
  }
}