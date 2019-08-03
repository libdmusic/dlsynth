#include "Wavesample.hpp"
#include "NumericUtils.hpp"
#include "StructUtils.hpp"
#include <algorithm>

using namespace DLSynth;

struct wsmp_loop {
  std::uint32_t cbSize;
  LoopType ulLoopType;
  std::uint32_t ulLoopStart;
  std::uint32_t ulLoopLength;
};

struct wsmp {
  std::uint32_t cbSize;
  std::uint16_t usUnityNote;
  std::int16_t sFineTune;
  std::int32_t lGain;
  std::uint32_t fulOptions;
  std::uint32_t cSampleLoops;
};

Wavesample::Wavesample(riffcpp::Chunk &chunk) {
  std::vector<char> data(chunk.size());
  chunk.read_data(data.data(), data.size());

  char *data_begin = data.data();
  char *data_end = data_begin + data.size();

  const wsmp *wavesample = readStruct<wsmp>(data_begin, data_end);
  m_gain = wavesample->lGain;
  m_fineTune = wavesample->sFineTune;
  m_unityNote = wavesample->usUnityNote;
  if (wavesample->cSampleLoops) {
    const wsmp_loop *loopData =
     readStruct<wsmp_loop>(data_begin + wavesample->cbSize, data_end);
    m_loop = std::make_unique<WavesampleLoop>(
     loopData->ulLoopType, loopData->ulLoopStart, loopData->ulLoopLength);
  }
}

float Wavesample::gain() const { return relativeGainUnitsToBels(m_gain); }
float Wavesample::fineTune() const { return m_fineTune; }
std::uint16_t Wavesample::unityNote() const { return m_unityNote; }
const WavesampleLoop *Wavesample::loop() const { return m_loop.get(); }