#ifndef STRUCTUTILS_HPP
#define STRUCTUTILS_HPP
#include "Error.hpp"
#include <cassert>
#include <cstddef>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {
template <typename T> const T *readStruct(const char *begin, const char *end) {
  if (end - begin < sizeof(T) || begin > end) {
    throw Error("Invalid data size", ErrorCode::INVALID_FILE);
  }

  return reinterpret_cast<const T *>(begin);
}

template <typename T>
const T *readStruct(const unsigned char *begin, const unsigned char *end) {
  if (end - begin < sizeof(T) || begin > end) {
    throw Error("Invalid data size", ErrorCode::INVALID_FILE);
  }

  return reinterpret_cast<const T *>(begin);
}

template <typename T> T readStruct(riffcpp::Chunk &chunk) {
  if (chunk.size() < sizeof(T)) {
    throw Error("Invalid data size", ErrorCode::INVALID_FILE);
  }

  std::vector<char> buf(chunk.size());
  chunk.read_data(buf.data(), buf.size());
  return *reinterpret_cast<T *>(buf.data());
}

template <typename T>
void readStructArray(const char *begin, const char *end, std::size_t count,
                     std::vector<T> &vect, std::size_t structSize = sizeof(T)) {
  assert(structSize >= sizeof(T));

  if ((end - begin) / structSize < count || begin > end) {
    throw Error("Invalid data size", ErrorCode::INVALID_FILE);
  }

  vect.reserve(count);
  for (const char *start = begin; start < end; start += structSize) {
    vect.push_back(*(reinterpret_cast<const T *>(start)));
  }
}
} // namespace DLSynth

#endif