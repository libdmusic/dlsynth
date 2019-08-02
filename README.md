# Introduction

[![Build Status](https://dev.azure.com/libdmusic/dlsynth/_apis/build/status/libdmusic.dlsynth?branchName=master)](https://dev.azure.com/libdmusic/dlsynth/_build/latest?definitionId=2&branchName=master)

`dlsynth` is a software synthesizer library that parses DLS files (as used by
DirectMusic) and outputs audio data.

## Examples

`dlsynth` exposes a pure C interface for ease of use from other languages:

```c
#include <stdio.h>
#include <dlsynth.h>

int main() {
  struct dlsynth_sound* snd;
  if(!dlsynth_load_sound_file("myfile.dls", 44100, &snd)) {
    fprintf(stderr, "Could not load file\n");
    return 1;
  }

  struct dlsynth_instr* instr;
  if(!dlsynth_get_instr_num(0, snd, &instr)) {
    fprintf(stderr, "Could not load instrument\n");
    return 1;
  }

  struct dlsynth_settings settings = {
    44100,               /* Sample rate        */
    2,                   /* Number of channels */
    DLSYNTH_INTERLEAVED, /* Interleaved output */
    16,                  /* Number of voices   */
    instr                /* Instrument to use  */
  };

  struct dlsynth* synth;
  if(!dlsynth_init(&settings, &synth)) {
    fprintf(stderr, "Could not init synth\n");
    return 1;
  }

  dlsynth_note_on(synth, 43);
  if(!dlsynth_render_int16(synth, someBuf, bufLen, 1.0f)) {
    fprintf(stderr, "Could not render buffer\n");
    return 1;
  }

  dlsynth_free(synth);
  dlsynth_free_sound(snd);
}
```

## Including `dlsynth` in an application

A fork of the vcpkg package manager is available [here](https://github.com/libdmusic/vcpkg)
that includes a portfile for `dlsynth`. You can install it by requesting the
`HEAD` version:

```sh
vcpkg install dlsynth --head
```

You can then use `dlsynth` as any other CMake-available library:

```cmake
find_package(dlsynth CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE dlsynth::dlsynth)
```

## Building

### Using [vckg](https://github.com/Microsoft/vcpkg)

After having cloned and build vcpkg, install `riffcpp`:

```sh
vcpkg install riffcpp
```

You will then be able to build dlsynth:

```sh
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
```

`dlsynth` can be built to use either static or dynamic linking for its
dependencies, and itself is able to be built a dynamic or static library

### Without vcpkg

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
