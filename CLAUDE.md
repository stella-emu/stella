# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Stella is an Atari 2600 VCS emulator written in C++23, targeting Linux, macOS, and Windows via SDL3.

## Build System

Uses a custom shell configure script + GNU Make (not CMake or autotools).

```bash
./configure          # First-time setup; generates config.mak
make -j$(nproc)      # Build main stella executable (output: out/stella)
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

**GUI** (`src/gui/`): Dialog/widget system for launcher, options, high scores, etc. New dialogs use the relative layout engine (`src/gui/Layout.*`) via a create-only constructor plus a `layout()` override — see [src/gui/DIALOGS.md](src/gui/DIALOGS.md) for the full guide.

#### Before touching `src/gui/` or `src/debugger/gui/` — read this first

Every dialog in `src/gui/` is already converted to the layout engine, so there is a
precedent for whatever you are about to do. These rules are not style preferences;
ignoring them has repeatedly cost whole sessions of rework.

1. **Read the exemplar.** Open the nearest already-converted file and follow its
   *shape* before designing anything. Notes and summaries give you the vocabulary,
   not the design.
2. **No new seam without asking.** Do not invent a virtual, protocol, collector or
   helper that has no counterpart in `src/gui/`. If the existing seams cannot
   express what you need, that is a question to ask, not a design to make.
3. **State intent, not pixels.** No pixel arithmetic, no `setPos()` after a layout,
   no hand-summed widths, no measured specimen strings
   (`getStringWidth("Manufacturer ")`), no labels padded with spaces to fake
   alignment. Extend the engine rather than working around it.
4. **One `align*` call per group**, made by the code that *builds* the group
   (`alignLabels` / `alignPopUps` / `alignButtons` / `alignTracks`). Size cells with
   `addAuto()`.
5. **Widgets own their size.** A constructor never bakes geometry: if a widget can
   derive it (button, pop-up, edit field, slider, static text), don't pass it. All
   geometry lives in `layout()` / `reflow()`.

The debugger tab widgets in `src/debugger/gui/` are `Widget`s, not `Dialog`s. The
five rules above still hold, but the shape and exemplars differ, so also obey:

6. **Create-only constructor + `reflow()`.** Build every child at `0, 0` in the
   constructor, which *ends* with `reflow()`. Add
   `setArea(x,y,w,h){ Widget::setArea(...); reflow(); }`. `reflow()` reads `_x/_y/_w`,
   builds a `GUI::BoxLayout`/`GridLayout` tree and `doLayout()`s it. Do **not** add a
   `max_w`/size constructor parameter (the old `CpuWidget`/`RamWidget` style).
7. **The exemplars are the debugger files**, not the `src/gui/` dialogs:
   `CartRamWidget::reflow` and `RomWidget::reflow` (the `setArea`/`reflow` shape),
   `CartDebugWidget` (skeleton + one `layoutContent` hook), `CpuWidget`
   (`VAlign::Baseline` label + `DataGrid` rows). Open one before designing.
8. **Labels beside a `DataGrid`'s rows** use the `RamWidget` `gridLabels` idiom (a
   vertical box at the grid's row pitch, offset by
   `grid->firstTextY() - label->firstTextY()`), never an `N*fontWidth` column.
9. **A composite that hosts sub-widgets** (`ControllerWidget`, `CartDebugWidget`)
   shares one seam: the base owns the skeleton plus a virtual
   `layoutContent(GUI::BoxLayout&)` hook. Don't invent a second one — that is a
   question to ask (rule 2).
10. **The root box carries the font-derived borders as its margins**
    (`HBORDER = fontWidth*1.25`, `VBORDER = fontHeight/2`, `VGAP = fontHeight/4`),
    computed from `_font`.

**Common utilities** (`src/common/`): Audio pipeline (`AudioQueue`, `AudioSettings`), filesystem abstraction (`FSNode`), state/rewind managers, high scores, palette, TV filters (`tv_filters/`), SDL blitters (`sdl_blitter/`), persistence layer (`repository/` — JSON, properties, SQLite).

**Platform code** (`src/os/unix/`, `src/os/windows/`, `src/os/macos/`): OSystem subclasses and filesystem implementations per platform.

**Third-party libs** (`src/lib/`): httplib, nanojpeg, tinyexif, nlohmann/json, SQLite wrapper.

### Data flow (one emulation frame)
`OSystem::mainLoop` → `Console::update` → `M6502::execute` (runs CPU cycles) → each cycle notifies `TIA` and `M6532` via `System` → TIA produces scanlines into a pixel buffer → `TIASurface` uploads buffer to `FBBackendSDL` → SDL presents frame.

## Code Conventions

- C++23; warnings treated as errors in CI (`-Wall -Wextra -Wunused -Woverloaded-virtual`)
- Tabs for indentation (width 2 per VSCode config)
- Headers use `.hxx`, sources use `.cxx`
- New devices/cartridges must register with `System::resetDevice` and implement `peek`/`poke`
