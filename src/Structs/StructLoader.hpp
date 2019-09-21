#ifndef STRUCTLOADER_HPP
#define STRUCTLOADER_HPP

#include "../Error.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <riffcpp.hpp>
#include <vector>

template <typename T> struct StructLoader {
  static const char *readBuffer(const char *begin, const char *end, T &output) {
    if (begin > end || (end - begin) < sizeof(T)) {
      throw DLSynth::Error("Wrong data size", DLSynth::ErrorCode::INVALID_FILE);
    }

    const char *data_end = begin + sizeof(T);

    alignas(T) std::array<char, sizeof(T)> buffer;
    std::copy(begin, data_end, buffer.begin());

#ifdef DLSYNTH_BIGENDIAN
    std::reverse(buffer.begin(), buffer.end());
#endif

    output = *reinterpret_cast<const T *>(buffer.data());

    return data_end;
  }
};

template <typename T> T readVector(const std::vector<char> &buffer) {
  const char *begin = buffer.data();
  const char *end = begin + buffer.size();
  T output;
  StructLoader<T>::readBuffer(begin, end, output);
  return output;
}

template <typename T> T readChunk(riffcpp::Chunk &chunk) {
  std::vector<char> buffer;
  buffer.resize(chunk.size());
  chunk.read_data(buffer.data(), buffer.size());
  return readVector<T>(buffer);
}

template <typename T>
const char *readArray(const char *begin, const char *end, std::size_t count,
                      std::vector<T> &output) {
  const char *pos = begin;
  for (std::size_t i = 0; i < count; i++) {
    pos = StructLoader<T>::readBuffer(pos, end, output[i]);
  }
  return pos;
}

template <typename T, std::size_t Count>
const char *readArray(const char *begin, const char *end,
                      std::array<T, Count> &output) {
  const char *pos = begin;
  for (std::size_t i = 0; i < Count; i++) {
    pos = StructLoader<T>::readBuffer(pos, end, output[i]);
  }
  return pos;
}

#endif