KBD Trainer
=========================================
A simple mini game for correct korean backdash inputs for Tekken


## Building
You need CMake 3.16+ and a C compiler. If SDL3, SDL3_image and SDL3_ttf are
installed, CMake uses them (on Windows it looks in `C:\SDL\...`). If not, CMake
downloads and builds them for you, so the first build takes a few minutes.
Add `-DKBD_VENDOR_SDL=ON` to always use the downloaded copy.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The game and its `assets` folder end up in `bin/`. Run `bin/KBDTrainer`
(or `bin\KBDTrainer.exe` on Windows) from any directory.

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
