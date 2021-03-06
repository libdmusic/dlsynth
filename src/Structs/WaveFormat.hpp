#ifndef WAVEFORMAT_HPP
#define WAVEFORMAT_HPP
/*
================= DO NOT EDIT THIS FILE ====================
This file has  been automatically generated by genstructs.py

Please  edit  structs.xml instead, and  re-run genstructs.py
============================================================
*/
#include "StructLoader.hpp"
struct WaveFormat {
  std::uint16_t FormatTag;
  std::uint16_t NumChannels;
  std::uint32_t SamplesPerSec;
  std::uint32_t AvgBytesPerSec;
  std::uint16_t BlockAlign;

  static constexpr std::uint16_t Unknown = 0x0000;
  static constexpr std::uint16_t Pcm = 0x0001;
  static constexpr std::uint16_t MsAdpcm = 0x0002;
  static constexpr std::uint16_t Float = 0x0003;
  static constexpr std::uint16_t ALaw = 0x0006;
  static constexpr std::uint16_t MuLaw = 0x0007;
  static constexpr std::uint16_t DviAdpcm = 0x0011;
  static constexpr std::uint16_t ImaAdpcm = DviAdpcm;
};

template <> struct StructLoader<WaveFormat> {
  static const char* readBuffer(const char *begin, const char *end, WaveFormat &output) {
    if (begin > end) {
      throw DLSynth::Error("Wrong data size", DLSynth::ErrorCode::INVALID_FILE);
    }
    const char *cur_pos = begin;
    cur_pos = StructLoader<std::uint16_t>::readBuffer(cur_pos, end, output.FormatTag);
    cur_pos = StructLoader<std::uint16_t>::readBuffer(cur_pos, end, output.NumChannels);
    cur_pos = StructLoader<std::uint32_t>::readBuffer(cur_pos, end, output.SamplesPerSec);
    cur_pos = StructLoader<std::uint32_t>::readBuffer(cur_pos, end, output.AvgBytesPerSec);
    cur_pos = StructLoader<std::uint16_t>::readBuffer(cur_pos, end, output.BlockAlign);
    return cur_pos;
  }
};
#endif
