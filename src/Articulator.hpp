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

enum class TransformType : std::uint16_t {
  None = 0x0000,
  Concave = 0x0001,
  Convex = 0x0002,
  Switch = 0x0003
};

class Transform {
  bool m_invert;
  bool m_bipolar;
  TransformType m_type;

public:
  Transform(bool invert, bool bipolar, TransformType type);
  bool invert() const;
  bool bipolar() const;
  TransformType type() const;
};

/// Defines how an input signal affects an instrument parameter
class ConnectionBlock {
  Source m_source;
  Source m_control;
  Destination m_destination;
  std::int32_t m_scale;
  Transform m_sourceTransform;
  Transform m_controlTransform;
  TransformType m_outputTransformType;

public:
  ConnectionBlock(Source src, Source ctrl, Destination dest, std::int32_t scale,
                  const Transform &srcTrans, const Transform &ctrlTrans,
                  TransformType outTrans);

  Source source() const;
  Source control() const;
  Destination destination() const;
  std::int32_t scale() const;
  const Transform &sourceTransform() const;
  const Transform &controlTransform() const;
  TransformType outputTransformType() const;
};

/// A set of \ref ConnectionBlock s that defines the parameters of an instrument
class Articulator {
  std::vector<ConnectionBlock> m_blocks;

  void load_art1(riffcpp::Chunk &chunk);

  void load_art2(riffcpp::Chunk &chunk);

public:
  Articulator(riffcpp::Chunk &chunk, const ExpressionParser &exprParser);

  /// List of connection blocks defined for this articulator
  const std::vector<ConnectionBlock> &connectionBlocks() const;
};
} // namespace DLSynth

#endif