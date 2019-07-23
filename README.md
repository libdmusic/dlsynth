# dlsynth

`dlsynth` is a software synthesizer library that parses DLS files (as used by
DirectMusic) and outputs audio data.

## Examples

`dlsynth` exposes a pure C interface for ease of use from other languages:

```c
#include <stdio.h>>
#include <dlsynth.h>

int main() {
  struct dlsynth_sound* snd;
  if(!dlsynth_load_sound_file("myfile.dls", 44100, &snd)) {
    fprintf(stderr, "Could not load file\n");
    return 1;
  }

  struct dlsynth_settings settings = { 44100, 2, DLSYNTH_INTERLEAVED };

  struct dlsynth* synth;
  if(!dlsynth_init(&settings, &synth)) {
    fprintf(stderr, "Could not init synth\n");
    return 1;
  }

  if(!dlsynth_render_int16(synth, someBuf, bufLen)) {
    fprintf(stderr, "Could not render buffer\n");
    return 1;
  }

  dlsynth_free(synth);
  dlsynth_free_sound(snd);
}
```

## Building

You will need to obtain a binary copy of [riffcpp](https://github.com/libdmusic/riffcpp)
and install it somewhere CMake can find it.

After cloning the repo, run the following commands:

```sh
mkdir build
cd build
cmake .. # -DCMAKE_PREFIX_PATH=/path/to/riffcpp if riffcpp was not installed in the $PATH
cmake --build .
```

If you also want to install it:
```sh
cmake --build . --target install
```

You can change the path where the library is installed by
changing the `CMAKE_INSTALL_PREFIX` value during configuration.
