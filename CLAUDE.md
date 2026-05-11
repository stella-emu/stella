# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Stella is an Atari 2600 VCS emulator written in C++20, targeting Linux, macOS, and Windows via SDL3.

## Build System

Uses a custom shell configure script + GNU Make (not CMake or autotools).

```bash
./configure          # First-time setup; generates config.mak
make -j$(nproc)      # Build main stella executable (output: out/stella)
make test            # Build and run unit tests (Google Test, output: out.test/stella-test)
make clean           # Remove object files
make distclean       # Remove object files + config.mak
make DEBUG=1         # Debug build with symbols and _GLIBCXX_DEBUG
make RELEASE=1       # Release build with LTO (-flto=auto)
make USE_FULL_WARNINGS=1  # Enable -Weverything (clang only)
```

Dependencies detected by `configure`: SDL3, libpng16, sqlite3, zlib. Tests require googletest.

Module-level build rules live in per-directory `module.mk` files; `Makefile` assembles them. Object files go to `out/` (or `$STELLA_BUILD_ROOT/stella-out`). Generated instruction table `src/emucore/M6502.ins` is produced from `src/emucore/M6502.m4` via `m4`.

## Architecture

Stella is a classic emulation architecture centered on `OSystem` owning everything, with `Console` representing the physical hardware.

### Initialization chain
`OSystem` → creates `Console` → which constructs `System` (addressing bus) + `M6502` (CPU) + `M6532` (RIOT) + `TIA` + `Cartridge`. `OSystem` also owns `FrameBuffer`, `EventHandler`, `Sound`, `Debugger`, and GUI.

### Key subsystems

**Emulation core** (`src/emucore/`):
- `OSystem` — top-level owner; manages lifecycle, framebuffer creation, event loop
- `Console` — the VCS hardware model; handles serialization, timing, audio
- `System` — 8KB addressing bus with 64-byte page granularity; interconnects all devices
- `M6502` — 6502 CPU; instruction set is generated from `M6502.m4` → `M6502.ins`
- `M6532` — RIOT chip (128-byte RAM, I/O ports, interval timer)
- `CortexM0` — ARM Cortex-M0 emulator used for ELF-based cartridges

**TIA** (`src/emucore/tia/`): TV Interface Adapter — video and audio generation. Subobjects: `Player`, `Missile`, `Ball`, `Playfield`, `Audio`, frame managers for NTSC/PAL.

**Cartridge system** (`src/emucore/Cart*.cxx`): 50+ cartridge types implementing various bankswitch schemes. `CartDetector` auto-identifies ROM type; `CartCreator` is the factory.

**Graphics** (`src/emucore/FrameBuffer.*`, `FBSurface.*`): Platform-agnostic abstraction. `FBBackendSDL` implements SDL3 rendering. `TIASurface` converts raw TIA output to display pixels.

**Input** (`src/emucore/EventHandler.*`, `src/emucore/Control*.cxx`): Keyboard/joystick/mouse → event dispatch → controller emulation. `ControllerDetector` auto-identifies peripheral type.

**Debugger** (`src/debugger/`): Interactive debugger with DiStella disassembler, breakpoints, watchpoints, expression parser (yacc/bison in `src/debugger/yacc/`), and dedicated GUI dialogs in `src/debugger/gui/`.

**GUI** (`src/gui/`): Dialog/widget system for launcher, options, high scores, etc.

**Common utilities** (`src/common/`): Audio pipeline (`AudioQueue`, `AudioSettings`), filesystem abstraction (`FSNode`), state/rewind managers, high scores, palette, TV filters (`tv_filters/`), SDL blitters (`sdl_blitter/`), persistence layer (`repository/` — JSON, properties, SQLite).

**Platform code** (`src/os/unix/`, `src/os/windows/`, `src/os/macos/`): OSystem subclasses and filesystem implementations per platform.

**Third-party libs** (`src/lib/`): httplib, nanojpeg, tinyexif, nlohmann/json, SQLite wrapper.

### Data flow (one emulation frame)
`OSystem::mainLoop` → `Console::update` → `M6502::execute` (runs CPU cycles) → each cycle notifies `TIA` and `M6532` via `System` → TIA produces scanlines into a pixel buffer → `TIASurface` uploads buffer to `FBBackendSDL` → SDL presents frame.

## Code Conventions

- C++20; warnings treated as errors in CI (`-Wall -Wextra -Wunused -Woverloaded-virtual`)
- Tabs for indentation (width 2 per VSCode config)
- Headers use `.hxx`, sources use `.cxx`
- New devices/cartridges must register with `System::resetDevice` and implement `peek`/`poke`
