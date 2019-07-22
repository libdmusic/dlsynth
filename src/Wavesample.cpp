#include "Wavesample.hpp"
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
  m_loops.reserve(wavesample->cSampleLoops);
  for (int i = 0; i < wavesample->cSampleLoops; i++) {
    m_loops.push_back(WavesampleLoop(wavesample->loops + i));
  }
}

std::int32_t Wavesample::gain() const { return m_gain; }
std::int16_t Wavesample::fineTune() const { return m_fineTune; }
std::uint16_t Wavesample::unityNote() const { return m_unityNote; }
const std::vector<WavesampleLoop> &Wavesample::loops() const { return m_loops; }