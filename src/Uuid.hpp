#ifndef UUID_ADDITIONAL_HPP
#define UUID_ADDITIONAL_HPP
#include "Structs/Uuid.hpp"
#include <array>
#include <cstdint>

constexpr bool operator==(const Uuid &lhs, const Uuid &rhs) noexcept {
  return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d;
}
constexpr bool operator!=(const Uuid &lhs, const Uuid &rhs) noexcept {
  return !(lhs == rhs);
}

static_assert(sizeof(Uuid) == 16, "Uuid needs to be 16-byte");
namespace std {
template <> struct hash<Uuid> {
  constexpr std::size_t operator()(const Uuid &id) const noexcept {
    return ((id.d[0] << 0) | (id.d[1] << 8) | (id.d[2] << 16) |
            (id.d[3] << 24) | ((std::size_t)id.d[4] << 32) |
            ((std::size_t)id.d[5] << 40) | ((std::size_t)id.d[6] << 48) |
            ((std::size_t)id.d[7] << 56)) ^
           id.c ^ id.b ^ id.a;
  }
};
} // namespace std

#endif