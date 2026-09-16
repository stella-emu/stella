/* vcs_render.h -- the cartridge composes the text the console cannot draw.
 *
 * The 2600 has no framebuffer and 128 bytes of RAM. On every sibling console
 * "draw a string" is a memory write; here it is a cycle-exact kernel, and the
 * glyph data has to be in the shape that kernel wants before the raster
 * arrives. So the cart keeps the font and publishes ready-to-stream bytes --
 * the console kernel then does nothing but indexed loads into GRP0/GRP1.
 *
 * The layout is six 128-byte planes at $1800-$1AFF, one per GRP write of the
 * 48-pixel kernel, indexed by ABSOLUTE SCANLINE (row * FN_T_CELL_H + line).
 * The 128-byte alignment is load-bearing: it is what makes `lda plane,y`
 * always four cycles and never five, and a 2600 kernel that spends an
 * unpredictable number of cycles does not draw, it tears.
 *
 * tools/vcsfont.py is the reference implementation of exactly this, written
 * first and independently; host_test/test_render.c byte-compares the two.
 */
#ifndef VCS_RENDER_H
#define VCS_RENDER_H

#include <stdbool.h>
#include <stdint.h>

/* Compose `len` characters into text row `row` of the planes in `win` (the
 * 4K served window). Short rows are space-filled and long ones truncated --
 * a client that streams an over-long filename gets it cut off rather than
 * corrupting the next row. */
void vcs_render_row(uint8_t *win, uint8_t row, const uint8_t *text, uint8_t len);

/* Blank every text row. */
void vcs_render_clear(uint8_t *win);

/* Run a staged blit: `cnt` bytes from the reply window at `src` into the text
 * planes at `dst`, with `transform` applied on the way (FN_BLIT_*).
 *
 * This is what lets a client turn a server reply into a display without ever
 * holding it in the console's 128 bytes of RAM -- six stores set it up and the
 * cartridge does the move. For FN_BLIT_TEXT, FN_BLIT_FIELD and FN_BLIT_HULLS
 * the destination is a text ROW, not a byte offset.
 *
 * `board` is FN_BOARD_CELLS bytes of cartridge scratch holding the glyph
 * currently in each cell of the board being composed. FN_BLIT_FIELD fills it
 * from the reply and FN_BLIT_HULLS overlays onto it, which is what lets your
 * own ships show through where nothing has been fired at them yet -- an
 * overlay needs to know what is already there, and the text planes are packed
 * glyph pairs that cannot be read back as characters.
 *
 * FN_BLIT_PFCLR / PFIELD / PFHULL / PFCELL compose the PLAYFIELD board --
 * the four-quadrant Battleship display -- straight into the plane region's
 * table bytes (fuji_mailbox.h has the layout and the bit map). They use no
 * scratch: every write is a whole table byte, masked in place, so they need
 * nothing from `board` and the kernel reading the tables never sees a
 * half-composed byte.
 *
 * Returns false for a transform it does not implement. */
bool vcs_blit(uint8_t *win, uint8_t *board, uint16_t src, uint16_t dst,
              uint8_t cnt, uint8_t transform);

/* FN_BLIT_PATH: render `cnt` characters of a path buffer, starting at
 * character `src`, into text row `row`.
 *
 * This is NOT part of vcs_blit() because vcs_blit()'s source is the reply
 * window and this one's is the cartridge's own path buffer -- a different
 * thing entirely, and widening that function's signature to carry it would
 * make eight existing call sites pass an argument seven of them ignore.
 * FN_BLIT_PATH is routed to here by the caller instead.
 *
 * Past the end of the buffer renders as spaces rather than stopping short, so
 * a shrinking value leaves no debris behind it on the row. */
void vcs_render_path_row(uint8_t *win, const uint8_t *path, uint16_t path_len,
                         uint16_t src, uint8_t row, uint8_t cnt);

/* FN_BLIT_TCELL: replace the single character at (row, col).
 *
 * A column shares its plane byte with its neighbour -- left in bits 7-5,
 * right in bits 3-1 -- so this is a masked read-modify-write of six bytes.
 * The planes cannot be read back as CHARACTERS, but they can be read back as
 * bits, which is all this needs: the neighbour's bits are simply kept. */
void vcs_render_cell(uint8_t *win, uint8_t row, uint8_t col, uint8_t c);

/* FN_BLIT_CARD: paint one seat's five cards across text rows `row` and
 * `row + 1`.
 *
 * `hand` is the eleven wire bytes -- five cards as two lowercase ASCII bytes
 * each, then the NUL that ends them. `flags` takes FN_CARD_*.
 *
 * Cards ignore the cell grid: five of them sit on a six-pixel pitch across
 * planes 0-3, so columns 8-11 stay free for the seat's name and purse. Whole
 * plane bytes are written, so the card bed clears itself and a stale hand
 * cannot show through a shorter one -- but the sixth line of each cell is
 * left blank, because rank and pip are separated by it and the console's
 * kernel reprograms colour there.
 *
 * Out-of-range rows draw nothing rather than writing past the planes. */
void vcs_render_cards(uint8_t *win, uint8_t row, const uint8_t *hand,
                      uint8_t flags);

#endif /* VCS_RENDER_H */
