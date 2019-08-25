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
 * @defgroup SoundCreation Sound collection creation
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
 int sample_rate, int num_voices,
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
 size_t nframes,        ///< [in] Number of frames to render
 float *lout,           ///< [in,out] Buffer to store left channel data in
 float *rout,           ///< [in,out] Buffer to store right channel data in
 size_t incr,           ///< [in] Increment between sample positions
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
 size_t nframes,        ///< [in] Number of frames to render
 int16_t *lout,         ///< [in,out] Buffer to store left channel data in
 int16_t *rout,         ///< [in,out] Buffer to store right channel data in
 size_t incr,           ///< [in] Increment between sample positions
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
 size_t nframes,        ///< [in] Number of frames to render
 float *lout,           ///< [in,out] Buffer to store left channel data in
 float *rout,           ///< [in,out] Buffer to store right channel data in
 size_t incr,           ///< [in] Increment between sample positions
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
 const struct dlsynth_instr
  *instr,         ///< [in] Instrument to use for rendering the note
 int channel,     ///< [in] Channel to associate the note to
 uint8_t note,    ///< [in] The note which the event refers to
 uint8_t velocity ///< [in] Velocity of the note
);

/// Sends a MIDI note off event to a synthesizer
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_note_off(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 int channel,           ///< [in] The channel which the event refers to
 uint8_t note           ///< [in] The note which the event refers to
);

/// Sends a MIDI polyphonic pressure (aftertouch) event
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_poly_pressure(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 int channel,           ///< [in] The channel which the event refers to
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
 int channel,           ///< [in] The channel which the event refers to
 uint8_t value          ///< [in] The pressure value of the event
);

/// Sends a MIDI pitch bend event
/**
 * @ingroup SynthManagement
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_pitch_bend(
 struct dlsynth *synth, ///< [in] The synthesizer to send the message to
 int channel,           ///< [in] The channel which the event refers to
 uint16_t value         ///< [in] Value of the pitch bend event
);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_volume(struct dlsynth *synth, int channel,
                                  uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_pan(struct dlsynth *synth, int channel,
                               uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_modulation(struct dlsynth *synth, int channel,
                                      uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_sustain(struct dlsynth *synth, int channel,
                                   int status);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_reverb(struct dlsynth *synth, int channel,
                                  uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_chorus(struct dlsynth *synth, int channel,
                                  uint8_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_pitch_bend_range(struct dlsynth *synth, int channel,
                                            uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_fine_tuning(struct dlsynth *synth, int channel,
                                       uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_coarse_tuning(struct dlsynth *synth, int channel,
                                         uint16_t value);

/**
 * @ingroup SynthManagement
 */
int DLSYNTH_EXPORT dlsynth_reset_controllers(struct dlsynth *synth,
                                             int channel);

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
/**
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_get_last_error(void);

/// Resets the last generated error to DLSYNTH_NO_ERROR
/**
 * @return Nonzero on success, zero on failure
 */
void DLSYNTH_EXPORT dlsynth_reset_last_error(void);

/// An object describing the parameters to be used when playing a sample
/**
 * @ingroup SoundCreation
 */
struct dlsynth_wavesample;

/// Type of loop for a wavesample
/**
 * @ingroup SoundCreation
 */
enum dlsynth_loop_type {
  DLSYNTH_LOOP_FORWARD = 0, ///< Standard loop type
  DLSYNTH_LOOP_RELEASE = 1 ///< When a note is released, the player does not run
                           ///< back to the start of the loop
};

/// Creates a new one-shot wavesample
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_wavesample_oneshot(
 uint16_t unityNote, ///< [in] The natural MIDI note pitch for this sample
 int16_t fineTune,   ///< [in] Fine tune value of the sample in 16-bit relative
                     ///< pitch units
 int32_t
  gain, ///< [in] Gain to apply to the sample in 32-bit relative gain units
 struct dlsynth_wavesample **wavesample ///< [out] The created wavesample
);

/// Creates a new looped wavesample
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_wavesample_looped(
 uint16_t unityNote, ///< [in] The natural MIDI note pitch for this sample
 int16_t fineTune,   ///< [in] Fine tune value of the sample in 16-bit relative
                     ///< pitch units
 int32_t
  gain, ///< [in] Gain to apply to the sample in 32-bit relative gain units
 enum dlsynth_loop_type type,           ///< [in] Type of loop
 uint32_t loopStart,                    ///< [in] Sample position of loop start
 uint32_t loopLength,                   ///< [in] Number of samples in loop
 struct dlsynth_wavesample **wavesample ///< [out] The created wavesample
);

/// Frees the resources associated with a wavesample
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT
dlsynth_free_wavesample(struct dlsynth_wavesample *wavesample);

/// Creates a new mono WAV
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_wav_mono(
 int sampleRate,         ///< [in] Sample rate of the WAV in samples per second
 const float *dataBegin, ///< [in] Pointer to the beginning of audio data
 const float *dataEnd,   ///< [in] Pointer to the end of audio data
 const struct dlsynth_wavesample
  *wavesample,            ///< [in] Wavesample associated to the WAV
 struct dlsynth_wav **wav ///< [out] The created WAV
);

/// Creates a new stereo WAV
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_wav_stereo(
 int sampleRate, ///< [in] Sample rate of the WAV in samples per second
 const float
  *leftDataBegin, ///< [in] Pointer to the beginning of left channel audio data
 const float
  *leftDataEnd, ///< [in] Pointer to the end of left channel audio data
 const float *
  rightDataBegin, ///< [in] Pointer to the beginning of right channel audio data
 const float
  *rightDataEnd, ///< [in] Pointer to the end of right channel audio data
 const struct dlsynth_wavesample
  *wavesample,            ///< [in] Wavesample associated to the WAV
 struct dlsynth_wav **wav ///< [out] The created WAV
);

/// A list of WAVs
/**
 * @ingroup SoundCreation
 */
struct dlsynth_wavepool;

/// Creates a new empty wavepool
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_wavepool(
 struct dlsynth_wavepool **wavepool ///< [out] The new wavepool
);

/// Adds a WAV to a wavepool
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_wavepool_add(
 struct dlsynth_wavepool
  *wavepool, ///< [in] The wavepool to which the WAV will be added
 const struct dlsynth_wav *wav ///< [in] The WAV to add
);

/// Frees the resources associated with a wavepool
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_wavepool(
 struct dlsynth_wavepool *wavepool ///< [in] The wavepool to free
);

/// A list of connection blocks
/**
 * @ingroup SoundCreation
 */
struct dlsynth_blocklist;

/**
 * @ingroup SoundCreation
 */
enum dlsynth_source {
  DLSYNTH_SRC_NONE = 0x0000,
  DLSYNTH_SRC_LFO = 0x0001,
  DLSYNTH_SRC_KEYONVELOCITY = 0x0002,
  DLSYNTH_SRC_KEYNUMBER = 0x0003,
  DLSYNTH_SRC_EG1 = 0x0004,
  DLSYNTH_SRC_EG2 = 0x0005,
  DLSYNTH_SRC_PITCHWHEEL = 0x0006,
  DLSYNTH_SRC_POLYPRESSURE = 0x0007,
  DLSYNTH_SRC_CHANNELPRESSURE = 0x0008,
  DLSYNTH_SRC_VIBRATO = 0x0009,
  DLSYNTH_SRC_CC1 = 0x0081,
  DLSYNTH_SRC_CC7 = 0x0087,
  DLSYNTH_SRC_CC10 = 0x008a,
  DLSYNTH_SRC_CC11 = 0x008b,
  DLSYNTH_SRC_CC91 = 0x00db,
  DLSYNTH_SRC_CC93 = 0x00dd,
  DLSYNTH_SRC_RPN0 = 0x0100,
  DLSYNTH_SRC_RPN1 = 0x0101,
  DLSYNTH_SRC_RPN2 = 0x0102
};

/**
 * @ingroup SoundCreation
 */
enum dlsynth_dest {
  DLSYNTH_DST_NONE = 0x0000,
  DLSYNTH_DST_GAIN = 0x0001,
  DLSYNTH_DST_PITCH = 0x0003,
  DLSYNTH_DST_PAN = 0x0004,
  DLSYNTH_DST_KEYNUM = 0x0005,
  DLSYNTH_DST_CHORUS = 0x0080,
  DLSYNTH_DST_REVERB = 0x0081,
  DLSYNTH_DST_LFOFREQUENCY = 0x0104,
  DLSYNTH_DST_LFOSTARTDELAY = 0x0105,
  DLSYNTH_DST_VIBRATOFREQUENCY = 0x0114,
  DLSYNTH_DST_VIBRATOSTARTDELAY = 0x0115,
  DLSYNTH_DST_EG1ATTACKTIME = 0x0206,
  DLSYNTH_DST_EG1DECAYTIME = 0x0207,
  DLSYNTH_DST_EG1RELEASETIME = 0x0209,
  DLSYNTH_DST_EG1SUSTAINLEVEL = 0x020A,
  DLSYNTH_DST_EG1DELAYTIME = 0x020B,
  DLSYNTH_DST_EG1HOLDTIME = 0x020C,
  DLSYNTH_DST_EG1SHUTDOWNTIME = 0x020D,
  DLSYNTH_DST_EG2ATTACKTIME = 0x030A,
  DLSYNTH_DST_EG2DECAYTIME = 0x030B,
  DLSYNTH_DST_EG2RELEASETIME = 0x030D,
  DLSYNTH_DST_EG2SUSTAINLEVEL = 0x030E,
  DLSYNTH_DST_EG2DELAYTIME = 0x030F,
  DLSYNTH_DST_EG2HOLDTIME = 0x0310,
  DLSYNTH_DST_FILTERCUTOFF = 0x0500,
  DLSYNTH_DST_FILTERQ = 0x0501
};

/**
 * @ingroup SoundCreation
 */
enum dlsynth_transf {
  DLSYNTH_TRN_LINEAR = 0,
  DLSYNTH_TRN_CONCAVE = 1,
  DLSYNTH_TRN_CONVEX = 2,
  DLSYNTH_TRN_SWITCH = 3
};

/// Creates a new blocklist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_blocklist(
 struct dlsynth_blocklist **blocklist ///< [out] The new blocklist
);

/// Adds a connection block to a blocklist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_blocklist_add(
 struct dlsynth_blocklist *
  blocklist, ///< [in] The blocklist to which the connection block will be added
 enum dlsynth_source source,  ///< [in] The source node of the connection block
 enum dlsynth_source control, ///< [in] The control node of the connection block
 enum dlsynth_dest
  destination,      ///< [in] The destination node of the connection block
 int32_t scale,     ///< [in] Scaling factor of the connection
 int sourceInvert,  ///< [in] Whether the source node should be inverted
 int sourceBipolar, ///< [in] Whether the source node is bipolar
 enum dlsynth_transf
  sourceTransform,   ///< [in] Transform to use for the source node
 int controlInvert,  ///< [in] Whether the control node should be inverted
 int controlBipolar, ///< [in] Whether the control node is bipolar
 enum dlsynth_transf
  controlTransform ///< [in] Transform to use for the control node
);

/// Releases the resources associated with a blocklist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_blocklist(
 struct dlsynth_blocklist *blocklist ///< [in] Blocklist to free
);

/// A list of regions
/**
 * @ingroup SoundCreation
 */
struct dlsynth_regionlist;

/// Creates a new regionlist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_regionlist(
 struct dlsynth_regionlist **regionlist ///< [in] The new regionlist
);

/// Adds a region to a regionlist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_add_region(
 struct dlsynth_regionlist
  *regionlist,         ///< [in] The list to which the new region will be added
 uint16_t minKey,      ///< [in] Bottom key in the region
 uint16_t maxKey,      ///< [in] Top key in the region
 uint16_t minVelocity, ///< [in] Lowest velocity in the region
 uint16_t maxVelocity, ///< [in] Greates velocity in the region
 const struct dlsynth_blocklist
  *blocklist,         ///< [in] List of connection blocks for the region
 uint32_t waveIndex,  ///< [in] Index in the wavepool of the sound collection
                      ///< that will be used as a sample
 int selfNonExclusive ///< [in] Whether playing the same note repeatedly should
                      ///< first stop the already playing voices or not
);

/// Adds a region to a regionlist with a wavesample that overrides the one
/// specified in the WAV
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_add_region_wavesample(
 struct dlsynth_regionlist
  *regionlist,         ///< [in] The list to which the new region will be added
 uint16_t minKey,      ///< [in] Bottom key in the region
 uint16_t maxKey,      ///< [in] Top key in the region
 uint16_t minVelocity, ///< [in] Lowest velocity in the region
 uint16_t maxVelocity, ///< [in] Greates velocity in the region
 const struct dlsynth_blocklist
  *blocklist,          ///< [in] List of connection blocks for the region
 uint32_t waveIndex,   ///< [in] Index in the wavepool of the sound collection
                       ///< that will be used as a sample
 int selfNonExclusive, ///< [in] Whether playing the same note repeatedly should
                       ///< first stop the already playing voices or not
 const struct dlsynth_wavesample
  *wavesample ///< [in] The wavesample to use in this region
);

/// Releases the resources associated with a regionlist
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_regionlist(
 struct dlsynth_regionlist *regionlist ///< [in] Regionlist to release
);

/// A list of instruments
/**
 * @ingroup SoundCreation
 */
struct dlsynth_instrlist;

/// Creates a new list of instruments
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_instrlist(
 struct dlsynth_instrlist **list ///< [out] The created instrument list
);

/// Adds an instrument to a list of instruments
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_add_instrument(
 struct dlsynth_instrlist
  *list,            ///< [in] The list to which the new instrument will be added
 uint32_t midiBank, ///< [in] The MIDI bank associated with the instrument
 uint32_t
  midiInstrument, ///< [in] The MIDI instrument associated with the instrument
 int isDrumInstrument, ///< [in] Whether this is a drum instrument or not
 const struct dlsynth_blocklist
  *blocklist, ///< [in] The list of connection blocks for this instrument
 const struct dlsynth_regionlist
  *regions ///< [in] The list of regions in this instrument
);

/// Releases the resources associated with a list of instruments
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_free_instrlist(
 struct dlsynth_instrlist *list ///< [in] The instrument list to free
);

/// Creates a new sound collection
/**
 * @ingroup SoundCreation
 * @return Nonzero on success, zero on failure
 */
int DLSYNTH_EXPORT dlsynth_new_sound(
 struct dlsynth_sound **sound, ///< [out] The created sound collection
 const struct dlsynth_instrlist *instruments, ///< [in] The instrument list
 const struct dlsynth_wavepool
  *wavepool ///< [in] Wavepool for the sound collection
);

#ifdef __cplusplus
}
#endif
#endif