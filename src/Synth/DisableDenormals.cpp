#include "DisableDenormals.hpp"

#if DLSYNTH_USE_SSE >= 1
#include <xmmintrin.h>
#endif

using namespace DLSynth::Synth;

DisableDenormals::DisableDenormals() noexcept {
#if DLSYNTH_USE_SSE >= 1
  m_status = _mm_getcsr();
  constexpr std::uint32_t mask = ~(1 << 6);
  _mm_setcsr(m_status & mask);
#endif
};

DisableDenormals::~DisableDenormals() noexcept {
#if DLSYNTH_USE_SSE >= 1
  _mm_setcsr(m_status);
#endif
}