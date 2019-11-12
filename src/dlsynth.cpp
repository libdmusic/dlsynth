#include "Error.hpp"
#include "Instrument.hpp"
#include "NumericUtils.hpp"
#include "Sound.hpp"
#include "Structs/Range.hpp"
#include "Synth/Synthesizer.hpp"
#include "Wave.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <dlsynth.h>
#include <limits>
#include <memory>
#include <riffcpp.hpp>

const char *dlsynth_get_error_message(dlsynth_error err) {
  switch (err) {
#define DLSYNTH_ERROR(name, code, msg)                                         \
  case code:                                                                   \
    return msg;
#include <dlsynth_errors.h>
#undef DLSYNTH_ERROR
  default:
    return nullptr;
  }
}

static dlsynth_error get_err_from_exception(riffcpp::ErrorType type) {
  switch (type) {
  case riffcpp::ErrorType::CannotOpenFile:
    return DLSYNTH_CANNOT_OPEN_FILE;
  case riffcpp::ErrorType::InvalidFile:
    return DLSYNTH_INVALID_FILE;
  case riffcpp::ErrorType::NullBuffer:
    return DLSYNTH_INVALID_ARGS;
  default:
    return DLSYNTH_UNKNOWN_ERROR;
  }
}

struct dlsynth {
  static constexpr std::size_t renderBufferSizeFrames = 1024;
  static constexpr std::size_t renderBufferSize = renderBufferSizeFrames * 2;

  std::unique_ptr<DLSynth::Synth::Synthesizer> synth;

  std::array<float, renderBufferSize> renderBuffer;

  ~dlsynth() { synth = nullptr; }
};

struct dlsynth_sound {
  DLSynth::Sound sound;
};
struct dlsynth_instr {
  const dlsynth_sound *sound;
  std::size_t index;
};

struct dlsynth_wav {
  DLSynth::Wave wave;
};
dlsynth_error dlsynth_load_sound_file(const char *path, uint32_t sampleRate,
                                      dlsynth_sound **sound) {
  if (sound == nullptr || path == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  try {
    riffcpp::Chunk chunk(path);
    *sound = new dlsynth_sound{DLSynth::Sound::readChunk(chunk, sampleRate)};
    return DLSYNTH_NO_ERROR;
  } catch (riffcpp::Error &e) {
    return get_err_from_exception(e.type());
  } catch (DLSynth::Error &e) {
    return static_cast<dlsynth_error>(e.code());
  } catch (std::exception &) {
    return DLSYNTH_UNKNOWN_ERROR;
  }
}

dlsynth_error dlsynth_load_sound_buf(const void *buf, size_t buf_size,
                                     uint32_t sampleRate,
                                     dlsynth_sound **sound) {
  if (sound == nullptr || buf == nullptr || buf_size == 0) {
    return DLSYNTH_INVALID_ARGS;
  }

  try {
    riffcpp::Chunk chunk(buf, buf_size);
    *sound = new dlsynth_sound{DLSynth::Sound::readChunk(chunk, sampleRate)};
    return DLSYNTH_NO_ERROR;
  } catch (riffcpp::Error &e) {
    return get_err_from_exception(e.type());
  } catch (DLSynth::Error &e) {
    return static_cast<dlsynth_error>(e.code());
  } catch (std::exception &) {
    return DLSYNTH_UNKNOWN_ERROR;
  }
}

dlsynth_error dlsynth_free_sound(dlsynth_sound *sound) {
  if (sound != nullptr) {
    delete sound;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_sound_instr_count(const dlsynth_sound *sound,
                                        std::size_t *count) {
  if (sound == nullptr || count == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *count = sound->sound.instruments().size();
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_sound_instr_info(const dlsynth_instr *instr,
                                       uint32_t *bank, uint32_t *patch,
                                       int *isDrum) {
  if (instr == nullptr || bank == nullptr || patch == nullptr ||
      instr->sound == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  const auto &i = instr->sound->sound.instruments()[instr->index];
  *bank = i.midiBank();
  *patch = i.midiInstrument();
  *isDrum = i.isDrumInstrument() ? 1 : 0;

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_get_instr_patch(std::uint32_t bank, std::uint32_t patch,
                                      int drum, const dlsynth_sound *sound,
                                      dlsynth_instr **instr) {
  if (sound == nullptr || instr == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  const auto &instruments = sound->sound.instruments();
  bool isDrum = drum ? true : false;
  for (std::size_t i = 0; i < instruments.size(); i++) {
    const auto &instrument = instruments[i];
    if (instrument.midiBank() == bank && instrument.midiInstrument() == patch &&
        instrument.isDrumInstrument() == isDrum) {
      *instr = new dlsynth_instr();
      (*instr)->index = i;
      (*instr)->sound = sound;

      return DLSYNTH_NO_ERROR;
    }
  }

  return DLSYNTH_INVALID_INSTR;
}

dlsynth_error dlsynth_get_instr_num(std::size_t instr_num,
                                    const dlsynth_sound *sound,
                                    dlsynth_instr **instr) {
  if (sound == nullptr || instr == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  if (instr_num >= sound->sound.instruments().size()) {
    return DLSYNTH_INVALID_INSTR;
  }

  *instr = new dlsynth_instr();
  (*instr)->index = instr_num;
  (*instr)->sound = sound;

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_instr(dlsynth_instr *instr) {
  if (instr != nullptr) {
    delete instr;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_load_wav_file(const char *path, dlsynth_wav **wav) {
  if (path == nullptr || wav == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  try {
    riffcpp::Chunk chunk(path);
    *wav = new dlsynth_wav{DLSynth::Wave::readChunk(chunk)};

    return DLSYNTH_NO_ERROR;
  } catch (riffcpp::Error &e) {
    return get_err_from_exception(e.type());
  } catch (DLSynth::Error &e) {
    return static_cast<dlsynth_error>(e.code());
  } catch (std::exception &) {
    return DLSYNTH_UNKNOWN_ERROR;
  }
}

dlsynth_error dlsynth_load_wav_buffer(const void *buf, size_t buf_size,
                                      dlsynth_wav **wav) {
  if (buf == nullptr || buf_size == 0 || wav == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  try {
    riffcpp::Chunk chunk(buf, buf_size);
    *wav = new dlsynth_wav{DLSynth::Wave::readChunk(chunk)};

    return DLSYNTH_NO_ERROR;
  } catch (riffcpp::Error &e) {
    return get_err_from_exception(e.type());
  } catch (DLSynth::Error &e) {
    return static_cast<dlsynth_error>(e.code());
  } catch (std::exception &) {
    return DLSYNTH_UNKNOWN_ERROR;
  }
}

dlsynth_error dlsynth_get_wav_info(const dlsynth_wav *wav, int *sample_rate,
                                   size_t *n_frames) {
  if (wav == nullptr || sample_rate == nullptr || n_frames == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *sample_rate = wav->wave.sampleRate();
  *n_frames = wav->wave.leftData().size();

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_get_wav_data(const dlsynth_wav *wav,
                                   const float **left_buf,
                                   const float **right_buf) {
  if (wav == nullptr || left_buf == nullptr || right_buf == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *left_buf = wav->wave.leftData().data();
  *right_buf = wav->wave.rightData().data();

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_wav(dlsynth_wav *wav) {
  if (wav != nullptr) {
    delete wav;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_init(int sample_rate, int num_voices, dlsynth **synth) {
  if (sample_rate <= 0 || num_voices <= 0 || synth == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *synth = new dlsynth();
  (*synth)->synth =
   std::make_unique<DLSynth::Synth::Synthesizer>(num_voices, sample_rate);

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free(dlsynth *synth) {
  if (synth != nullptr) {
    delete synth;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_render_float(dlsynth *synth, size_t frames, float *lout,
                                   float *rout, size_t incr, float gain) {
  if (synth == nullptr || lout == nullptr || incr == 0) {
    return DLSYNTH_INVALID_ARGS;
  }

  float *lbuf = lout;
  float *lbuf_end = lbuf + (frames * incr);

  float *rbuf = rout;
  float *rbuf_end = rbuf + (frames * incr);

  synth->synth->render_fill(lbuf, lbuf_end, rbuf, rbuf_end, incr, gain);

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_render_int16(dlsynth *synth, size_t frames, int16_t *lout,
                                   int16_t *rout, size_t incr, float gain) {
  if (synth == nullptr || lout == nullptr || incr == 0) {
    return DLSYNTH_INVALID_ARGS;
  }

  std::size_t remainingFrames = frames;
  const std::size_t framesInBuffer = dlsynth::renderBufferSize / 2;
  int16_t *l = lout;
  int16_t *r = rout;
  while (remainingFrames > 0) {
    std::size_t num_frames = std::min(remainingFrames, framesInBuffer);
    float *renderBuffer = synth->renderBuffer.data();
    dlsynth_render_float(synth, num_frames, renderBuffer, renderBuffer + 1, 2,
                         gain);

    for (size_t i = 0; i < num_frames; i++) {
      *l = DLSynth::inverse_normalize<std::int16_t>(renderBuffer[i * 2 + 0]);
      l += incr;

      if (r != nullptr) {
        *r = DLSynth::inverse_normalize<std::int16_t>(renderBuffer[i * 2 + 1]);
        r += incr;
      }
    }

    remainingFrames -= num_frames;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_render_float_mix(dlsynth *synth, size_t frames,
                                       float *lout, float *rout, size_t incr,
                                       float gain) {
  if (synth == nullptr || lout == nullptr || incr == 0) {
    return DLSYNTH_INVALID_ARGS;
  }

  float *lbuf = lout;
  float *lbuf_end = lbuf + (frames * incr);

  float *rbuf = rout;
  float *rbuf_end = rbuf + (frames * incr);

  synth->synth->render_mix(lbuf, lbuf_end, rbuf, rbuf_end, incr, gain);

  return DLSYNTH_NO_ERROR;
}

#define DLSYNTH_CHECK_SYNTH_NOT_NULL                                           \
  if (synth == nullptr || synth->synth == nullptr) {                           \
    return DLSYNTH_INVALID_ARGS;                                               \
  }

dlsynth_error dlsynth_note_on(dlsynth *synth, const dlsynth_instr *instr,
                              int channel, int priority, uint8_t note,
                              uint8_t velocity) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->noteOn(instr->sound->sound, instr->index, channel, priority,
                       note, velocity);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_note_off(dlsynth *synth, int channel, uint8_t note) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->noteOff(channel, note);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_poly_pressure(dlsynth *synth, int channel, uint8_t note,
                                    uint8_t velocity) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->pressure(channel, note, velocity);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_channel_pressure(dlsynth *synth, int channel,
                                       uint8_t velocity) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->pressure(channel, velocity);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_pitch_bend(dlsynth *synth, int channel, uint16_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->pitchBend(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_volume(dlsynth *synth, int channel, uint8_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->volume(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_pan(dlsynth *synth, int channel, uint8_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->pan(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_modulation(dlsynth *synth, int channel, uint8_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->modulation(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_sustain(dlsynth *synth, int channel, int status) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->sustain(channel, status);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_reverb(dlsynth *synth, int channel, uint8_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->reverb(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_chorus(dlsynth *synth, int channel, uint8_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->chorus(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_pitch_bend_range(dlsynth *synth, int channel,
                                       uint16_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->pitchBendRange(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_fine_tuning(dlsynth *synth, int channel, uint16_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->fineTuning(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_coarse_tuning(dlsynth *synth, int channel,
                                    uint16_t value) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->coarseTuning(channel, value);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_reset_controllers(dlsynth *synth, int channel) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->resetControllers(channel);
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_all_notes_off(dlsynth *synth) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->allNotesOff();
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_all_sound_off(dlsynth *synth) {
  DLSYNTH_CHECK_SYNTH_NOT_NULL

  synth->synth->allSoundOff();
  return DLSYNTH_NO_ERROR;
}

struct dlsynth_wavesample {
  DLSynth::Wavesample wavesample;
};

dlsynth_error dlsynth_new_wavesample_oneshot(uint16_t unityNote,
                                             int16_t fineTune, int32_t gain,
                                             dlsynth_wavesample **wavesample) {
  if (wavesample == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }
  *wavesample =
   new dlsynth_wavesample{DLSynth::Wavesample(unityNote, fineTune, gain)};
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_new_wavesample_looped(uint16_t unityNote,
                                            int16_t fineTune, int32_t gain,
                                            enum dlsynth_loop_type type,
                                            uint32_t loopStart,
                                            uint32_t loopLength,
                                            dlsynth_wavesample **wavesample) {
  if (wavesample == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }
  *wavesample = new dlsynth_wavesample{DLSynth::Wavesample(
   unityNote, fineTune, gain,
   DLSynth::WavesampleLoop(static_cast<DLSynth::LoopType>(type), loopStart,
                           loopLength))};
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_free_wavesample(dlsynth_wavesample *wavesample) {
  if (wavesample != nullptr) {
    delete wavesample;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_new_wav_mono(int sampleRate, const float *dataBegin,
                                   const float *dataEnd,
                                   const dlsynth_wavesample *wavesample,
                                   dlsynth_wav **wav) {
  if (wav == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  std::vector<float> data(dataBegin, dataEnd);

  std::size_t dataSize = data.size();
  if (wavesample != nullptr) {
    if (wavesample->wavesample.loop() != nullptr) {
      auto loop = wavesample->wavesample.loop();
      if (loop->start() > dataSize ||
          loop->start() + loop->length() > dataSize) {
        return DLSYNTH_INVALID_ARGS;
      }
    }
    *wav = new dlsynth_wav{DLSynth::Wave(
     data, data, sampleRate, &wavesample->wavesample, nullptr, nullptr)};
    return DLSYNTH_NO_ERROR;
  } else {
    *wav = new dlsynth_wav{DLSynth::Wave(
     data, data, sampleRate, &wavesample->wavesample, nullptr, nullptr)};
    return DLSYNTH_NO_ERROR;
  }
}
dlsynth_error dlsynth_new_wav_stereo(int sampleRate, const float *leftDataBegin,
                                     const float *leftDataEnd,
                                     const float *rightDataBegin,
                                     const float *rightDataEnd,
                                     const dlsynth_wavesample *wavesample,
                                     dlsynth_wav **wav) {
  if (wav == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  std::vector<float> ldata(leftDataBegin, leftDataEnd);
  std::vector<float> rdata(rightDataBegin, rightDataEnd);

  if (ldata.size() != rdata.size()) {
    return DLSYNTH_INVALID_ARGS;
  }

  std::size_t dataSize = ldata.size();
  if (wavesample != nullptr) {
    if (wavesample->wavesample.loop() != nullptr) {
      auto loop = wavesample->wavesample.loop();
      if (loop->start() > dataSize ||
          loop->start() + loop->length() > dataSize) {
        return DLSYNTH_INVALID_ARGS;
      }
    }
    *wav = new dlsynth_wav{DLSynth::Wave(
     ldata, rdata, sampleRate, &wavesample->wavesample, nullptr, nullptr)};
    return DLSYNTH_NO_ERROR;
  } else {
    *wav = new dlsynth_wav{DLSynth::Wave(
     ldata, rdata, sampleRate, &wavesample->wavesample, nullptr, nullptr)};
    return DLSYNTH_NO_ERROR;
  }
}

struct dlsynth_wavepool {
  std::vector<DLSynth::Wave> waves;
};

dlsynth_error dlsynth_new_wavepool(dlsynth_wavepool **wavepool) {
  if (wavepool == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *wavepool = new dlsynth_wavepool();
  return DLSYNTH_NO_ERROR;
}
dlsynth_error dlsynth_wavepool_add(dlsynth_wavepool *wavepool,
                                   const dlsynth_wav *wav) {
  if (wavepool == nullptr || wav == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  wavepool->waves.push_back(wav->wave);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_wavepool(dlsynth_wavepool *wavepool) {
  if (wavepool != nullptr) {
    delete wavepool;
  }

  return DLSYNTH_NO_ERROR;
}

struct dlsynth_blocklist {
  std::vector<DLSynth::ConnectionBlock> blocks;
};

dlsynth_error dlsynth_new_blocklist(dlsynth_blocklist **blocklist) {
  if (blocklist == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *blocklist = new dlsynth_blocklist();
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_blocklist_add(
 dlsynth_blocklist *blocklist, enum dlsynth_source source,
 enum dlsynth_source control, enum dlsynth_dest destination, int32_t scale,
 int sourceInvert, int sourceBipolar, enum dlsynth_transf sourceTransform,
 int controlInvert, int controlBipolar, enum dlsynth_transf controlTransform) {
  if (blocklist == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  blocklist->blocks.emplace_back(
   static_cast<DLSynth::Source>(source), static_cast<DLSynth::Source>(control),
   static_cast<DLSynth::Destination>(destination), scale,
   DLSynth::TransformParams(
    sourceInvert, sourceBipolar,
    static_cast<DLSynth::TransformType>(sourceTransform)),
   DLSynth::TransformParams(
    controlInvert, controlBipolar,
    static_cast<DLSynth::TransformType>(controlTransform)));

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_blocklist(dlsynth_blocklist *blocklist) {
  if (blocklist != nullptr) {
    delete blocklist;
  }

  return DLSYNTH_NO_ERROR;
}

struct dlsynth_regionlist {
  std::vector<DLSynth::Region> regions;
};

dlsynth_error dlsynth_new_regionlist(dlsynth_regionlist **regionlist) {
  if (regionlist == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *regionlist = new dlsynth_regionlist();
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_add_region(dlsynth_regionlist *list, uint16_t minKey,
                                 uint16_t maxKey, uint16_t minVelocity,
                                 uint16_t maxVelocity,
                                 const dlsynth_blocklist *blocklist,
                                 uint32_t waveIndex, int selfNonExclusive) {
  if (list == nullptr || minKey > maxKey || minVelocity > maxVelocity ||
      blocklist == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  list->regions.emplace_back(Range{minKey, maxKey},
                             Range{minVelocity, maxVelocity}, blocklist->blocks,
                             waveIndex, selfNonExclusive);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_add_region_wavesample(
 dlsynth_regionlist *list, uint16_t minKey, uint16_t maxKey,
 uint16_t minVelocity, uint16_t maxVelocity, const dlsynth_blocklist *blocklist,
 uint32_t waveIndex, int selfNonExclusive,
 const dlsynth_wavesample *wavesample) {
  if (list == nullptr || minKey > maxKey || minVelocity > maxVelocity ||
      blocklist == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  list->regions.emplace_back(
   Range{minKey, maxKey}, Range{minVelocity, maxVelocity}, blocklist->blocks,
   waveIndex, selfNonExclusive, wavesample->wavesample);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_regionlist(dlsynth_regionlist *list) {
  if (list != nullptr) {
    delete list;
  }

  return DLSYNTH_NO_ERROR;
}

struct dlsynth_instrlist {
  std::vector<DLSynth::Instrument> instruments;
};

dlsynth_error dlsynth_new_instrlist(dlsynth_instrlist **list) {
  if (list == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  *list = new dlsynth_instrlist();
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_add_instrument(dlsynth_instrlist *list, uint32_t midiBank,
                                     uint32_t midiInstrument,
                                     int isDrumInstrument,
                                     const dlsynth_blocklist *blocklist,
                                     const dlsynth_regionlist *regions) {
  if (list == nullptr || blocklist == nullptr || regions == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  list->instruments.emplace_back(midiBank, midiInstrument, isDrumInstrument,
                                 blocklist->blocks, regions->regions);
  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_free_instrlist(dlsynth_instrlist *list) {
  if (list != nullptr) {
    delete list;
  }

  return DLSYNTH_NO_ERROR;
}

dlsynth_error dlsynth_new_sound(dlsynth_sound **sound,
                                const dlsynth_instrlist *instruments,
                                const dlsynth_wavepool *wavepool) {
  if (sound == nullptr || instruments == nullptr || wavepool == nullptr) {
    return DLSYNTH_INVALID_ARGS;
  }

  auto wavepool_size = wavepool->waves.size();

  for (const auto &instr : instruments->instruments) {
    for (const auto &region : instr.regions()) {
      if (region.waveIndex() >= wavepool_size) {
        return DLSYNTH_INVALID_WAVE_INDEX;
      }

      auto wsmpl = region.wavesample();
      const auto &wave = wavepool->waves[region.waveIndex()];
      if (!wsmpl) {
        wsmpl = wave.wavesample();
      }

      if (!wsmpl) {
        return DLSYNTH_NO_WAVESAMPLE;
      }

      const DLSynth::WavesampleLoop *loop = wsmpl->loop();
      if (loop && (loop->start() >= wave.leftData().size() ||
                   loop->start() + loop->length() >= wave.leftData().size())) {
        return DLSYNTH_INVALID_WAVESAMPLE;
      }
    }
  }

  *sound =
   new dlsynth_sound{DLSynth::Sound(instruments->instruments, wavepool->waves)};
  return DLSYNTH_NO_ERROR;
}

#ifdef DLSYNTH_COMMIT
static dlsynth_version version = {DLSYNTH_MAJOR, DLSYNTH_MINOR, DLSYNTH_PATCH,
                                  DLSYNTH_COMMIT};
#else
static dlsynth_version version = {DLSYNTH_MAJOR, DLSYNTH_MINOR, DLSYNTH_PATCH,
                                  ""};
#endif

const dlsynth_version *dlsynth_get_version() { return &version; }