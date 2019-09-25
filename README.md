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

  struct dlsynth* synth;
  if(!dlsynth_init(
      44100, /* Output sample rate */
      32,    /* Number of voices   */
      &synth)) {
    fprintf(stderr, "Could not init synth\n");
    return 1;
  }

  dlsynth_note_on(synth,
    instr, /* Instrument to use   */
    0,     /* Channel of the note */
    100,   /* Note priority       */
    43     /* MIDI note           */
  );

  int16_t* outputBuffer = /* ... */;

  // Mono rendering
  if(!dlsynth_render_int16(synth, numFrames, outputBuffer, NULL, 1, 1.0f)) {
    fprintf(stderr, "Could not render buffer\n");
    return 1;
  }

  // Interleaved stereo rendering
  if(!dlsynth_render_int16(synth, numFrames, outputBuffer, outputBuffer + 1, 2, 1.0f)) {
    fprintf(stderr, "Could not render buffer\n");
    return 1;
  }

  // Sequential stereo rendering
  if(!dlsynth_render_int16(synth, numFrames, outputBuffer, outputBuffer + numFrames, 1, 1.0f)) {
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

On Debian-based distros (like Ubuntu) you can add the libdmusic PPA:

```sh
sudo add-apt-repository ppa:libdmusic/unstable
```

then run

```sh
sudo apt update
sudo apt install libdlsynth-dev
```

You can then use `dlsynth` as any other CMake-available library:

```cmake
find_package(dlsynth CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE libdmusic::dlsynth)
```

## Building

### Using [vcpkg](https://github.com/Microsoft/vcpkg)

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
and its headers and install it somewhere CMake can find it.

Under Debian-based distros (like Ubuntu) you can add the following line to `/etc/apt/sources.list.d/libdmusic.list`:

```txt
deb [trusted=yes] https://repo.libdmusic.org/apt/ /
```

then run

```sh
sudo apt update
sudo apt install libriffcpp-dev
```

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
