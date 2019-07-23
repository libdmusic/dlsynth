#include "Wavesample.hpp"
#include "NumericUtils.hpp"
#include <algorithm>

using namespace DLSynth;

namespace DLSynth {
struct wsmp_loop {
  std::uint32_t cbSize;
  LoopType ulLoopType;
  std::uint32_t ulLoopStart;
  std::uint32_t ulLoopLength;
};
} // namespace DLSynth

WavesampleLoop::WavesampleLoop(const wsmp_loop *loop)
  : m_type(loop->ulLoopType)
  , m_start(loop->ulLoopStart)
  , m_length(loop->ulLoopLength) {}

LoopType WavesampleLoop::type() const { return m_type; }
std::uint32_t WavesampleLoop::start() const { return m_start; }
std::uint32_t WavesampleLoop::length() const { return m_length; }

struct wsmp {
  std::uint32_t cbSize;
  std::uint16_t usUnityNote;
  std::int16_t sFineTune;
  std::int32_t lGain;
  std::uint32_t fulOptions;
  std::uint32_t cSampleLoops;
  wsmp_loop loops[0];
};

Wavesample::Wavesample(riffcpp::Chunk &chunk) {
  std::vector<char> data(chunk.size());
  chunk.read_data(data.data(), data.size());

  wsmp *wavesample = reinterpret_cast<wsmp *>(data.data());
  m_gain = wavesample->lGain;
  m_fineTune = wavesample->sFineTune;
  m_unityNote = wavesample->usUnityNote;
  if (wavesample->cSampleLoops) {
    m_loop = std::make_unique<WavesampleLoop>(wavesample->loops);
  }
}

float Wavesample::gain() const { return relativeGainToRatio(m_gain); }
float Wavesample::fineTune() const { return relativePitchToRatio(m_fineTune); }
std::uint16_t Wavesample::unityNote() const { return m_unityNote; }
const WavesampleLoop *Wavesample::loop() const { return m_loop.get(); }