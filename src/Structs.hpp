#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include "Error.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <riffcpp.hpp>
#include <vector>

namespace DLSynth {

#undef STRUCT_BEGIN
#define STRUCT_BEGIN(NAME) struct NAME {
#undef STRUCT_END
#define STRUCT_END(NAME)                                                       \
  }                                                                            \
  ;
#undef FIELD
#define FIELD(TYPE, NAME) TYPE NAME;
#undef VARARR
#define VARARR(TYPE, NAME, COUNT) std::vector<TYPE> NAME;
#undef VARARR_MAX
#define VARARR_MAX(TYPE, NAME, COUNT) std::vector<TYPE> NAME;
#undef VARARR_OFF
#define VARARR_OFF(TYPE, NAME, COUNT, OFFSET) std::vector<TYPE> NAME;
#undef FIXARR
#define FIXARR(TYPE, NAME, COUNT) std::array<TYPE, COUNT> NAME;
#undef FIXARR_OFF
#define FIXARR_OFF(TYPE, NAME, COUNT, OFFSET) std::vector<TYPE, COUNT> NAME;
#undef STRUCT_METHOD
#define STRUCT_METHOD(...) __VA_ARGS__

#include "Structs/StructList.hpp"

template <typename T> struct StructLoader {
  static const char *readBuffer(const char *begin, const char *end, T &output) {
    if (begin > end || (end - begin) < sizeof(T)) {
      throw Error("Wrong data size", ErrorCode::INVALID_FILE);
    }

    const char *data_end = begin + sizeof(T);

    std::array<char, sizeof(T)> buffer;
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

#undef STRUCT_BEGIN
#define STRUCT_BEGIN(NAME)                                                     \
  template <> struct StructLoader<NAME> {                                      \
    static const char *readBuffer(const char *begin, const char *end,          \
                                  NAME &output) {                              \
      if (begin > end) {                                                       \
        throw Error("Wrong data size", ErrorCode::INVALID_FILE);               \
      }                                                                        \
      const char *cur_pos = begin;
#undef STRUCT_END
#define STRUCT_END(NAME)                                                       \
  return cur_pos;                                                              \
  }                                                                            \
  }                                                                            \
  ;
#undef FIELD
#define FIELD(TYPE, NAME)                                                      \
  cur_pos = StructLoader<TYPE>::readBuffer(cur_pos, end, output.NAME);
#undef VARARR
#define VARARR(TYPE, NAME, COUNT)                                              \
  output.NAME.resize(output.COUNT);                                            \
  cur_pos = readArray<TYPE>(cur_pos, end, output.COUNT, output.NAME);
#undef VARARR_MAX
#define VARARR_MAX(TYPE, NAME, COUNT)                                          \
  std::size_t count = 0;                                                       \
  while (count < output.COUNT && cur_pos < end) {                              \
    TYPE elem;                                                                 \
    cur_pos = StructLoader<TYPE>::readBuffer(cur_pos, end, elem);              \
    output.NAME.push_back(elem);                                               \
  }
#undef VARARR_OFF
#define VARARR_OFF(TYPE, NAME, COUNT, OFFSET)                                  \
  cur_pos = begin + output.OFFSET;                                             \
  output.NAME.resize(output.COUNT);                                            \
  cur_pos = readArray<TYPE>(cur_pos, end, output.COUNT, output.NAME);
#undef FIXARR
#define FIXARR(TYPE, NAME, COUNT)                                              \
  cur_pos = readArray<TYPE>(cur_pos, end, output.NAME);
#undef FIXARR_OFF
#define FIXARR_OFF(TYPE, NAME, COUNT, OFFSET)                                  \
  cur_pos = begin + output.OFFSET;                                             \
  cur_pos = readArray<TYPE>(cur_pos, end, output.NAME);
#undef STRUCT_METHOD
#define STRUCT_METHOD(...)

#include "Structs/StructList.hpp"

} // namespace DLSynth

#endif