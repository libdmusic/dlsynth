#include "Error.hpp"
#include "Instrument.hpp"
#include "NumericUtils.hpp"
#include "Sound.hpp"
#include "Synth/Synthesizer.hpp"
#include "Wave.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <dlsynth.h>
#include <limits>
#include <memory>
#include <riffcpp.hpp>
static std::atomic<int> dlsynth_error{DLSYNTH_NO_ERROR};

int dlsynth_get_last_error() { return dlsynth_error; }
void dlsynth_reset_last_error() { dlsynth_error = DLSYNTH_NO_ERROR; }

static void set_err_from_exception(riffcpp::ErrorType type) {
  switch (type) {
  case riffcpp::ErrorType::CannotOpenFile:
    dlsynth_error = DLSYNTH_CANNOT_OPEN_FILE;
    break;
  case riffcpp::ErrorType::InvalidFile:
    dlsynth_error = DLSYNTH_INVALID_FILE;
    break;
  case riffcpp::ErrorType::NullBuffer:
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    break;
  default:
    dlsynth_error = DLSYNTH_UNKNOWN_ERROR;
    break;
  }
}

struct dlsynth {
  static constexpr std::size_t renderBufferSizeFrames = 1024;
  static constexpr std::size_t renderBufferSize = renderBufferSizeFrames * 2;

  std::unique_ptr<DLSynth::Synth::Synthesizer> synth;
  int num_channels;
  dlsynth_interleave interleaved;

  std::array<float, renderBufferSize> renderBuffer;

  ~dlsynth() { synth = nullptr; }
};

struct dlsynth_sound {
  std::unique_ptr<DLSynth::Sound> sound;

  ~dlsynth_sound() { sound = nullptr; }
};
struct dlsynth_instr {
  const dlsynth_sound *sound;
  std::size_t index;
};

struct dlsynth_wav {
  std::unique_ptr<DLSynth::Wave> wave;

  ~dlsynth_wav() { wave = nullptr; }
};
int dlsynth_load_sound_file(const char *path, uint32_t sampleRate,
                            dlsynth_sound **sound) {
  if (sound == nullptr || path == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  try {
    riffcpp::Chunk chunk(path);
    *sound = new dlsynth_sound();
    (*sound)->sound = std::make_unique<DLSynth::Sound>(chunk, sampleRate);

    return 1;
  } catch (riffcpp::Error &e) {
    set_err_from_exception(e.type());
    return 0;
  } catch (DLSynth::Error &e) {
    dlsynth_error = static_cast<int>(e.code());
    return 0;
  } catch (std::exception &) {
    dlsynth_error = DLSYNTH_UNKNOWN_ERROR;
    return 0;
  }
}

int dlsynth_load_sound_buf(const void *buf, size_t buf_size,
                           uint32_t sampleRate, dlsynth_sound **sound) {
  if (sound == nullptr || buf == nullptr || buf_size == 0) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  try {
    riffcpp::Chunk chunk(buf, buf_size);
    *sound = new dlsynth_sound();
    (*sound)->sound = std::make_unique<DLSynth::Sound>(chunk, sampleRate);

    return 1;
  } catch (riffcpp::Error &e) {
    set_err_from_exception(e.type());
    return 0;
  } catch (DLSynth::Error &e) {
    dlsynth_error = static_cast<int>(e.code());
    return 0;
  } catch (std::exception &) {
    dlsynth_error = DLSYNTH_UNKNOWN_ERROR;
    return 0;
  }
}

int dlsynth_free_sound(dlsynth_sound *sound) {
  if (sound != nullptr) {
    delete sound;
  }

  return 1;
}

int dlsynth_sound_instr_count(const dlsynth_sound *sound, std::size_t *count) {
  if (sound == nullptr || count == nullptr || sound->sound == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  *count = sound->sound->instruments().size();
  return 1;
}

int dlsynth_sound_instr_info(const dlsynth_instr *instr, uint32_t *bank,
                             uint32_t *patch) {
  if (instr == nullptr || bank == nullptr || patch == nullptr ||
      instr->sound == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  const auto &i = instr->sound->sound->instruments()[instr->index];
  *bank = i.midiBank();
  *patch = i.midiInstrument();

  return 1;
}

int dlsynth_get_instr_patch(std::uint32_t bank, std::uint32_t patch,
                            const dlsynth_sound *sound, dlsynth_instr **instr) {
  if (sound == nullptr || instr == nullptr || sound->sound == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  const auto &instruments = sound->sound->instruments();
  for (std::size_t i = 0; i < instruments.size(); i++) {
    const auto &instrument = instruments[i];
    if (instrument.midiBank() == bank && instrument.midiInstrument() == patch) {
      *instr = new dlsynth_instr();
      (*instr)->index = i;
      (*instr)->sound = sound;

      return 1;
    }
  }

  dlsynth_error = DLSYNTH_INVALID_INSTR;
  return 0;
}

int dlsynth_get_instr_num(std::size_t instr_num, const dlsynth_sound *sound,
                          dlsynth_instr **instr) {
  if (sound == nullptr || instr == nullptr || sound->sound == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  if (instr_num >= sound->sound->instruments().size()) {
    dlsynth_error = DLSYNTH_INVALID_INSTR;
    return 0;
  }

  *instr = new dlsynth_instr();
  (*instr)->index = instr_num;
  (*instr)->sound = sound;

  return 1;
}

int dlsynth_free_instr(dlsynth_instr *instr) {
  if (instr != nullptr) {
    delete instr;
  }

  return 1;
}

int dlsynth_load_wav_file(const char *path, dlsynth_wav **wav) {
  if (path == nullptr || wav == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  try {
    riffcpp::Chunk chunk(path);
    *wav = new dlsynth_wav();
    (*wav)->wave = std::make_unique<DLSynth::Wave>(chunk);

    return 1;
  } catch (riffcpp::Error &e) {
    set_err_from_exception(e.type());
    return 0;
  } catch (DLSynth::Error &e) {
    dlsynth_error = static_cast<int>(e.code());
    return 0;
  } catch (std::exception &) {
    dlsynth_error = DLSYNTH_UNKNOWN_ERROR;
    return 0;
  }
}

int dlsynth_load_wav_buffer(const void *buf, size_t buf_size,
                            dlsynth_wav **wav) {
  if (buf == nullptr || buf_size == 0 || wav == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  try {
    riffcpp::Chunk chunk(buf, buf_size);
    *wav = new dlsynth_wav();
    (*wav)->wave = std::make_unique<DLSynth::Wave>(chunk);

    return 1;
  } catch (riffcpp::Error &e) {
    set_err_from_exception(e.type());
    return 0;
  } catch (DLSynth::Error &e) {
    dlsynth_error = static_cast<int>(e.code());
    return 0;
  } catch (std::exception &) {
    dlsynth_error = DLSYNTH_UNKNOWN_ERROR;
    return 0;
  }
}

int dlsynth_get_wav_info(const dlsynth_wav *wav, int *sample_rate,
                         size_t *n_frames) {
  if (wav == nullptr || sample_rate == nullptr || n_frames == nullptr ||
      wav->wave == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  *sample_rate = wav->wave->sampleRate();
  *n_frames = wav->wave->leftData().size();

  return 1;
}

int dlsynth_get_wav_data(const dlsynth_wav *wav, const float **left_buf,
                         const float **right_buf) {
  if (wav == nullptr || left_buf == nullptr || right_buf == nullptr ||
      wav->wave == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  *left_buf = wav->wave->leftData().data();
  *right_buf = wav->wave->rightData().data();

  return 1;
}

int dlsynth_free_wav(struct dlsynth_wav *wav) {
  if (wav != nullptr) {
    delete wav;
  }

  return 1;
}

int dlsynth_init(const dlsynth_settings *settings, dlsynth **synth) {
  if (settings == nullptr || settings->instrument == nullptr ||
      settings->num_channels == 0 || settings->num_channels > 2 ||
      settings->sample_rate == 0 || synth == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  const DLSynth::Sound &sound = *(settings->instrument->sound->sound);

  *synth = new dlsynth();
  (*synth)->synth = std::make_unique<DLSynth::Synth::Synthesizer>(
   sound, settings->instrument->index, settings->num_voices,
   settings->sample_rate);

  (*synth)->interleaved = settings->interleaved;
  (*synth)->num_channels = settings->num_channels;

  return 1;
}

int dlsynth_free(dlsynth *synth) {
  if (synth != nullptr) {
    delete synth;
  }

  return 1;
}

int dlsynth_render_float(dlsynth *synth, float *buffer, size_t frames,
                         float gain) {
  if (synth == nullptr || buffer == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  if (synth->num_channels == 2) {
    if (synth->interleaved == DLSYNTH_INTERLEAVED) {
      float *lbuf = buffer;
      float *lbuf_end = lbuf + (frames * 2);
      float *rbuf = lbuf + 1;
      float *rbuf_end = lbuf_end + 1;

      synth->synth->render_fill(lbuf, lbuf_end, rbuf, rbuf_end, 2, gain);
    } else {
      float *lbuf = buffer;
      float *rbuf = buffer + frames;
      float *rbuf_end = rbuf + frames;
      synth->synth->render_fill(lbuf, rbuf, rbuf, rbuf_end, 1, gain);
    }
  } else {
    synth->synth->render_fill(buffer, buffer + frames, nullptr, nullptr, 1,
                              gain);
  }

  return 1;
}

int DLSYNTH_EXPORT dlsynth_render_int16(struct dlsynth *synth, int16_t *buffer,
                                        size_t frames, float gain) {
  if (synth == nullptr || buffer == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  std::size_t remainingFrames = frames;
  const std::size_t framesInBuffer =
   dlsynth::renderBufferSize / synth->num_channels;
  int16_t *target = buffer;
  while (remainingFrames > 0) {
    std::size_t num_frames = std::min(remainingFrames, framesInBuffer);
    dlsynth_render_float(synth, synth->renderBuffer.data(), num_frames, gain);
    std::transform(
     std::begin(synth->renderBuffer), std::end(synth->renderBuffer), target,
     [](auto x) { return DLSynth::inverse_normalize<int16_t>(x); });
    target += num_frames * synth->num_channels;
    remainingFrames -= num_frames;
  }

  return 1;
}

int dlsynth_render_float_mix(dlsynth *synth, float *buffer, size_t frames,
                             float gain) {
  if (synth == nullptr || buffer == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  if (synth->num_channels == 2) {
    if (synth->interleaved == DLSYNTH_INTERLEAVED) {
      float *lbuf = buffer;
      float *lbuf_end = lbuf + (frames * 2);
      float *rbuf = lbuf + 1;
      float *rbuf_end = lbuf_end + 1;

      synth->synth->render_mix(lbuf, lbuf_end, rbuf, rbuf_end, 2, gain);
    } else {
      float *lbuf = buffer;
      float *rbuf = buffer + frames;
      float *rbuf_end = rbuf + frames;
      synth->synth->render_mix(lbuf, rbuf, rbuf, rbuf_end, 1, gain);
    }
  } else {
    synth->synth->render_mix(buffer, buffer + frames, nullptr, nullptr, 1,
                             gain);
  }

  return 1;
}

int dlsynth_note_on(struct dlsynth *synth, uint8_t note, uint8_t velocity) {
  if (synth == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  synth->synth->noteOn(note, velocity);
  return 1;
}
int dlsynth_note_off(struct dlsynth *synth, uint8_t note) {
  if (synth == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  synth->synth->noteOff(note);
  return 1;
}