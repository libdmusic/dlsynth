#ifndef DLSYNTH_H
#define DLSYNTH_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dlsynth_export.h"
#include <stddef.h>
#include <stdint.h>
/// A DLS synthesizer
struct dlsynth;

/// A collection of DLS instruments
struct dlsynth_sound;

/// A DLS instrument
struct dlsynth_instr;

/// A WAV file
struct dlsynth_wav;

enum dlsynth_interleave { DLSYNTH_INTERLEAVED, DLSYNTH_SEQUENTIAL };

/// Settings that should be used when rendering audio
struct dlsynth_settings {
  /// Sample rate at which to render the audio
  uint32_t sample_rate;

  /// Number of audio channels to render
  int num_channels;

  /// Wheter the audio channels should be interleaved or sequential
  enum dlsynth_interleave interleaved;

  /// Maximum number of voices to allocate for rendering
  unsigned num_voices;

  /// The instrument to use for rendering
  const struct dlsynth_instr *instrument;
};

/**
 * @defgroup SynthManagement Synthesizer management
 *
 */

/**
 * @defgroup SoundManagement Sound collection management
 *
 */

/**
 * @defgroup InstrManagement Instrument management
 *
 */

/**
 * @defgroup Rendering Sound rendering
 *
 */
/// Initializes a DLS synthesizer
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_init(
 const struct dlsynth_settings
  *settings,            ///< [in] Settings to use when rendering audio
 struct dlsynth **synth ///< [out] The initialized synthesizer
);

/// Frees the resources allocated for a synthesizer
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free(
 struct dlsynth *synth ///< [in] The synthesizer to free
);

/// Renders audio in 32-bit float format
/**
 * @ingroup Rendering
 * Data in the buffer passed as a parameter will be overwritten.
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_render_float(
 struct dlsynth *synth, ///< [in] The synthesizer to use for rendering
 float *buffer,         ///< [in,out] The buffer to execute rendering in
 size_t frames,         ///< [in] The number of frames to render
 float gain ///< [in] Gain to apply while rendering (0 means silent, 1 means
            ///< unity gain)
);

/// Renders audio in 16-bit integer format
/**
 * @ingroup Rendering
 * Data in the buffer passed as a parameter will be overwritten.
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_render_int16(
 struct dlsynth *synth, ///< [in] The synthesizer to use for rendering
 int16_t *buffer,       ///< [in,out] The buffer to execute rendering in
 size_t frames,         ///< [in] The number of frames to render
 float gain ///< [in] Gain to apply while rendering (0 means silent, 1 means
            ///< unity gain)
);

/// Renders audio in 32-bit float format
/**
 * @ingroup Rendering
 * New audio data will be mixed on top of the existing data in the buffer.
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_render_float_mix(
 struct dlsynth *synth, ///< [in] The synthesizer to use for rendering
 float *buffer,         ///< [in,out] The buffer to execute rendering in
 size_t frames,         ///< [in] The number of frames to render
 float gain ///< [in] Gain to apply while rendering (0 means silent, 1 means
            ///< unity gain)
);

/// Sends a MIDI note on event to a synthesizer
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_note_on(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 uint8_t note,          ///< [in] The note which the event refers to
 uint8_t velocity       ///< [in] Velocity of the note
);

/// Sends a MIDI note off event to a synthesizer
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_note_off(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 uint8_t note           ///< [in] The note which the event refers to
);

/// Sends a MIDI polyphonic pressure (aftertouch) event
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_poly_pressure(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 uint8_t note,          ///< [in] The note which the event refers to
 uint8_t value          ///< [in] The pressure value of the event
);

/// Sends a MIDI channel pressure (aftertouch) event
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_channel_pressure(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 uint8_t value          ///< [in] The pressure value of the event
);

/// Sends a MIDI pitch bend event
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_pitch_bend(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 uint16_t value         ///< [in] Value of the pitch bend event
);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_volume(struct dlsynth *synth, uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_pan(struct dlsynth *synth, uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_modulation(struct dlsynth *synth, uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_sustain(struct dlsynth *synth, int status);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_reverb(struct dlsynth *synth, uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_chorus(struct dlsynth *synth, uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_pitch_bend_range(struct dlsynth *synth,
                                            uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_fine_tuning(struct dlsynth *synth, uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_coarse_tuning(struct dlsynth *synth, uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_reset_controllers(struct dlsynth *synth);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_all_notes_off(struct dlsynth *synth);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_all_sound_off(struct dlsynth *synth);

/// Loads a DLS file from a path
/**
 * @ingroup SoundManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_load_sound_file(
 const char *path,            ///< [in] Path of the file to load
 uint32_t sampleRate,         ///< [in] Sampling rate that will
                              ///< be used during reproduction
 struct dlsynth_sound **sound ///< [out] The loaded file
);

/// Loads a DLS file from a memory buffer
/**
 * @ingroup SoundManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_load_sound_buf(
 const void *buf, ///< [in] The buffer from which to load the file
 size_t buf_size, ///< [in] Size of the buffer in bytes
 uint32_t
  sampleRate, ///< [in] Sampling rate that will be used during reproduction
 struct dlsynth_sound **sound ///< [out] The loaded file
);

/// Frees the resources allocated for a DLS file
/**
 * @ingroup SoundManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_sound(struct dlsynth_sound *sound);

/// Obtains the number of instruments in this sound
/**
 * @ingroup SoundManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_sound_instr_count(
 const struct dlsynth_sound
  *sound,      ///< [in] The sound of which to obtain the number of instruments
 size_t *count ///< [out] The number of instruments contained in the sound
);

/// Obtains info about an instrument contained in a sound collection
/**
 * @ingroup InstrManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_sound_instr_info(
 const struct dlsynth_instr
  *instr,        ///< [in] The instrument of which to obtain info
 uint32_t *bank, ///< [out] MIDI bank of the instrument
 uint32_t *patch ///< [out] MIDI Program Change of the instrument
);

/// Obtains a reference to a DLS instrument from its MIDI parameters
/**
 * @ingroup InstrManagement
 * @remarks Access to an instrument after the containing \ref dlsynth_sound has
 * been free'd is undefined and should not be attempted.
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_get_instr_patch(
 uint32_t bank,  ///< [in] MIDI bank of the instrument
 uint32_t patch, ///< [in] MIDI Program Change of the instrument
 const struct dlsynth_sound
  *sound, ///< [in] DLS sound where the instrument is contained
 struct dlsynth_instr **instr ///< [out] The instrument
);

/// Obtains a reference to a DLS instrument from its index
/**
 * @ingroup InstrManagement
 * @remarks Access to an instrument after the containing \ref dlsynth_sound has
 * been free'd is undefined and should not be attempted.
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_get_instr_num(
 size_t instr_num, ///< [in] Index of the instrument
 const struct dlsynth_sound
  *sound, ///< [in] DLS sound where the instrument is contained
 struct dlsynth_instr **instr ///< [out] The instrument
);

/// Frees a reference to a DLS instrument
/**
 * @ingroup InstrManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_instr(struct dlsynth_instr *instr);

/// Loads a WAV file for decoding
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_load_wav_file(
 const char *path,        ///< [in] Path of the file to load
 struct dlsynth_wav **wav ///< [out] The loaded file
);

/// Loads a WAV file for decoding from a buffer
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_load_wav_buffer(
 const void *buf,         ///< [in] The buffer from which to load the file
 size_t buf_size,         ///< [in] Size of the buffer in bytes
 struct dlsynth_wav **wav ///< [out] The loaded file
);

/// Obtains info about a loaded WAV file
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_get_wav_info(
 const struct dlsynth_wav *wav, ///< [in] The file of which to obtain info
 int *sample_rate,              ///< [out] Sample rate of the file
 size_t *n_frames               ///< [out] Number of frames in the file
);

/// Returns the decoded audio data of a WAV file
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_get_wav_data(
 const struct dlsynth_wav *wav, ///< [in] The file to decode
 const float **left_buf,        ///< [out] The left channel data
 const float **right_buf        ///< [out] The right channel data
);

/// Frees the resources allocated for a WAV file
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_wav(struct dlsynth_wav *wav);

enum dlsynth_error {
#define DLSYNTH_ERROR(name, value) DLSYNTH_##name = value,
#include "dlsynth_errors.h"
#undef DLSYNTH_ERROR
};

/// Returns the last error generated by a function call
int DLSYNTH_EXPORT dlsynth_get_last_error(void);

/// Resets the last generated error to \ref DLSYNTH_NO_ERROR
void DLSYNTH_EXPORT dlsynth_reset_last_error(void);

#ifdef __cplusplus
}
#endif
#endif