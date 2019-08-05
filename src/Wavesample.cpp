#include "Wavesample.hpp"
#include "NumericUtils.hpp"
#include "Structs.hpp"
#include <algorithm>

using namespace DLSynth;

Wavesample::Wavesample(riffcpp::Chunk &chunk) {
  wsmp wavesample = readChunk<wsmp>(chunk);
  m_gain = wavesample.lGain;
  m_fineTune = wavesample.sFineTune;
  m_unityNote = wavesample.usUnityNote;
  if (wavesample.cSampleLoops) {
    const auto &loopData = wavesample.loops[0];
    m_loop = std::make_unique<WavesampleLoop>(
     static_cast<LoopType>(loopData.ulLoopType), loopData.ulLoopStart,
     loopData.ulLoopLength);
  }
}

float Wavesample::gain() const { return relativeGainUnitsToBels(m_gain); }
float Wavesample::fineTune() const { return m_fineTune; }
std::uint16_t Wavesample::unityNote() const { return m_unityNote; }
const WavesampleLoop *Wavesample::loop() const { return m_loop.get(); }