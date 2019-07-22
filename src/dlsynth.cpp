#include "Error.hpp"
#include "Sound.hpp"
#include "Wave.hpp"
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

struct dlsynth {};

struct dlsynth_sound {
  std::unique_ptr<DLSynth::Sound> sound;
};
struct dlsynth_instr {};

struct dlsynth_wav {
  std::unique_ptr<DLSynth::Wave> wave;
};
int dlsynth_load_sound_file(const char *path, uint32_t sampleRate,
                            struct dlsynth_sound **sound) {
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
                           uint32_t sampleRate, struct dlsynth_sound **sound) {
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

int dlsynth_free_sound(struct dlsynth_sound *sound) {
  if (sound != nullptr) {
    delete sound;
  }

  return 1;
}

int dlsynth_load_wav_file(const char *path, struct dlsynth_wav **wav) {
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
                            struct dlsynth_wav **wav) {
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

int dlsynth_get_wav_info(const struct dlsynth_wav *wav, int *sample_rate,
                         size_t *n_frames) {
  if (wav == nullptr || sample_rate == nullptr || n_frames == nullptr) {
    dlsynth_error = DLSYNTH_INVALID_ARGS;
    return 0;
  }

  *sample_rate = wav->wave->sampleRate();
  *n_frames = wav->wave->leftData().size();

  return 1;
}

int dlsynth_get_wav_data(const struct dlsynth_wav *wav, const float **left_buf,
                         const float **right_buf) {
  if (wav == nullptr || left_buf == nullptr || right_buf == nullptr) {
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