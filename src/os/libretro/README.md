# Stella libretro core

Stella is a multi-platform Atari 2600 VCS emulator. This document covers
libretro/RetroArch-specific information. For general Stella documentation
see the [Stella website](https://stella-emu.github.io).

## Controller support

Stella's internal ROM database identifies the correct controller type for each
port automatically. No manual configuration is needed for most games.

If a ROM is not in the database, or you need to override the detected type,
go to **Quick Menu → Controls → Port 1 (or Port 2) Device Type** and select
the appropriate controller. The setting takes effect immediately without
reloading the game.

### Supported controller types

| Controller | Notes |
|---|---|
| Joystick | Default. |
| BoosterGrip | B = fire, A = booster button, Y = trigger. |
| Genesis | B = fire, A = button C. |
| Joy 2B+ | B = fire, A = button C, Y = button 3. |
| Paddles | Two paddles per port. Uses left analog stick or mouse. |
| Driving | Uses left analog stick or mouse. |
| Keyboard (keypad) | See below. |
| Trakball | Uses mouse or analog stick. |
| Amiga Mouse | Uses mouse or analog stick. |
| Atari Mouse | Uses mouse or analog stick. |
| Lightgun | Uses RetroArch lightgun device. |
| QuadTari | Four-player adapter. Players 1+2 use ports 1+2; players 3+4 use ports 3+4. |
| MindLink | Uses mouse X axis or left analog stick. B/A buttons act as the trigger. |
| KidVid | Tape selection via keyboard keys `8`/`9`/`0` or joypad B/A/Y. Skip song via key `O` or joypad X. Also selectable via Select + difficulty switches. Requires user-supplied WAV files; see below. |
| AtariVox | Speech synthesis output device; no user input. EEPROM saves to `stella/nvram/` in the RetroArch saves directory. |
| SaveKey | EEPROM storage device; no user input. Saves to `stella/nvram/` in the RetroArch saves directory. |

### KidVid

The KidVid is a cassette player peripheral that plays audio narration and songs
through the TV speaker in sync with gameplay. Two games use it:

- **Smurfs Save the Day** — requires `KVSHARED.WAV`, `KVS1.WAV`, `KVS2.WAV`, `KVS3.WAV`
- **Berenstain Bears** — requires `KVSHARED.WAV`, `KVB1.WAV`, `KVB2.WAV`, `KVB3.WAV`

Place these files in the `stella/` subdirectory of the RetroArch saves
directory. If the files are not present the game still runs, but without
audio narration.

### Keyboard (keypad) controller

The Atari keyboard controller is a 12-key telephone-style keypad. Games that
use it are automatically detected from the ROM database.

Physical key mappings match Stella's defaults:

| Keypad key | Left port | Right port |
|---|---|---|
| 1 | `1` | `8` |
| 2 | `2` | `9` |
| 3 | `3` | `0` |
| 4 | `Q` | `I` |
| 5 | `W` | `O` |
| 6 | `E` | `P` |
| 7 | `A` | `K` |
| 8 | `S` | `L` |
| 9 | `D` | `;` |
| * | `Z` | `,` |
| 0 | `X` | `.` |
| # | `C` | `/` |

The two key sets are deliberately non-overlapping so that games using keyboard
controllers on both ports (e.g. Basic Programming) work without conflict.

#### Recommended hardware setup

The Atari 2600 hardware always used **two physically separate input devices**
for games combining a joystick with a keyboard controller. Star Raiders, for
example, requires a joystick in the left port and a keyboard controller in the
right port. The recommended RetroArch setup mirrors this:

- Use a physical **gamepad** for the joystick port.
- Use the **keyboard** for the keyboard controller port.

#### Hotkey conflicts

RetroArch assigns its own hotkeys (pause, save state, etc.) to bare unmodified
keys, which overlap with the keyboard controller key layout. This is true even
with the recommended gamepad + keyboard setup above — pressing a keyboard
controller key will also trigger any RetroArch hotkey bound to that key.

The libretro API provides no mechanism for a core to suppress RetroArch
hotkeys, so this cannot be resolved from the core side. (Stella's standalone
build avoids the problem entirely by assigning all emulator hotkeys to
modifier key combinations such as Ctrl/Shift/Alt, leaving bare keys free for
controller input. RetroArch does not follow this convention.)

The recommended workaround is to configure a **Hotkey Enable** button in
RetroArch:

> **Settings → Input → Hotkeys → Hotkey Enable**

Set this to any gamepad button you are not otherwise using (e.g. the right
thumbstick click). Once set, RetroArch hotkeys only fire when that button is
held simultaneously, leaving all keyboard controller keys free during normal
gameplay.

For keyboard/keyboard games (Basic Programming, Codebreaker, etc.), two
gamepads are required — one per player. Each gamepad's button layout maps to
the corresponding player's keypad. The Hotkey Enable button must still be
configured to prevent the keyboard controller keys from triggering RetroArch
hotkeys.
