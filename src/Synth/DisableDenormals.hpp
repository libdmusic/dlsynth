#ifndef DISABLEDENORMALS_HPP
#define DISABLEDENORMALS_HPP

#include <cstdint>

namespace DLSynth {
namespace Synth {
  /// Truncates denormal floats to zero for the duration of the scope
  class DisableDenormals final {
    std::uint32_t m_status;

  public:
    DisableDenormals() noexcept;
    ~DisableDenormals() noexcept;
  };
} // namespace Synth
} // namespace DLSynth

#endif