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

template <typename T> constexpr T inverse_normalize(float value) {
  if (value > 1.f) {
    value = 1.f;
  } else if (value < -1.f) {
    value = -1.f;
  }

  if (value > 0) {
    return static_cast<T>(value * std::numeric_limits<T>::max());
  } else {
    return static_cast<T>(-value * std::numeric_limits<T>::min());
  }
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

/// Converts a frequency expressed in absolute pitch units to cents
/**
 * A value of 6900 means 440hz (A4)
 */
inline float absolutePitchToCents(std::int32_t value) {
  return value / 65536.f;
}

/// Converts a duration expressed in 32-bit time cents to time cents
inline float timeUnitsToCents(std::int32_t value) {
  if (value == 0x80000000) {
    return -std::numeric_limits<float>::infinity();
  }
  return value / 65536.f;
}

/// Converts a gain expressed in relative gain units to centibels
inline float relativeGainUnitsToBels(std::int32_t value) {
  return (float)value / (200.f * 65536.f);
}

/// Converts a percentage expressed in percent units to a ratio
inline float percentUnitsToRatio(std::int32_t value) {
  return (float)value / (1000.f * 65536.f);
}

inline float centsToRatio(float cents) { return std::exp2(cents / 1200.f); }

inline float centsToFreq(float cents) {
  return centsToRatio(cents - 6900) * 440.f;
}

inline float belsToGain(float bels) { return std::pow(10.f, bels); }

inline float centsToSecs(float cents) { return std::exp2(cents / 1200.f); }

constexpr float PI = 3.14159f;

} // namespace DLSynth

static_assert(sizeof(DLSynth::int24_t) == 3, "Needs to be 3-byte aligned");

namespace std {
template <> struct numeric_limits<DLSynth::int24_t> {
  static constexpr int min() { return (int)(0x7FFFFF00); }
  static constexpr int max() { return (int)(0x80000000); }
};
} // namespace std

#endif