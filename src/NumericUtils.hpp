#ifndef NUMERICUTILS_HPP
#define NUMERICUTILS_HPP

#include <array>
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
} // namespace DLSynth

static_assert(sizeof(DLSynth::int24_t) == 3, "Needs to be 3-byte aligned");

namespace std {
template <> struct numeric_limits<DLSynth::int24_t> {
  static constexpr int min() { return (int)(0x7FFFFF00); }
  static constexpr int max() { return (int)(0x80000000); }
};
} // namespace std

#endif