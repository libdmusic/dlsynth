#ifndef DECODERS_HPP
#define DECODERS_HPP
#include "../Wave.hpp"

#define DLSYNTH_FACTORY(name)                                                  \
  class name : public WaveDecoderFactory {                                     \
    name();                                                                    \
                                                                               \
  public:                                                                      \
    ~name() override;                                                          \
    virtual WaveDecoderPtr                                                     \
    createDecoder(const WaveFormat &format, std::uint32_t fact,                \
                  const std::vector<char> &data) override;                     \
    virtual WaveDecoderPtr                                                     \
    createDecoder(const WaveFormat &format,                                    \
                  const std::vector<char> &data) override;                     \
    virtual WaveDecoderPtr                                                     \
    createDecoder(const WaveFormatEx &format, std::uint32_t fact,              \
                  const std::vector<char> &data) override;                     \
    virtual WaveDecoderPtr                                                     \
    createDecoder(const WaveFormatEx &format,                                  \
                  const std::vector<char> &data) override;                     \
    static name *getInstance();                                                \
  };

namespace DLSynth {
namespace Decoders {
  DLSYNTH_FACTORY(PcmDecoderFactory)
  DLSYNTH_FACTORY(FloatDecoderFactory)
  DLSYNTH_FACTORY(MsAdpcmDecoderFactory)
  DLSYNTH_FACTORY(ALawDecoderFactory)
  DLSYNTH_FACTORY(MuLawDecoderFactory)
} // namespace Decoders
} // namespace DLSynth

#endif