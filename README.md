KBD Trainer
=========================================
A simple mini game for correct korean backdash inputs for Tekken

## Timing stats
In the KBD modes, the panel under the score shows how many frames you held
neutral, back (the second back, before downback cancels the backdash) and
downback. LAST is the most recent rep, and AVG, MIN and MAX cover the last 10
reps. The stats reset when you pick a mode from the menu.


## Building
You need CMake 3.16+ and a C compiler. If SDL3, SDL3_image and SDL3_ttf are
installed, CMake uses them. If not, CMake downloads and builds them for you, so
the first build takes a few minutes. Add `-DKBD_VENDOR_SDL=ON` to always use
the downloaded copy.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The game and its `assets` folder end up in `bin/`. Run `bin/KBDTrainer`
from any directory.

### Windows
On Windows the build makes a single `bin\KBDTrainer.exe` with SDL, the C
runtime, the images and the font all built in, so you can copy just that file
anywhere. SDL is always downloaded and built statically. To use a prebuilt SDL
in `C:\SDL\...` instead, add `-DKBD_VENDOR_SDL=OFF` (you then need its DLLs
next to the exe). Add `-DKBD_EMBED_ASSETS=OFF` to load `assets\` from disk.

### Linux
When CMake builds SDL3 itself, it needs the usual X11/Wayland/audio headers.
On Debian or Ubuntu:

```
sudo apt install build-essential cmake git libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols libegl-dev libgl-dev libasound2-dev \
  libpulse-dev libudev-dev libdbus-1-dev
```

On Fedora or Arch you can install the SDL3 packages instead
(`SDL3-devel SDL3_image-devel SDL3_ttf-devel`, or `sdl3 sdl3_image sdl3_ttf`).
