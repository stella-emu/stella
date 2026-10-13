# Raw stream (`-rawstream`)

This fork of Stella can write every emulated frame and every audio sample,
untouched, to a file, a pipe or stdout. Stella does nothing else with it: no
palette, scaling, filters, resampling or encoding. Outside tools turn it into
whatever they need (a video, a live stream, a test log).

```
Stella -rawstream capture.raw game.bin      write to a file
Stella -rawstream - game.bin | tool         write to stdout
Stella -rawstream \\.\pipe\name game.bin    write to a Windows named pipe
```

With `-`, Stella's own console messages move to stderr so stdout carries only
the stream. On Windows, don't pipe it through PowerShell 5.1, which corrupts
binary data between programs; start Stella from the reading program instead.

The stream is written in emulated time, not wall-clock time. If the reader
falls behind, Stella waits for it rather than drop anything. If the reader
goes away, the stream stops and Stella keeps running.

## Chunks

The stream is a sequence of chunks:

| Bytes | Meaning |
|---|---|
| 4 | Tag, four ASCII characters |
| 4 | Payload length in bytes (uInt32) |
| n | Payload |

All numbers are little-endian. Readers must skip tags they don't know; later
versions may add new chunks, and may add fields to the end of a payload.

| Tag | When | Payload |
|---|---|---|
| `HEAD` | Before the first frame, and again when the ROM or the TV type changes | Format and timing, see below |
| `PALT` | After every `HEAD`, and whenever the palette changes | 256 x (R, G, B) bytes, indexed by the TIA color value |
| `AUDI` | Before each `FRAM` | The audio samples since the previous `FRAM` |
| `RAM ` | Just before each `FRAM` | The console's 128 bytes of RAM as the frame ended |
| `FRAM` | Every emulated frame | One frame of TIA color values |
| `JUMP` | Reset, state load (including rewind), ROM load | 1 byte: why |
| `END ` | Stella closed the stream normally | empty |

A stream normally looks like: `HEAD PALT AUDI RAM FRAM AUDI RAM FRAM ...`

### `HEAD`

| Type | Field |
|---|---|
| uInt16 | Format version (1) |
| uInt8 | TV type: 0 NTSC, 1 PAL, 2 SECAM |
| uInt8 | 0 |
| uInt32 | Audio sample rate: 31440 (NTSC) or 31200 (PAL, SECAM) |
| uInt32 | CPU cycles per second: 38 x the sample rate |
| uInt16 | Frame width in pixels (160) |
| uInt16 | 0 |
| 31 x Int16 | Mono mixing table, indexed by channel 0 volume + channel 1 volume |
| 16 x Int16 | Stereo mixing table, indexed by one channel's volume |
| uInt16 + bytes | ROM file name (UTF-8) |
| uInt16 + bytes | ROM MD5 (hex text) |

These are the numbers Stella's own emulation runs on: an NTSC frame of 262
lines is 1/60 s, so 524 samples; a PAL frame of 312 lines is 1/50 s, so 624
samples. The mixing tables are the ones Stella uses to turn TIA volumes into
sound, so a reader can reproduce Stella's output exactly, or mix its own way.

### `AUDI`

One byte per sample, two samples per scanline (the TIA's own audio clock):
channel 0's volume (0-15) in the low four bits, channel 1's in the high four.
This is the same packing Stella uses internally for Time Machine sound.

To get Stella's mono output: `mono[(b & 15) + (b >> 4)]`.
For stereo: left `stereo[b & 15]`, right `stereo[b >> 4]`.

### `RAM `

128 bytes: the console's RAM (the 6532 RIOT chip's), addresses $80 to $FF in
order, read at the moment the frame ended (the same moment as the `FRAM` that
follows). Byte n is address $80 + n. Cartridge RAM (Superchip, ARM carts) is
not included.

### `FRAM`

| Type | Field |
|---|---|
| uInt64 | CPU cycle count when the frame ended |
| uInt32 | Frame number |
| uInt16 | Scanlines in this frame (VSYNC to VSYNC) |
| uInt16 | Scanline (counted from VSYNC) of the first row below |
| uInt16 | Width (160) |
| uInt16 | Height: number of rows below |
| width x height bytes | TIA color value of each pixel, row by row |

The rows are the part of the frame Stella displays, so the height can change
when Stella changes its display window. A reader that needs a fixed size
(video encoders do) should choose one and pad or crop.

Each frame's audio is the `AUDI` chunk just before it, so picture and sound
never drift: for an unbroken run of frames, a frame's sample count is twice
its scanline count, and its cycle count is 76 x its scanlines past the
previous frame's.

### `JUMP`

| Value | Reason |
|---|---|
| 0 | Reset |
| 1 | State loaded (save state, rewind, Time Machine) |
| 2 | ROM loaded or reloaded (a `HEAD` follows) |

Emulated time runs on from frame to frame except across a `JUMP`.

## Where the data comes from

| Data | Stella source |
|---|---|
| Frames | `TIA::onFrameComplete()`, every frame, including frames the screen skips |
| RAM | `M6532::getRAM()`, read in `TIA::onFrameComplete()` |
| Samples | `Audio::createSample()`, before Stella resamples for the sound card |
| Palette | `FrameBuffer::setTIAPalette()`, the RGB palette Stella displays with |

Frames and samples are produced on the emulation thread in emulated order,
which is why the interleaving is exact. Frame layout autodetection runs the
emulation ahead at startup; those frames are left out.
