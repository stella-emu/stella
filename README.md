Stella is a multi-platform Atari 2600 VCS emulator which allows you to
play all of your favourite Atari 2600 games on your PC.  You'll find the
Stella Users Manual in the docs subdirectory.  If you'd like to verify
that you have the latest release of Stella, visit the Stella Website at:

  [stella-emu.github.io](https://stella-emu.github.io)

Enjoy,

The Stella Team

# Reporting issues

Please check the list of known issues first (see below) before reporting new ones.
If you encounter any issues, please open a new issue on the project
[issue tracker](https://github.com/stella-emu/stella/issues).

Note: Please do **not** report issues regarding platforms other than PC and RetroN 77 
(e.g. Libretro) here. The Stella Team is not responsible for these forks and cannot 
help with them.

# Known issues

Please check out the [issue tracker](https://github.com/stella-emu/stella/issues) for
a list of all currently known issues.

# macOS Installation

When downloading Stella on macOS, Gatekeeper may block the app from opening with a message like *"Stella cannot be opened because Apple cannot verify it is free from malware."*

To work around this, use either of the following methods:

**Option 1 — Right-click to open (easiest):**
1. Right-click (or Control-click) `Stella.app`
2. Select **Open** from the context menu
3. Click **Open** in the confirmation dialog

You only need to do this once; Stella will open normally after that.

**Option 2 — Terminal:**
```bash
xattr -dr com.apple.quarantine /path/to/Stella.app
```
