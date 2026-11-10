KBD Trainer
=========================================
A simple mini game for correct korean backdash inputs for Tekken

## Timing stats
In the KBD modes, the panel under the score shows how many frames you held
neutral, back (the second back, before downback cancels the backdash) and
downback. LAST is the most recent rep, and AVG, MIN and MAX cover the last 10
reps. The stats reset when you pick a mode from the menu.


## Movement string mode
P1 MOVES and P2 MOVES show a random string of movement options, five at a
time. Input each move in order; the list scrolls up as you finish one. Every
finished move is worth 100 points, and the run ends on the first wrong input.
Wrong inputs before the first move starts are ignored, and between moves you
can go back to neutral or let go of part of the last input.

Inputs are for the P1 side (P2 swaps forward and back). The first KBD of a
chain comes with its backdash as one move, BACKDASH KBD (b, n, b, db, b, n, b).
After that, the last back of each KBD counts as the next KBD's first back: KBD
into KBD is b, db, b, n, b, db, b, n, b. A dash into a wave dash is also one
move, DASH WAVEDASH, where the dash's second f is the wave dash's first f.
After a wave dash, the next move needs a neutral first: wave dash into dash is
f, n, d, df, f, n, f, n, f.

| Move          | Input                              |
|---------------|------------------------------------|
| BACKDASH      | b, n, b                            |
| BACKDASH KBD  | b, n, b, db, b, n, b               |
| KBD           | b, db, b, n, b (only after a KBD)  |
| SIDESTEP UP   | tap u (let go within 15 frames)    |
| SIDEWALK UP   | u, n, u and hold for 15 frames     |
| SIDESTEP DOWN | tap d (let go within 15 frames)    |
| SIDEWALK DOWN | d, n, d and hold for 15 frames     |
| DASH          | f, n, f                            |
| WAVEDASH      | f, n, d, df, f                     |
| DASH WAVEDASH | f, n, f, n, d, df, f               |
| BLOCK         | b (only shows up between two dashes) |

A backdash and the side steps and walks never come up twice in a row (two
backdashes in a row is a KBD), and a backdash never comes right after a KBD.
KBD and wave dash can repeat. Two dashes in a row always have a BLOCK between
them. The bar next to a side step or walk fills while you hold the input; a
side step has to let go before it fills.


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
