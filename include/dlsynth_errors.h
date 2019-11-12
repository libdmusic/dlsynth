DLSYNTH_ERROR(
 UNKNOWN_ERROR, -1,
 "An unknown error has been encountered while processing the request")
DLSYNTH_ERROR(NO_ERROR, 0, "No error has been encountered")
DLSYNTH_ERROR(CANNOT_OPEN_FILE, 1, "The file couldn't be opened")
DLSYNTH_ERROR(INVALID_FILE, 2,
              "The file was opened correctly, but contained invalid data")
DLSYNTH_ERROR(INVALID_INSTR, 3, "The specified instrument doesn't exist")
DLSYNTH_ERROR(INVALID_ARGS, 4, "The arguments that were passed were not valid")
DLSYNTH_ERROR(UNSUPPORTED_CODEC, 5, "The file uses an unsupported codec")
DLSYNTH_ERROR(CONDITION_FAILED, 6,
              "A condition has failed while loading the file")
DLSYNTH_ERROR(INVALID_WAVE_INDEX, 7, "A region uses an invalid wavepool index")
DLSYNTH_ERROR(INVALID_WAVESAMPLE, 8,
              "A wavesample specifies an out-of-range region")
DLSYNTH_ERROR(NO_WAVESAMPLE, 9, "A region has no wavesample")