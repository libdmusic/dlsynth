STRUCT_BEGIN(WaveFormatEx)
FIELD(std::uint16_t, FormatTag)
FIELD(std::uint16_t, NumChannels)
FIELD(std::uint32_t, SamplesPerSec)
FIELD(std::uint32_t, AvgBytesPerSec)
FIELD(std::uint16_t, BlockAlign)
FIELD(std::uint16_t, BitsPerSample)
FIELD(std::uint16_t, ExtraInfoSize)
VARARR_MAX(char, ExtraData, ExtraInfoSize)

STRUCT_METHOD(inline WaveFormat toWaveFormat() const {
  return {FormatTag, NumChannels, SamplesPerSec, AvgBytesPerSec, BlockAlign};
})
STRUCT_END(WaveFormatEx)