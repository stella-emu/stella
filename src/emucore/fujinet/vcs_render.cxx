/* vcs_render.c -- see vcs_render.h. */

#include <string.h>

#include "fuji_mailbox.hxx"
#include "vcs_font.hxx"
#include "vcs_render.hxx"

/* The five ink rows of one character, as 3-bit values, MSB leftmost.
 * Lowercase folds to uppercase for now; the Channel F port derives real
 * lowercase by squashing to four rows and that is the follow-up when a
 * password field needs it. Anything outside the table renders as '?' rather
 * than reading off the end. */
static const uint8_t *glyph(uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        c = (uint8_t)(c - 'a' + 'A');
    if (c < VCS_FONT_FIRST || c > VCS_FONT_LAST)
        c = '?';
    return vcs_font[c - VCS_FONT_FIRST];
}

void vcs_render_row(uint8_t *win, uint8_t row, const uint8_t *text, uint8_t len)
{
    unsigned g, s;

    if (row >= FN_T_ROWS)
        return;

    for (g = 0; g < FN_T_PLANES; g++) {
        unsigned lc = g * 2, rc = g * 2 + 1;
        const uint8_t *l = glyph(lc < len ? text[lc] : (uint8_t)' ');
        const uint8_t *r = glyph(rc < len ? text[rc] : (uint8_t)' ');
        /* Plane base, then the absolute scanline within it. */
        uint8_t *p = win + (FN_T_PLANE(g) - FN_WINDOW_BASE) + row * FN_T_CELL_H;

        for (s = 0; s < FN_T_CELL_H; s++) {
            /* Left column in bits 7-5 with bit 4 as its inter-character gap,
             * right column in bits 3-1 with bit 0 as its gap. The last row of
             * the cell is blank leading. */
            p[s] = (s < VCS_FONT_INK_H)
                     ? (uint8_t)((l[s] << 5) | (r[s] << 1))
                     : 0u;
        }
    }
}

void vcs_render_clear(uint8_t *win)
{
    unsigned g;

    for (g = 0; g < FN_T_PLANES; g++)
        memset(win + (FN_T_PLANE(g) - FN_WINDOW_BASE), 0, FN_T_PLANE_LEN);
}

/* Ship lengths, longest first, matching the server's myShips[] order. */
static const uint8_t ship_len[5] = { 5, 4, 3, 3, 2 };

/* ---------------- the playfield board ----------------
 *
 * Where cell x of a half lands: which of PF0/PF1/PF2, and its two bits. PF0
 * is drawn from bit 4 upward, PF1 from bit 7 downward, PF2 from bit 0
 * upward -- the TIA's own order, which is why the masks are not monotonic. */
static const uint8_t pf_reg[FN_BOARD_DIM]  = { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 };
static const uint8_t pf_bits[FN_BOARD_DIM] = { 0x30, 0xC0,
                                               0xC0, 0x30, 0x0C, 0x03,
                                               0x03, 0x0C, 0x30, 0xC0 };

/* The table byte for (slot, kind, register-within-half, cell row). */
static uint8_t *pf_byte(uint8_t *win, unsigned slot, unsigned kind,
                        unsigned reg, unsigned y)
{
    unsigned r = reg + 3u * (slot & 1u);
    unsigned entry = (slot >> 1) * FN_BOARD_DIM + y;

    return win + (FN_PF_TAB(r, kind) - FN_WINDOW_BASE) + entry;
}

/* Set or clear one cell's bits in every kind the mask names. */
static void pf_cell(uint8_t *win, unsigned slot, unsigned cell, uint8_t mask,
                    int set)
{
    unsigned x = cell % FN_BOARD_DIM, y = cell / FN_BOARD_DIM, k;

    for (k = 0; k < FN_PF_KINDS; k++) {
        uint8_t *b;

        if (!(mask & (1u << k)))
            continue;
        b = pf_byte(win, slot, k, pf_reg[x], y);
        if (set)
            *b |= pf_bits[x];
        else
            *b &= (uint8_t)~pf_bits[x];
    }
}

/* Zero every byte of the slot's half for the kinds the mask names. */
static void pf_clear(uint8_t *win, unsigned slot, uint8_t mask)
{
    unsigned k, r, y;

    for (k = 0; k < FN_PF_KINDS; k++) {
        if (!(mask & (1u << k)))
            continue;
        for (r = 0; r < 3; r++)
            for (y = 0; y < FN_BOARD_DIM; y++)
                *pf_byte(win, slot, k, r, y) = 0;
    }
}

/* Paint the composed board into ten text rows starting at `row`: the row
 * digit, then the ten cells. */
static void board_rows(uint8_t *win, const uint8_t *board, uint8_t row)
{
    unsigned y;

    for (y = 0; y < FN_BOARD_DIM; y++) {
        uint8_t line[FN_BOARD_DIM + 1];
        unsigned x;

        line[0] = (uint8_t)('0' + y);
        for (x = 0; x < FN_BOARD_DIM; x++)
            line[x + 1] = board[y * FN_BOARD_DIM + x];
        vcs_render_row(win, (uint8_t)(row + y), line, FN_BOARD_DIM + 1);
    }
}

void vcs_render_cell(uint8_t *win, uint8_t row, uint8_t col, uint8_t c)
{
    const uint8_t *g;
    uint8_t *p;
    unsigned s;

    if (row >= FN_T_ROWS || col >= FN_T_COLS)
        return;

    g = glyph(c);
    p = win + (FN_T_PLANE(col / 2) - FN_WINDOW_BASE) + row * FN_T_CELL_H;

    for (s = 0; s < FN_T_CELL_H; s++) {
        uint8_t ink = (s < VCS_FONT_INK_H) ? g[s] : 0u;

        /* Keep the neighbour's three ink bits and both gap bits; replace only
         * this column's. The gap bits are always zero, so masking them into
         * the kept half rather than the written one costs nothing and keeps
         * the two cases symmetrical. */
        p[s] = (col & 1) ? (uint8_t)((p[s] & 0xF0u) | (ink << 1))
                         : (uint8_t)((p[s] & 0x0Fu) | (ink << 5));
    }
}

/* The console cannot read the path buffer back -- the page it is written
 * through is write-only -- so this is the only way a client sees what it has
 * typed, or which directory it is standing in. A caller showing the tail of a
 * long string passes src = len - FN_T_COLS. */
void vcs_render_path_row(uint8_t *win, const uint8_t *path, uint16_t path_len,
                         uint16_t src, uint8_t row, uint8_t cnt)
{
    uint8_t line[FN_T_COLS];
    unsigned n = 0;

    for (; n < cnt && n < FN_T_COLS; n++) {
        unsigned s = src + n;
        line[n] = (s < path_len) ? path[s] : (uint8_t)' ';
    }
    vcs_render_row(win, row, line, (uint8_t)n);
}

/* ------------------------------------------------------------------ cards --
 * See the FN_BLIT_CARD block in fuji_mailbox.h for why a card is not a glyph.
 * Every table below is five rows of five ink bits, bit 4 leftmost.
 */

/* The ten, the one rank the 3x5 font cannot spell. */
static const uint8_t card_ten[VCS_FONT_INK_H] = {
    0x17,       /* #.### */
    0x15,       /* #.#.# */
    0x15,       /* #.#.# */
    0x15,       /* #.#.# */
    0x17        /* #.### */
};

/* The pips: hearts, diamonds, spades, clubs -- and card_suits[] below MUST
 * spell that same order. Five wide is the width at
 * which all four are actually distinguishable; three is not. */
static const uint8_t card_pip[4][VCS_FONT_INK_H] = {
    { 0x0A,     /* .#.#. */
      0x1F,     /* ##### */
      0x1F,     /* ##### */
      0x0E,     /* .###. */
      0x04 },   /* ..#.. */   /* hearts   */
    { 0x04,     /* ..#.. */
      0x0E,     /* .###. */
      0x1F,     /* ##### */
      0x0E,     /* .###. */
      0x04 },   /* ..#.. */   /* diamonds */
    { 0x04,     /* ..#.. */
      0x0E,     /* .###. */
      0x1F,     /* ##### */
      0x04,     /* ..#.. */
      0x0E },   /* .###. */   /* spades   */
    { 0x04,     /* ..#.. */
      0x1B,     /* ##.## */
      0x0E,     /* .###. */
      0x04,     /* ..#.. */
      0x0E }    /* .###. */   /* clubs    */
};

static const char card_suits[] = "hdsc";   /* the order of card_pip[] */

/* A face-down card: a hatch capped top and bottom so it reads as a card back
 * rather than as ink that failed to become a glyph. A 50% hatch is used
 * rather than a sparser lattice because five pixels of width cannot carry a
 * period-3 weave -- it comes out as one or two lit pixels a row, which reads
 * as damage. [0] is the upper text row, [1] the lower. */
static const uint8_t card_back[2][VCS_FONT_INK_H] = {
    { 0x1F,     /* ##### */
      0x15,     /* #.#.# */
      0x0A,     /* .#.#. */
      0x15,     /* #.#.# */
      0x0A },   /* .#.#. */
    { 0x15,     /* #.#.# */
      0x0A,     /* .#.#. */
      0x15,     /* #.#.# */
      0x0A,     /* .#.#. */
      0x1F }    /* ##### */
};

/* One scanline of the card bed: `ink` is one five-bit row per slot. Whole
 * plane bytes are written, never merged, so the bed clears itself.
 *
 * The bed starts at FN_CARD_X0, not at zero: pixel 7 is bit 0 of plane 0 and
 * no client can draw it -- see the FN_BLIT_CARD block in fuji_mailbox.h. */
static void card_line(uint8_t *win, unsigned row, unsigned line,
                      const uint8_t *ink)
{
    uint8_t b[FN_CARD_PLANES];
    unsigned k, i;

    for (k = 0; k < FN_CARD_PLANES; k++)
        b[k] = 0u;

    if (ink) {
        for (k = 0; k < FN_CARD_SLOTS; k++) {
            for (i = 0; i < FN_CARD_INK_W; i++) {
                unsigned px = FN_CARD_X0 + k * FN_CARD_PITCH + i;

                if (ink[k] & (uint8_t)(1u << (FN_CARD_INK_W - 1 - i)))
                    b[px >> 3] |= (uint8_t)(0x80u >> (px & 7u));
            }
        }
    }

    for (k = 0; k < FN_CARD_PLANES; k++)
        win[(FN_T_PLANE(k) - FN_WINDOW_BASE) + row * FN_T_CELL_H + line] = b[k];
}

void vcs_render_cards(uint8_t *win, uint8_t row, const uint8_t *hand,
                      uint8_t flags)
{
    uint8_t top[FN_CARD_SLOTS], bot[FN_CARD_SLOTS];
    bool dealt = true;
    unsigned k, line;

    /* A card is two rows tall, so the pair must both exist. */
    if ((unsigned)row + 1u >= FN_T_ROWS)
        return;

    for (line = 0; line < FN_T_CELL_H; line++) {
        /* The sixth line of both cells is blank leading: it separates rank
         * from pip, and the console's kernel reprograms colour there. */
        if (line >= VCS_FONT_INK_H) {
            card_line(win, row, line, NULL);
            card_line(win, (unsigned)row + 1u, line, NULL);
            continue;
        }

        for (k = 0; k < FN_CARD_SLOTS; k++) {
            uint8_t r = hand[k * 2], s = hand[k * 2 + 1];
            const char *f;

            /* hand[] is a C string: the first NUL rank ends the hand and
             * everything past it is undefined, not blank. */
            if (r == 0u)
                dealt = false;

            if (!dealt) {
                top[k] = 0u;
                bot[k] = 0u;
                continue;
            }

            if (r == '?' || s == '?' ||
                (k == 0u && (flags & FN_CARD_HIDE0) != 0u)) {
                top[k] = card_back[0][line];
                bot[k] = card_back[1][line];
                continue;
            }

            if (r == 't' || r == 'T') {
                top[k] = card_ten[line];
            } else {
                /* Re-centre the font's own 3x5 glyph in the five-wide field,
                 * so a rank and a letter can never disagree. */
                top[k] = (uint8_t)(glyph(r)[line] << 1);
            }

            f = strchr(card_suits, (char)(s | 0x20));
            bot[k] = (f != NULL && s != 0u)
                       ? card_pip[(unsigned)(f - card_suits)][line]
                       : 0u;
        }

        card_line(win, row, line, top);
        card_line(win, (unsigned)row + 1u, line, bot);
        dealt = true;              /* re-walk the hand on the next line */
    }
}

bool vcs_blit(uint8_t *win, uint8_t *board, uint16_t src, uint16_t dst,
              uint8_t cnt, uint8_t transform)
{
    unsigned i;

    if (transform == FN_BLIT_RAW) {
        for (i = 0; i < cnt; i++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + i;
            unsigned d = (FN_T_BASE - FN_WINDOW_BASE) + dst + i;
            if (s < FN_WINDOW_SIZE && d < FN_WINDOW_SIZE)
                win[d] = win[s];
        }
        return true;
    }

    if (transform == FN_BLIT_TEXT) {
        uint8_t line[FN_T_COLS];
        unsigned n = 0;
        for (; n < cnt && n < FN_T_COLS; n++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + n;
            line[n] = (s < FN_WINDOW_SIZE) ? win[s] : (uint8_t)' ';
        }
        vcs_render_row(win, (uint8_t)dst, line, (uint8_t)n);
        return true;
    }

    if (transform == FN_BLIT_FIELD) {
        /* 100 bytes of gamefield at y*10+x, straight into the composed board
         * and then into ten rows. cnt is the cursor cell, or FN_BLIT_NOCUR. */
        for (i = 0; i < FN_BOARD_CELLS; i++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + i;
            uint8_t v = (s < FN_WINDOW_SIZE) ? win[s] : 0u;

            board[i] = (v == 1u) ? (uint8_t)FN_CELL_HIT
                     : (v == 2u) ? (uint8_t)FN_CELL_MISS
                                 : (uint8_t)FN_CELL_SEA;
        }
        if (cnt < FN_BOARD_CELLS)
            board[cnt] = FN_CELL_CUR;
        board_rows(win, board, (uint8_t)dst);
        return true;
    }

    if (transform == FN_BLIT_SEA) {
        memset(board, FN_CELL_SEA, FN_BOARD_CELLS);
        return true;
    }

    if (transform == FN_BLIT_CELL) {
        /* One cell, no repaint. The placement screen sets seventeen of these
         * between two paints; repainting ten rows after each would be
         * seventeen times the work for the same picture. */
        if (cnt < FN_BOARD_CELLS)
            board[cnt] = (uint8_t)(src & 0xFFu);
        return true;
    }

    if (transform == FN_BLIT_PAINT) {
        board_rows(win, board, (uint8_t)dst);
        return true;
    }

    if (transform == FN_BLIT_HULLS) {
        /* Five placement bytes: pos + 100 * dir, pos = y*10 + x, dir 0
         * horizontal and 1 vertical -- the server's encoding, unchanged.
         *
         * Hulls go on ONLY where the sea still shows. A hit or a miss on your
         * own hull is the thing you most need to see, and painting the hull
         * over it would hide exactly that. */
        for (i = 0; i < cnt && i < 5u; i++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + i;
            unsigned pos, dir, x, y, seg;

            if (s >= FN_WINDOW_SIZE)
                break;
            pos = win[s];
            dir = (pos >= FN_BOARD_CELLS) ? 1u : 0u;
            if (dir)
                pos -= FN_BOARD_CELLS;
            if (pos >= FN_BOARD_CELLS)
                continue;              /* not placed, or nonsense */
            x = pos % FN_BOARD_DIM;
            y = pos / FN_BOARD_DIM;

            for (seg = 0; seg < ship_len[i]; seg++) {
                if (x >= FN_BOARD_DIM || y >= FN_BOARD_DIM)
                    break;             /* a placement running off the edge */
                if (board[y * FN_BOARD_DIM + x] == (uint8_t)FN_CELL_SEA)
                    board[y * FN_BOARD_DIM + x] = FN_CELL_HULL;
                if (dir) y++; else x++;
            }
        }
        board_rows(win, board, (uint8_t)dst);
        return true;
    }

    if (transform == FN_BLIT_CARD) {
        /* hand[11] out of the reply window, bounds-checked, then rendered.
         * The NUL is carried too: vcs_render_cards() needs it to know where
         * the hand stops. */
        uint8_t hand[FN_CARD_SLOTS * 2 + 1];

        for (i = 0; i < sizeof hand; i++) {
            unsigned o = (FN_R_DATA - FN_WINDOW_BASE) + src + i;

            hand[i] = (o < FN_WINDOW_SIZE) ? win[o] : 0u;
        }
        vcs_render_cards(win, (uint8_t)dst, hand, cnt);
        return true;
    }

    /* The playfield board. dst is the SLOT for all four; see fuji_mailbox.h
     * for the tables and the bit map. */
    if (transform == FN_BLIT_PFCLR) {
        pf_clear(win, dst & 3u, (uint8_t)src);
        return true;
    }

    if (transform == FN_BLIT_PFIELD) {
        /* HIT and MID are rewritten for the slot from the 100 wire cells;
         * AUX is left alone, because the cursor and the hulls it carries are
         * the client's business, not the wire's. */
        pf_clear(win, dst & 3u, FN_PFM_HIT | FN_PFM_MID);
        for (i = 0; i < FN_BOARD_CELLS; i++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + i;
            uint8_t v = (s < FN_WINDOW_SIZE) ? win[s] : 0u;

            if (v == 1u)
                pf_cell(win, dst & 3u, i, FN_PFM_HIT | FN_PFM_MID, 1);
            else if (v == 2u)
                pf_cell(win, dst & 3u, i, FN_PFM_MID, 1);
        }
        return true;
    }

    if (transform == FN_BLIT_PFHULL) {
        /* The same placement bytes FN_BLIT_HULLS decodes, ORed into AUX.
         * Never clears: a caller that wants a clean slate says PFCLR first,
         * and one that is adding a cursor to a board with hulls on it does
         * not. */
        for (i = 0; i < cnt && i < 5u; i++) {
            unsigned s = (FN_R_DATA - FN_WINDOW_BASE) + src + i;
            unsigned pos, dir, x, y, seg;

            if (s >= FN_WINDOW_SIZE)
                break;
            pos = win[s];
            dir = (pos >= FN_BOARD_CELLS) ? 1u : 0u;
            if (dir)
                pos -= FN_BOARD_CELLS;
            if (pos >= FN_BOARD_CELLS)
                continue;
            x = pos % FN_BOARD_DIM;
            y = pos / FN_BOARD_DIM;
            for (seg = 0; seg < ship_len[i]; seg++) {
                if (x >= FN_BOARD_DIM || y >= FN_BOARD_DIM)
                    break;
                pf_cell(win, dst & 3u, y * FN_BOARD_DIM + x, FN_PFM_AUX, 1);
                if (dir) y++; else x++;
            }
        }
        return true;
    }

    if (transform == FN_BLIT_PFCELL) {
        if (cnt < FN_BOARD_CELLS)
            pf_cell(win, dst & 3u, cnt, (uint8_t)(src & FN_PFM_ALL),
                    !(src & FN_PFM_CLEAR));
        return true;
    }

    return false;
}
