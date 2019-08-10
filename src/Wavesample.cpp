#include "Wavesample.hpp"
#include "NumericUtils.hpp"
#include "Structs.hpp"
#include <algorithm>

using namespace DLSynth;

Wavesample::Wavesample(std::uint16_t unityNode, std::int16_t fineTune,
                       std::int32_t gain) noexcept
  : m_unityNote(unityNode), m_fineTune(fineTune), m_gain(gain) {}

Wavesample::Wavesample(std::uint16_t unityNode, std::int16_t fineTune,
                       std::int32_t gain, const WavesampleLoop &loop) noexcept
  : m_unityNote(unityNode)
  , m_fineTune(fineTune)
  , m_gain(gain)
  , m_loop(std::make_unique<WavesampleLoop>(loop)) {}

Wavesample::Wavesample(Wavesample &&wavesample) noexcept
  : m_unityNote(wavesample.m_unityNote)
  , m_fineTune(wavesample.m_fineTune)
  , m_gain(wavesample.m_gain)
  , m_loop(std::move(wavesample.m_loop)) {}
Wavesample::Wavesample(const Wavesample &wavesample) noexcept
  : m_unityNote(wavesample.m_unityNote)
  , m_fineTune(wavesample.m_fineTune)
  , m_gain(wavesample.m_gain)
  , m_loop(wavesample.m_loop
            ? std::make_unique<WavesampleLoop>(*wavesample.m_loop)
            : nullptr) {}

Wavesample &Wavesample::operator=(const Wavesample &wavesample) noexcept {
  m_unityNote = wavesample.m_unityNote;
  m_fineTune = wavesample.m_fineTune;
  m_gain = wavesample.m_gain;
  m_loop = wavesample.m_loop
            ? std::make_unique<WavesampleLoop>(*wavesample.m_loop)
            : nullptr;

  return *this;
}

std::uint16_t Wavesample::unityNote() const noexcept { return m_unityNote; }

float Wavesample::fineTune() const noexcept { return m_fineTune; }

float Wavesample::gain() const noexcept {
  return relativeGainUnitsToBels(m_gain);
}

const WavesampleLoop *Wavesample::loop() const noexcept { return m_loop.get(); }

Wavesample Wavesample::readChunk(riffcpp::Chunk &chunk) {
  wsmp wavesample = ::DLSynth::readChunk<wsmp>(chunk);
  if (wavesample.cSampleLoops) {
    const auto &loopData = wavesample.loops[0];

    return Wavesample(
     wavesample.usUnityNote, wavesample.sFineTune, wavesample.lGain,
     WavesampleLoop(static_cast<LoopType>(loopData.ulLoopType),
                    loopData.ulLoopStart, loopData.ulLoopLength));
  } else {
    return Wavesample(wavesample.usUnityNote, wavesample.sFineTune,
                      wavesample.lGain);
  }
}