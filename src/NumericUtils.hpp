#ifndef NUMERICUTILS_HPP
#define NUMERICUTILS_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace DLSynth {
struct int24_t {
  std::array<std::uint8_t, 3> value;

  inline explicit operator int() const {
    return ((int)value[0] << 8) | ((int)value[1] << 16) | ((int)value[2] << 24);
  }

  inline explicit operator float() const { return (float)(int)(*this); }

  inline bool operator<(int other) { return (int)(*this) < other; }

  inline bool operator>(int other) { return (int)(*this) > other; }

  inline bool operator<=(int other) { return !(*this > other); }

  inline bool operator>=(int other) { return !(*this < other); }

  inline bool operator==(int other) { return (int)(*this) == other; }

  inline bool operator!=(int other) { return (int)(*this) == other; }
};

template <typename T> constexpr float normalize(T value) {
  return value < 0 ? -((float)value / std::numeric_limits<T>::min())
                   : (float)value / std::numeric_limits<T>::max();
}

template <> constexpr float normalize(std::uint8_t value) {
  return ((value / 255.f) - .5f) * 2.f;
}

template <typename TIn, typename TOut> constexpr TOut clamp_convert(TIn value) {
  if (value > std::numeric_limits<TOut>::max()) {
    return std::numeric_limits<TOut>::max();
  } else if (value < std::numeric_limits<TOut>::min()) {
    return std::numeric_limits<TIn>::min();
  } else {
    return (TOut)value;
  }
}

/// Converts a frequency expressed in absolute pitch units to hertz
inline float absolutePitchToFrequency(std::int32_t value) {
  return std::exp2((((float)value / 65536.f) - 6900.f) / 1200.f) * 440.f;
}

/// Converts a relative pitch value to a ratio
inline float relativePitchToRatio(std::int16_t value) {
  return std::exp2((float)value / 1200.f);
}

/// Converts a duration expressed in time cents to seconds
inline float timeCentsToSecs(std::int32_t value) {
  return std::exp2((float)value / (1200.f * 65536.f));
}

/// Converts a gain expressed in relative gain units to a ratio
inline float relativeGainToRatio(std::int32_t value) {
  return std::exp((float)value / (200.f * 65536.f));
}

} // namespace DLSynth

static_assert(sizeof(DLSynth::int24_t) == 3, "Needs to be 3-byte aligned");

namespace std {
template <> struct numeric_limits<DLSynth::int24_t> {
  static constexpr int min() { return (int)(0x7FFFFF00); }
  static constexpr int max() { return (int)(0x80000000); }
};
} // namespace std

#endif