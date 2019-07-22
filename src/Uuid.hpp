#ifndef UUID_HPP
#define UUID_HPP
#include <array>
#include <cstdint>

namespace DLSynth {
struct Uuid {
  std::uint32_t a;
  std::uint16_t b;
  std::uint16_t c;
  std::array<std::uint8_t, 8> d;

  constexpr bool operator==(const Uuid &rhs) const {
    return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d;
  }

  constexpr bool operator!=(const Uuid &rhs) const { return !(*this == rhs); }
};

static_assert(sizeof(Uuid) == 16, "Uuid needs to be 16-byte");
} // namespace DLSynth

namespace std {
template <> struct hash<DLSynth::Uuid> {
  constexpr std::size_t operator()(const DLSynth::Uuid &id) const noexcept {
    return ((id.d[0] << 0) | (id.d[1] << 8) | (id.d[2] << 16) |
            (id.d[3] << 24) | ((std::size_t)id.d[4] << 32) |
            ((std::size_t)id.d[5] << 40) | ((std::size_t)id.d[6] << 48) |
            ((std::size_t)id.d[7] << 56)) ^
           id.c ^ id.b ^ id.a;
  }
};
} // namespace std

#endif