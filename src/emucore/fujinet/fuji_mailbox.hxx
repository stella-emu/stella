/* fuji_mailbox.h -- FujiNet cartridge mailbox layout for the Atari 2600.
 *
 * Single source of truth, shared by the RP2040 cart firmware, the MAME cart
 * device model, and (hand-mirrored) the 6502 console client in
 * testrom/fujilib.inc.
 *
 * THE CHANNEL F'S MECHANISM, THE ASTROCADE'S HAZARDS
 *
 * The Astrocade, Arcadia and ColecoVision carts push both mailbox directions
 * through the read path because none of those cartridge edges carries a write
 * strobe. The 2600's edge does not carry one either -- A0-A12, D0-D7, +5V and
 * GND, nothing else -- and yet this port takes writes, the way the Channel F
 * does.
 *
 * It can, because the 6507 drives D0-D7 during a store and the cart sees that
 * bus. The cart declares certain pages write-only, never drives them, and
 * recovers the byte by parking on the stable address and keeping the
 * second-to-last data sample:
 *
 *     while (ADDR_IN == addr) { data_prev = data; data = DATA_IN; }
 *     append(data_prev);
 *
 * That is not invented here. It is how every shipping PlusROM game talks to
 * its cart and how every Superchip cart implements its 128 bytes of RAM --
 * both idioms are in ../src/cartridge_emulation.cpp, which this tree carries.
 *
 * BUT THE SIMPLIFICATION THE CHANNEL F GOT DOES NOT FOLLOW. The Channel F
 * cart knows a store from a fetch: ROMC 05 against ROMC 02. That is why its
 * header can say reads of the register pages are inert and the stray-READ
 * hazard class does not exist. This cart knows no such thing. It sees an
 * address and nothing else, so A READ OF A WRITE PORT IS BIT-IDENTICAL TO A
 * WRITE OF ONE: a stray LDA $1D10 decodes as a register access carrying
 * whatever the floating bus held.
 *
 * So the REGSEL/REGDATA arm-then-commit pair is KEPT, and here it is the
 * defence rather than the shape it is on the Channel F. fujimail.c's
 * disarm-after-one-use (regsel = REGSEL_INERT) does real work on this
 * console. One register write is two stores:
 *
 *     lda #value
 *     sta FN_H_REGSEL+n     ; arm register n; this store's data is ignored
 *     sta FN_H_COMMIT       ; register n = value; disarms
 *
 * What we do get, and it is worth more than the store we spend: THE 6507 HAS
 * NO INTERRUPTS. MAME's own m6507.cpp says it plainly -- "28-pin package,
 * address bus is 13 bits, no NMI, no SO, no SYNC" -- and the 2600 wires
 * nothing to /IRQ. Only BRK reaches $1FFE. Every stray access on this console
 * therefore originates in the client's own instruction stream, never from an
 * asynchronous event, which is why tools/checkrom.py can be a complete static
 * proof rather than the heuristic it would have to be on the ColecoVision,
 * where the vblank NMI lands mid-transaction by construction.
 *
 * MEMORY MAP (console addresses; A12 high is the whole chip select)
 *
 *   $1000-$17FF  2K banked    client code, page selected by FN_HOT_BANK
 *   $1800-$1AFF  768 B        six 128-byte text planes -- see below
 *   $1B00-$1CFF  512 B        reply window (2 slices x 512 = 1024)
 *   $1D00-$1DFF  write-only   control: arm, commit, bank, render, swap
 *   $1E00-$1EFF  write-only   TX stream: a write anywhere appends its data
 *   $1F00-$1FFF  256 B        status, claim, fixed tail, and the vectors
 *
 * WHY THE REPLY IS 512 AND NOT 256 OR 1024. 1K would be four of sixteen
 * pages, which the map cannot afford. 256 would fit in one -- but
 * fujinet-5cardstud/arcadia's fujinet.inc pins GMAXLEN at 418 (a Game) and
 * TMAXLEN at 361 (a Tables), so 512 is the number at which THE FLAGSHIP
 * CLIENT NEVER PAGES A SLICE AT ALL. Its whole reply lands in slice 0 and the
 * slice-crossing cursor that port needed disappears. FN_R_NSLICES 2 keeps
 * FUJIMAIL_RX_MAX at 1024 for everything else.
 *
 * WHY FN_H_REGDATA AND FN_H_DATA COST NO ADDRESS SPACE. fujimail_read_hotspot
 * switches on offset >> 8 and only needs three DISTINCT page numbers; the bus
 * layer synthesises the events, exactly as the Channel F's CHF_EV_REG case
 * does. So two of the three live OUTSIDE the 4K window and never appear on
 * the console bus. Sixteen pages cannot spare three for control, and this is
 * what buys them back.
 */

#ifndef FUJI_MAILBOX_H
#define FUJI_MAILBOX_H

/* ---------------- address map ---------------- */

#define FN_WINDOW_BASE   0x1000   /* A12 high: the cartridge is selected     */
#define FN_WINDOW_SIZE   0x1000   /* 4K, and there is no more                */

#define FN_BANK_BASE     0x1000   /* the banked low half                     */
#define FN_BANK_SIZE     0x0800   /* 2K per bank                             */
#define FN_FIXED_BASE    0x1800   /* the fixed high half: everything below   */

/* ---------------- cart -> console: the text planes ----------------
 *
 * The 2600 has no framebuffer. The CPU races the beam and every scanline is
 * built by hand, so on every sibling console "draw a string" is a memory
 * write and here it is a kernel. Rather than spend the client's RAM and
 * cycles composing glyphs, the CART composes them: it keeps the font, renders
 * ASCII into bytes already in the shape the 48-pixel player kernel wants, and
 * publishes them here.
 *
 * SIX PLANES OF 128 BYTES, one per GRP write of the 48-pixel kernel:
 *
 *   plane 0  $1800  1st GRP0   text columns 0-1
 *   plane 1  $1880  1st GRP1   text columns 2-3
 *   plane 2  $1900  2nd GRP0   text columns 4-5
 *   plane 3  $1980  2nd GRP1   text columns 6-7
 *   plane 4  $1A00  3rd GRP0   text columns 8-9
 *   plane 5  $1A80  3rd GRP1   text columns 10-11
 *
 * The index is the ABSOLUTE SCANLINE, Y = row * FN_T_CELL_H + scanline, and
 * that is the whole trick. Because a plane is 128-byte aligned and Y < 128,
 * base_lo + Y can never carry: `lda $1800,y` is ALWAYS exactly four cycles,
 * never five. A 2600 kernel that spends an unpredictable number of cycles
 * does not draw, it tears -- so this alignment is not tidiness, it is the
 * difference between a display and a mess. It also means the kernel needs NO
 * zero-page pointers and NO per-row setup: Y simply counts 0..125 down the
 * whole screen and the text rows are implicit in the data.
 *
 * Byte format: bit 7 is the leftmost pixel; the left text column of the pair
 * occupies bits 7-5 with bit 4 as its inter-character gap, the right column
 * bits 3-1 with bit 0 as its gap. A 3x5 glyph in a 4x6 cell.
 */
#define FN_T_BASE        0x1800
#define FN_T_PLANES      6
#define FN_T_PLANE_LEN   0x80     /* 128-byte alignment is load-bearing      */
#define FN_T_PLANE(p)    (FN_T_BASE + (p) * FN_T_PLANE_LEN)
#define FN_T_COLS        12       /* 12 x 4px = the 48-pixel player span     */
#define FN_T_CELL_H      6        /* 5 rows of ink, 1 of leading             */
#define FN_T_ROWS        21       /* 21 * 6 = 126 <= 128                     */

/* ---------------- cart -> console: painted memory ---------------- */

#define FN_R_DATA        0x1B00   /* the reply slice                         */
#define FN_R_SLICE_LEN   0x200    /* 512 bytes...                            */
#define FN_R_NSLICES     2        /* ...x 2 = FUJIMAIL_RX_MAX                */

#define FN_R_BASE        0x1F00   /* first painted status byte               */
#define FN_R_ACKSEQ      0x1F00   /* echoes SEQ when the reply is ready      */
#define FN_R_STATUS      0x1F01   /* bit0 link up, bit1 busy                 */
#define FN_R_ERR         0x1F02   /* fb_status_t of the last transaction     */
#define FN_R_REPLY_CMD   0x1F03   /* 0x06 ACK / 0x15 NAK                     */
#define FN_R_RXLEN_LO    0x1F04   /* total reply length, LE                  */
#define FN_R_RXLEN_HI    0x1F05
#define FN_R_BOOT_STATE  0x1F06
#define FN_R_BOOT_PCT    0x1F07   /* 0-100, for a MOUNT_IMAGE progress bar   */
#define FN_R_BOOT_ERR    0x1F08
#define FN_R_MAGIC0      0x1F09   /* 'F' -- cart presence check              */
#define FN_R_MAGIC1      0x1F0A   /* 'N'                                     */
#define FN_R_PROTO_VER   0x1F0B
#define FN_PROTO_VER     2        /* 2 = banking, as on the Astrocade        */
#define FN_R_SLICE_ECHO  0x1F0C   /* published LAST after every repaint      */

#define FN_R_PAINT_END   0x1F0D   /* paint covers [FN_R_DATA, FN_R_PAINT_END) */

/* Published by the bus-serving layer, deliberately just past the paint span
 * so fujimail.c never clears them. */
#define FN_B_TEXTGEN     0x1F0D   /* last row rendered, plus a toggle bit    */
#define FN_B_BANK        0x1F0E   /* the live bank -- a stray read can move  */
                                  /* it, so a careful client can notice      */
#define FN_B_FLAGS       0x1F0F   /* bit0 armed, bit1 claim honoured         */

/* The claim. An image carrying "FUJI" here promises it is a FujiNet client,
 * so the mailbox stays live after it boots; a booted game carries no such
 * promise and the mailbox goes dead for the session.
 *
 * Unlike the Channel F -- where every commercial Videocart is smaller than
 * the window, so the claim offset is past the end of the image and reads as
 * open bus -- a 2600 image is exactly the size of the window and its top page
 * is the game's own code and vectors. So the claim is four specific bytes
 * landing at one specific offset in a cartridge that was not built to carry
 * them; that is the same bet the ColecoVision port makes at 0x7CFC. */
#define FN_R_CLAIM       0x1F10
#define FN_R_CLAIM_LEN   4
#define FN_R_CLAIM_SIG   "FUJI"
#define FN_R_HDR         0x1F14   /* bank count, cell height, layout rev     */

/* How many bytes the path buffer holds, little-endian, republished on every
 * change. A client needs it for exactly one thing: SET_DEVICE_FULLPATH takes
 * the directory AND the filename in one 256-byte payload, so after the
 * cartridge has emitted the directory the client must know how much of the
 * payload is left to pad. Past the paint span, like the other FN_B_*. */
#define FN_B_PATHLEN     0x1F17   /* 2 bytes, little-endian                  */

/* Bumped after every blit has actually landed on the planes. A blit is a
 * single-slot request the bus core hands to core0 -- there is no queue -- so
 * a client that fires a second one before the first has run loses the first
 * on hardware and notices nothing in emulation, where the blit runs inside
 * the store. Poll this for a change before the next blit; FN_B_TEXTGEN is the
 * same thing for a text row. Past the paint span, like the other FN_B_*. */
#define FN_B_BLITGEN     0x1F19

/* $1F20-$1FFB is the client's fixed tail: the cold stub, the bank
 * trampolines, and whatever else must be reachable from every bank.
 * $1FFC-$1FFD is the RESET vector and $1FFE-$1FFF the BRK vector. */
#define FN_FIXED_TAIL    0x1F20
#define FN_VEC_RESET     0x1FFC
#define FN_VEC_BRK       0x1FFE

#define FN_R_STATUS_LINK 0x01
#define FN_R_STATUS_BUSY 0x02

/* Reply-window stability invariant: the reply and RXLEN are repainted ONLY by
 * a SEQ commit or an RXSLICE select. Between those, a client may stream bytes
 * straight out of the reply window into the TX page while building the next
 * transaction, with no bounce buffer. On a machine with 128 bytes of RAM --
 * of which the stack takes the top, because $0100-$01FF mirrors to $80-$FF --
 * this is not an optimisation, it is the only way a directory browser fits. */

/* ---------------- console -> cart: writes ---------------- */

/* Page $1D is the control page and the cart NEVER drives it; not driving it is
 * precisely what lets the cart sample the store. Offsets below 0x80 arm a
 * register, 0x80 and up are one-shot operations belonging to the layer that
 * serves the bus -- each must take effect before the next fetch, so none of
 * them can be queued through the ring. fujimail.c already treats that whole
 * half as a no-op, by design. */
#define FN_H_REGSEL      0x1D00   /* + n (n < 0x80): arm register n          */
#define FN_H_PAGE_MASK   0xFF00

/* Never console addresses. See the header comment: these only have to be page
 * numbers distinct from FN_H_REGSEL for fujimail.c's switch, and keeping them
 * out of $1000-$1FFF is worth two pages of a sixteen-page machine. */
#define FN_H_REGDATA     0x2E00   /* synthesised by a store to FN_H_COMMIT   */
#define FN_H_DATA        0x2F00   /* synthesised by a store anywhere in $1E  */
#define FN_H_PATHTX      0x3000   /* the path buffer, expanded into FN_H_DATA*/
#define FN_H_PATHRAW     0x3100   /* the same, without the padding           */

#define FN_TX_BASE       0x1E00   /* the real console page behind FN_H_DATA  */

/* One-shot operations, all in the bit7-set half of the control page. */
#define FN_HOT_BANK      0x80     /* + b: select bank b (b <= 0x6F)          */
#define FN_HOT_BANK_LAST 0xEF
#define FN_APP_MAX_PAGES 112

#define FN_HOT_TROW      0xF0     /* data = row: begin a row, reset cursor   */
#define FN_HOT_TCHR      0xF1     /* data = char: append, cursor saturates   */
#define FN_HOT_TEND      0xF2     /* data = attribute: render into the planes */

/* The blit port. Six stores -- 24 cycles -- move a screenful of bytes the
 * console has neither the RAM nor the raster time to move itself: source is
 * an offset into the reply window, destination an offset into the text
 * planes, and the transform is applied on the way. This is what lets a client
 * turn a server reply into a display without ever holding it in its 128
 * bytes of RAM, and it is why fujinet-battleship's two 10x10 boards are
 * possible at all. Issue it in vblank or overscan; a 2600 client always knows
 * where the raster is, so that costs nothing. */
#define FN_HOT_BLIT_SL   0xF5     /* source offset, low                      */
#define FN_HOT_BLIT_SH   0xF6     /* source offset, high                     */
#define FN_HOT_BLIT_DL   0xF7     /* destination offset, low                 */
#define FN_HOT_BLIT_DH   0xF8     /* destination offset, high                */
#define FN_HOT_BLIT_CNT  0xF9     /* count                                   */
#define FN_HOT_BLIT_GO   0xFA     /* data = transform; fire                  */
#define FN_BLIT_RAW      0        /* copy bytes unchanged                    */
#define FN_BLIT_TEXT     1        /* ASCII -> packed glyph pairs             */
#define FN_BLIT_FIELD    2        /* a 10x10 game field -> ten text rows      */
#define FN_BLIT_HULLS    3        /* five ship placements -> a hull overlay   */
#define FN_BLIT_SEA      4        /* fill the composed board with open sea    */
#define FN_BLIT_CELL     5        /* board[cnt] = the low byte of src         */
#define FN_BLIT_PAINT    6        /* paint the composed board into ten rows   */
#define FN_BLIT_PATH     7        /* cnt chars of the active path buffer at
                                     src, into text row dst                  */
#define FN_BLIT_TCELL    8        /* ONE character (low byte of src) into ONE
                                     cell, dst = row * FN_T_COLS + col       */
#define FN_BLIT_CARD     9        /* one seat's five cards, src = the reply
                                     offset of hand[11], dst = the TOP text
                                     row of the pair                         */
#define FN_CARD_SLOTS    5        /* five cards in a stud hand               */
#define FN_CARD_PITCH    6        /* pixels per card: 5 of art plus a gap    */
#define FN_CARD_PLANES   4        /* planes 0-3 -- pixels 0-31, the card bed */
#define FN_CARD_INK_W    5        /* pixels of art in a card                 */
#define FN_CARD_X0       2        /* left margin -- keeps ink off pixel 7,
                                     which no kernel can draw. See below.    */
#define FN_CARD_HIDE0    0x01     /* cnt: draw card 0 face-down regardless   */
#define FN_BOARD_DIM     10
#define FN_BOARD_CELLS   100
#define FN_BLIT_NOCUR    0xFF     /* FIELD: cnt = the cursor cell, or this    */

/* The PLAYFIELD board, for Battleship's four-quadrant client.
 *
 * Two 10x10 boards side by side fill a scanline exactly: a cell is two
 * playfield bits, eight pixels, so a board pair is the whole 40-bit
 * asymmetric playfield. The console rewrites PF0/PF1/PF2 twice a line out of
 * six tables indexed by CELL ROW -- 0-9 the top pair, 10-19 the bottom pair --
 * one table per register and per KIND of line: HIT (the cell has been hit),
 * MID (hit OR miss: the white core of a hit and the whole of a miss) and AUX
 * (your own hulls, and the cursor on the enemy boards). Each kind is drawn on
 * different lines of the cell in a different colour, which is how a console
 * with one playfield colour per line shows red, white and yellow cells on
 * one board without flicker.
 *
 * The tables live in the text-plane region, which nothing but a text render
 * ever writes: plane r holds REGISTER r's three tables at bytes 30-89 --
 * after text rows 0-4 and before rows 15-20. A client in board mode must
 * therefore never render rows 5-14. Register 0-2 is PF0/PF1/PF2 of the left
 * half, 3-5 of the right. Bits, per half: cells 0-1 -> PF0 bits 4-5 / 6-7,
 * cells 2-5 -> PF1 bits 7-6 / 5-4 / 3-2 / 1-0, cells 6-9 -> PF2 bits 0-1 /
 * 2-3 / 4-5 / 6-7 (the TIA draws PF0 and PF2 low bit first and PF1 high bit
 * first). A SLOT is pairrow * 2 + half: 0 top-left, 1 top-right, 2
 * bottom-left, 3 bottom-right. Every table byte is written whole and in
 * place, so the kernel's `lda table,y` never sees a half-composed byte. */
#define FN_BLIT_PFCLR    10       /* dst = slot, src low byte = a kind mask:
                                     clear those kinds' bits in the slot    */
#define FN_BLIT_PFIELD   11       /* src = reply offset of gamefield[100],
                                     dst = slot: HIT and MID from the cells
                                     (0 sea, 1 hit, 2 miss); AUX untouched   */
#define FN_BLIT_PFHULL   12       /* src = reply offset of cnt placements
                                     (pos + 100*dir), dst = slot: OR into AUX */
#define FN_BLIT_PFCELL   13       /* dst = slot, cnt = cell 0-99, src low
                                     byte = a kind mask; bit 7 set clears    */
#define FN_PF_ROW0       30       /* the first table byte of every plane     */
#define FN_PF_KIND_LEN   20       /* entries per table: cell rows 0-19       */
#define FN_PF_KINDS      3
#define FN_PF_HIT        0
#define FN_PF_MID        1
#define FN_PF_AUX        2
#define FN_PFM_HIT       0x01     /* the kind masks PFCLR and PFCELL take    */
#define FN_PFM_MID       0x02
#define FN_PFM_AUX       0x04
#define FN_PFM_ALL       0x07
#define FN_PFM_CLEAR     0x80     /* PFCELL: clear the bits instead of set   */
#define FN_PF_SLOTS      4
#define FN_PF_TAB(r, k)  (FN_T_PLANE(r) + FN_PF_ROW0 + (k) * FN_PF_KIND_LEN)

/* What FN_BLIT_FIELD paints. A Battleship gamefield is 100 bytes at y*10+x
 * in the reply window, and turning it into ten rows of text is 100 reads,
 * 100 compares and a 16-bit reply cursor -- about 250 bytes of 6502 in a bank
 * that has under a thousand to spare. Doing it here costs the client six
 * stores.
 *
 * Column 0 of each row is the row digit, columns 1-10 the cells, and the
 * cursor cell is drawn as FN_CELL_CUR so a client can blink it by simply
 * re-issuing the blit with cnt = FN_BLIT_NOCUR.
 *
 * FIELD and HULLS compose AND paint, which is the common case. SEA, CELL and
 * PAINT split the two so a client can build a board that is not in the reply
 * window at all -- which is exactly the ship-placement screen, where the five
 * placements are still in console RAM because the server has not been told
 * about them yet. Ten rows are painted once at the end rather than after
 * every cell. */
/* What FN_BLIT_CARD paints, and why a card is a TRANSFORM and not a glyph.
 *
 * A suit is not in ASCII, so the obvious move -- four new font entries -- was
 * measured and rejected: a font cell is three pixels wide, and at three
 * pixels a heart, a spade and a club are very nearly the same picture. The
 * pips that ARE distinguishable are five wide (fujinet-5cardstud's Channel F
 * port settled their shapes), and five does not fit a four-pixel cell.
 *
 * So a card ignores the cell grid entirely. Five cards sit on a SIX-pixel
 * pitch -- five of art, one of gap -- filling pixels 2-30 of the 48-pixel
 * span, which is planes 0-3. Planes 4 and 5 (columns 8-11) are never touched,
 * so a seat row carries a name or a purse beside its hand and the two halves
 * cannot collide.
 *
 * FN_CARD_X0 -- the two blank pixels the bed starts with -- is NOT taste.
 * PIXEL 7, BIT 0 OF PLANE 0, CANNOT BE DRAWN BY ANY CLIENT. The 48-pixel
 * block is six player copies and the kernel's seven GRP writes; the four late
 * ones span 33 pixels while the distance from the end of group 0 to the start
 * of group 5 is 32, so no block position serves all 48. Every position loses
 * exactly one pixel and the least bad one to lose is plane 0 bit 0 -- which
 * vcs_render_row() already spends as column 1's inter-character gap, so text
 * never notices and emu/dispcheck.py (text only) cannot see it.
 *
 * A bed at pixel 0 puts slot 1's LEFTMOST ink column on pixel 7, and slot 1
 * then draws with its left edge shaved off on every real console: a K renders
 * as a bare vertical bar, a heart as a lopsided blob. Starting at 2 moves the
 * slots to 2-6, 8-12, 14-18, 20-24 and 26-30, which clears pixel 7 and still
 * ends inside plane 3. Pixels 15 and 23 -- bit 0 of planes 1 and 2 -- ARE
 * drawable and slots 2 and 3 use them; group 0 is the only group whose tail
 * the schedule cannot reach.
 *
 * A card is TWO text rows tall: the rank on the upper row's five ink lines,
 * the suit pip on the lower row's. The sixth line of a cell is blank leading
 * and stays blank, which is what separates rank from pip for free -- and is
 * also the line the console's kernel uses to reprogram colour, so a card must
 * not write into it. This function writes whole plane bytes including that
 * line, so the bed is cleared as a side effect and a stale hand cannot show
 * through a shorter one.
 *
 * hand[11] is the wire format: five cards as two lowercase ASCII bytes each
 * (rank in "23456789tjqka", suit in "hdcs"), then a NUL. It is a C STRING --
 * bytes past the NUL are undefined -- so the first NUL rank ends the hand and
 * every later slot is blank, not garbage. "??" is a hole card and draws as a
 * hatched back; FN_CARD_HIDE0 forces slot 0 to that back whatever the wire
 * says, which is how a client masks its own hole card before the showdown
 * without the server having to lie to it.
 *
 * Ranks re-centre the cartridge's own 3x5 font glyph in the five-wide field
 * rather than carrying a second alphabet, so a rank and a letter can never
 * disagree. The ten is the single exception: "T" is poker shorthand, not a
 * number, and five wide is exactly enough to spell it. */
#define FN_CELL_SEA      '.'      /* 0: unknown water                        */
#define FN_CELL_HIT      'X'      /* 1                                       */
#define FN_CELL_MISS     'O'      /* 2                                       */
#define FN_CELL_HULL     '#'      /* painted by FN_BLIT_HULLS                */
#define FN_CELL_CUR      '+'      /* the cursor, over whatever was there     */

/* The path buffer. SET_DEVICE_FULLPATH and OPEN_DIRECTORY both read EXACTLY
 * 256 bytes, and a directory browser has to remember which directory it is
 * in -- which on this console is impossible, because there are 128 bytes of
 * RAM and the stack owns the top of them. So the working directory lives in
 * the CARTRIDGE, built a character at a time and emitted straight into the TX
 * stream when a transaction needs it. It never occupies console RAM at all.
 *
 * FN_PATH_POP is what makes ".." cost nothing: the client does not have to
 * know where the last separator was, because the cartridge does.
 *
 * The buffer is cartridge state, so it SURVIVES A CONSOLE RESET -- there is no
 * reset line on this connector. A browser restarted by the RESET switch comes
 * back in the directory it was in, which is the behaviour you want, but a
 * client that assumes an empty buffer at startup is wrong. Reset it. */
/* FOUR BUFFERS, selected by FN_PATH_SEL0..SEL3, because CONFIG provably needs
 * four 256-byte strings live at once and the console has 128 bytes of RAM:
 *
 *   0  the working directory   (and the SSID, during WiFi setup)
 *   1  the directory filter    (and the passphrase, during WiFi setup)
 *   2  a pending copy's source path
 *   3  the edit scratch: what the on-screen keyboard is typing into
 *
 * Buffers 0 and 1 double up because WiFi setup happens before anything is
 * mounted -- there is no working directory and no filter to lose.
 *
 * OPEN_DIRECTORY forces the split between 0 and 1: its payload is
 * "path\0filter\0" in ONE 256-byte block, so both have to exist together. The
 * copy source forces 2, because it has to survive the user browsing away to a
 * destination. And 3 exists so that CANCELLING an edit is possible at all --
 * the keyboard mutates as you type, so it must not type into the original.
 *
 * FN_PATH_COMMIT and FN_PATH_SEED are that last pair's whole API: SEED copies
 * the selected buffer into the scratch to start editing it, COMMIT copies the
 * scratch back on accept, and cancel is simply neither. Four ops became two by
 * making the scratch implicit rather than naming a destination in each one.
 *
 * FN_PATH_POPCH is the text editor's backspace. FN_PATH_POP drops a whole
 * component, which is right for ".." and useless for typing.
 *
 * There is deliberately no "emit just the basename" op. COPY_FILE's payload is
 * "srcfullpath|destdir", and fujiDevice.cpp:1117 appends the source's basename
 * ITSELF when the destination ends in '/' -- which a working directory always
 * does, because FN_PATH_POP keeps the trailing separator. The Intellivision
 * port does that work client-side; on this console it is free.
 *
 * The console cannot read either buffer back -- these pages are write-only.
 * FN_BLIT_PATH is how a client SEES what it has typed: the cartridge renders
 * the buffer into a text row. That is also what finally gives the browser a
 * path row, which it could never draw before.
 *
 * FN_BLIT_TCELL is the other half of making these screens usable. There is no
 * inverse video here, so a list marks its selection with a '>' in column 0,
 * and moving the cursor changes exactly two characters. Recomposing a whole
 * row to change one of them means having the row's TEXT -- and a scan list's
 * text arrives one GET_SCAN_RESULT at a time, so every cursor step would cost
 * two round trips. The Intellivision hit this and worked around it by
 * recolouring; this console has no colour per cell either. Poking the one
 * cell costs nothing, because the text planes CAN be read back as bits even
 * though they cannot be read back as characters. */
#define FN_HOT_PATH_CH   0xF3     /* data = char: append to the path buffer  */
#define FN_HOT_PATH_OP   0xF4     /* data = FN_PATH_*: act on the buffer     */
#define FN_PATH_RST      0        /* empty it                                */
#define FN_PATH_POP      1        /* drop the last component, keeping "/"    */
#define FN_PATH_TX       2        /* emit it into TX, NUL-padded to 256      */
#define FN_PATH_TXRAW    3        /* emit just its bytes; the client pads    */
#define FN_PATH_POPCH    4        /* drop ONE character                      */
#define FN_PATH_SEL0     5        /* subsequent ops act on buffer 0          */
#define FN_PATH_SEL1     6        /* ...on buffer 1                          */
#define FN_PATH_SEL2     7        /* ...on buffer 2                          */
#define FN_PATH_SEL3     8        /* ...on buffer 3, the edit scratch        */
#define FN_PATH_COMMIT   9        /* selected <- the edit scratch            */
#define FN_PATH_SEED     10       /* the edit scratch <- selected            */
#define FN_PATH_MAX      256
#define FN_PATH_BUFS     4
#define FN_PATH_SCRATCH  3        /* the buffer COMMIT and SEED work through */

#define FN_HOT_ARM1      0xFC     /* data must be FN_ARM_MAGIC1, then...     */
#define FN_HOT_ARM2      0xFD     /* ...FN_ARM_MAGIC2: decode goes live      */
#define FN_HOT_SWAP      0xFE     /* serve the staged image; armed-only      */
#define FN_HOT_COMMIT    0xFF     /* commit the armed register with data     */

#define FN_H_COMMIT      (FN_H_REGSEL + FN_HOT_COMMIT)   /* $1DFF */

/* The arming gate. PlusCart carries one at $1FF4 (../src/cartridge_firmware.c
 * pp, `comms_enabled`) because a 7800's BIOS probes cartridge space on
 * startup and would otherwise trip the hotspots. Ours is stronger: pages $1D
 * and $1E are ALWAYS tri-stated, so a probe reads the floating bus and moves
 * on, and the decode itself stays dead until an ordered pair of stores with
 * two specific values arrives -- which no probe produces. Disarmed again when
 * an image that does not claim the mailbox boots. */
#define FN_ARM_MAGIC1    0xB5
#define FN_ARM_MAGIC2    0x4A

/* Registers, reached by arming FN_H_REGSEL + n and committing.
 * 0x00-0x13 is the O2/Astrocade/Arcadia/Coleco/Channel F numbering, unchanged
 * so that fujimail.c compiles here byte-identically. */
#define FN_REG_DEVICE    0x00     /* FujiBus device id, e.g. 0x70            */
#define FN_REG_CMD       0x01     /* FujiBus command id                      */
#define FN_REG_NPARAM    0x02     /* number of parameters in the TX stream   */
#define FN_REG_DATA_RST  0x05     /* any value: rewind the TX write pointer  */
#define FN_REG_RXSLICE   0x06     /* which reply slice FN_R_DATA shows       */
#define FN_REG_SEQ       0x10     /* nonzero, != ACKSEQ: launch transaction  */
#define FN_REG_BOOTLOCK  0x11     /* FN_BOOTLOCK_MAGIC: arm the ROM swap     */
#define FN_REG_BOOTSEL_1 0x12     /* FN_BOOTSEL_MAGIC1, then...              */
#define FN_REG_BOOTSEL_2 0x13     /* ...FN_BOOTSEL_MAGIC2: reboot to BOOTSEL */

#define FN_BOOTLOCK_MAGIC 0xB5
#define FN_BOOTSEL_MAGIC1 0xB5
#define FN_BOOTSEL_MAGIC2 0x4A

/* The FN_H_DATA stream is, in order:
 *     NPARAM x { size byte (1|2|4), then that many value bytes, little-endian }
 *     then the raw payload
 * Identical to every sibling's stream. A write ANYWHERE in $1E00-$1EFF
 * appends, so `sta $1E00,x` works for any X -- and because the base low byte
 * is $00 the index can never carry, which means the dummy read that STA abs,X
 * always performs lands on the same address and is inert. */
#define FN_TX_MAX        320

/* THE ONE RULE THAT WILL BITE, and it is new to this console: NEVER use a
 * read-modify-write instruction on a write port. INC/DEC/ASL/LSR/ROL/ROR on
 * an absolute address are THREE bus cycles at that address -- read, write the
 * OLD value, write the new one -- so "the last value before the address
 * changed" samples an indeterminate point in that burst, and the cart has no
 * clock pin with which to count cycles and do better. Only STA/STX/STY may
 * target $1D00-$1EFF. tools/checkrom.py disassembles the image and fails the
 * build on any other opcode reaching those pages. */

/* DBC push stream ids, matching lib/media/rs232/diskTypeROM.cpp. Stream 1 is
 * the optional .cfg sibling; this console has real mapper variants to choose
 * between, so like the ColecoVision it actually reads it -- see vcsmap.c. */
#define FN_STREAM_ROM    0
#define FN_STREAM_CFG    1

/* FN_R_BOOT_STATE values. */
#define FN_BOOT_IDLE     0
#define FN_BOOT_XFER     1
#define FN_BOOT_READY    2        /* image staged; arm BOOTLOCK, then swap   */
#define FN_BOOT_FAILED   0x80

/* FN_R_BOOT_ERR values. */
#define FN_BOOT_ERR_TOOBIG    1
#define FN_BOOT_ERR_TRUNCATED 2
#define FN_BOOT_ERR_NOMAP     3
#define FN_BOOT_ERR_STOREBUSY 4

/* FN_R_ERR values; mirrors fb_status_t in fujibus.h. */
#define FN_ERR_OK        0
#define FN_ERR_NOLINK    1
#define FN_ERR_TIMEOUT   2
#define FN_ERR_BADFRAME  3
#define FN_ERR_TOOBIG    4

/* Booting a staged image, from the client's side:
 *   1. poll FN_R_BOOT_STATE until FN_BOOT_READY;
 *   2. write FN_REG_BOOTLOCK = FN_BOOTLOCK_MAGIC to arm the swap;
 *   3. copy the swap stub into zero-page RAM and JMP to it;
 *   4. the stub stores anything to $1DFE: the cart flips the window to the
 *      staged image between that store and the next fetch, which is safe
 *      because the next fetch is at $00xx, in RAM, with A12 low;
 *   5. the stub does JMP ($FFFC) -- which mirrors to $1FFC, the new image's
 *      own reset vector, exactly what the 6507 fetches at power-on.
 *
 * The stub must run from RAM, because the swap replaces every byte of the
 * window including the code that triggered it. This console makes that the
 * tightest of the family: the ONLY RAM is $80-$FF and $0100-$01FF mirrors
 * into it, so the stub and the stack are the same 128 bytes -- the client
 * must keep SP above the stub, and the copier checks that at run time rather
 * than corrupting itself.
 *
 * Three stores in the stub are easy to leave out and each breaks a different
 * thing: AUDV0/AUDV1, or a tone screams until the next game's cold start
 * runs; and SWACNT/SWBCNT back to inputs, because almost no game writes them
 * and a client that left the RIOT ports as outputs bricks every game booted
 * after it.
 *
 * There is no NMI on the 6507 and BRK is held off by SEI, so unlike the
 * ColecoVision there is no interrupt to dance around, and unlike the
 * Astrocade no per-bank sentinel is needed: $1FFC lives in the fixed half and
 * points at a cold stub that re-selects bank 0, so a console RESET is
 * survivable from any bank.
 *
 * CRITICAL, inherited from every bring-up before this one: the client derives
 * its next sequence number from the cart's own persisted FN_R_ACKSEQ + 1
 * (wrapping 255 -> 1; 0 is reserved as "never used"), never from a
 * program-local counter. A console RESET restarts the client and re-zeroes
 * its variables but does NOT reset the cart -- and on this console the cart
 * cannot even see that reset, because the cartridge port carries no reset
 * line and the RESET switch is just a bit in a RIOT register. */

#endif /* FUJI_MAILBOX_H */
