#ifndef ARTICULATOR_HPP
#define ARTICULATOR_HPP

#include "ExpressionParser.hpp"
#include <cstdint>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
enum class Source : std::uint16_t {
  None = 0x0000,
  LFO = 0x0001,
  KeyOnVelocity = 0x0002,
  KeyNumber = 0x0003,
  EG1 = 0x0004,
  EG2 = 0x0005,
  PitchWheel = 0x0006,
  PolyPressure = 0x0007,
  ChannelPressure = 0x0008,
  Vibrato = 0x0009,
  CC1 = 0x0081,
  CC7 = 0x0087,
  CC10 = 0x008a,
  CC11 = 0x008b,
  CC91 = 0x00db,
  CC93 = 0x00dd,
  RPN0 = 0x0100,
  RPN1 = 0x0101,
  RPN2 = 0x0102
};

constexpr std::uint16_t max_source = static_cast<std::uint16_t>(Source::RPN2);

constexpr std::uint16_t maxValue(Source source) {
  if (source == Source::PitchWheel || ((std::uint16_t)source & 0x0100)) {
    return 0x4000;
  } else {
    return 0x0080;
  }
}

enum class Destination : std::uint16_t {
  None = 0x0000,
  Gain = 0x0001,
  Reserved = 0x0002,
  Pitch = 0x0003,
  Pan = 0x0004,
  KeyNumber = 0x0005,
  LeftCh = 0x0010,
  RightCh = 0x0011,
  CenterCh = 0x0012,
  LfeCh = 0x0013,
  LeftRearCh = 0x0014,
  RightRearCh = 0x0015,
  Chorus = 0x0080,
  Reverb = 0x0081,
  LfoFrequency = 0x0104,
  LfoStartDelay = 0x0105,
  VibratoFrequency = 0x0114,
  VibratoStartDelay = 0x0115,
  EG1AttackTime = 0x0206,
  EG1DecayTime = 0x0207,
  EG1Reserved = 0x0208,
  EG1ReleaseTime = 0x0209,
  EG1SustainLevel = 0x020A,
  EG1DelayTime = 0x020B,
  EG1HoldTime = 0x020C,
  EG1ShutdownTime = 0x020D,
  EG2AttackTime = 0x030A,
  EG2DecayTime = 0x030B,
  EG2Reserved = 0x030C,
  EG2ReleaseTime = 0x030D,
  EG2SustainLevel = 0x030E,
  EG2DelayTime = 0x030F,
  EG2HoldTime = 0x0310,
  FilterCutoff = 0x0500,
  FilterQ = 0x0501
};

constexpr std::uint16_t max_destination =
 static_cast<std::uint16_t>(Destination::FilterQ);

enum class TransformType : std::uint16_t {
  None = 0x0000,
  Concave = 0x0001,
  Convex = 0x0002,
  Switch = 0x0003
};

class TransformParams final {
  bool m_invert;
  bool m_bipolar;
  TransformType m_type;

public:
  constexpr TransformParams(bool invert, bool bipolar,
                            TransformType type) noexcept
    : m_invert(invert), m_bipolar(bipolar), m_type(type) {}
  constexpr bool invert() const noexcept { return m_invert; }
  constexpr bool bipolar() const noexcept { return m_bipolar; }
  constexpr TransformType type() const noexcept { return m_type; }
};

/// Defines how an input signal affects an instrument parameter
class ConnectionBlock final {
  Source m_source;
  Source m_control;
  Destination m_destination;
  std::int32_t m_scale;
  TransformParams m_sourceTransform;
  TransformParams m_controlTransform;

public:
  constexpr ConnectionBlock(Source src, Source ctrl, Destination dest,
                            std::int32_t scale, const TransformParams &srcTrans,
                            const TransformParams &ctrlTrans)
    : m_source(src)
    , m_control(ctrl)
    , m_destination(dest)
    , m_scale(scale)
    , m_sourceTransform(srcTrans)
    , m_controlTransform(ctrlTrans) {}

  constexpr Source source() const { return m_source; }
  constexpr Source control() const { return m_control; }
  constexpr Destination destination() const { return m_destination; }
  constexpr std::int32_t scale() const { return m_scale; }
  constexpr const TransformParams &sourceTransform() const {
    return m_sourceTransform;
  }
  constexpr const TransformParams &controlTransform() const {
    return m_controlTransform;
  }
};

/// A set of \ref ConnectionBlock s that defines the parameters of an instrument
class Articulator final {
  std::vector<ConnectionBlock> m_blocks;

public:
  Articulator(const std::vector<ConnectionBlock> &blocks) noexcept;

  /// List of connection blocks defined for this articulator
  const std::vector<ConnectionBlock> &connectionBlocks() const noexcept;

  static Articulator readChunk(riffcpp::Chunk &chunk,
                               const ExpressionParser &exprParser);
};
} // namespace DLSynth

#endif