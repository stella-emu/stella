/* vcs_cart.h -- the 6507 bus as seen from inside an Atari 2600 cartridge.
 *
 * This lives in the header as static inline so core1, the MAME cart device and
 * the host tests all compile ONE implementation -- the ColecoVision pattern.
 * The device and the cartridge then cannot disagree about which store is a
 * register write, which is the class of bug that costs days.
 *
 * WHAT THE CART CAN AND CANNOT SEE
 *
 * The connector carries A0-A12, D0-D7, +5V and GND. That is all. There is no
 * R/W line, no clock, no chip select beyond A12, and no reset. Two
 * consequences run through everything below:
 *
 *   1. A READ OF A WRITE PORT IS INDISTINGUISHABLE FROM A WRITE TO IT. The
 *      cart sees an address and a settled data bus, nothing more. So the
 *      REGSEL/REGDATA arm-then-commit pair is kept and is a real defence here,
 *      not the formality it is on the Channel F (which can tell ROMC 05 from
 *      ROMC 02). A single stray access mutates nothing: an arm with no commit
 *      is discarded by the next arm, and a commit with nothing armed is a
 *      no-op in fujimail.c.
 *
 *   2. The cart recovers a written byte by parking on the stable address and
 *      keeping the second-to-last data sample. That is PlusROM's and the
 *      Superchip's technique, shipping on real hardware for years; see
 *      ../src/cartridge_emulation.cpp. It means the write pages must NEVER be
 *      driven -- not driving them is precisely what lets the cart sample them.
 *
 * The one structural gift: THE 6507 HAS NO INTERRUPTS. MAME's own m6507.cpp
 * says it -- "28-pin package, address bus is 13 bits, no NMI, no SO, no SYNC"
 * -- and nothing on the 2600 is wired to /IRQ. Only BRK reaches $1FFE. Every
 * stray access therefore originates in the client's own instruction stream and
 * is statically analysable, unlike the ColecoVision where the vblank NMI lands
 * mid-transaction by construction. tools/checkrom.py can be a proof rather
 * than a heuristic.
 */
#ifndef VCS_CART_H
#define VCS_CART_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fuji_mailbox.hxx"

/* VCS_CART_HOST_MAPPER: the HOST emulator owns a booted game's mapper.
 *
 * A cartridge has to be every board it might ever serve, so it carries
 * vcsmap. An emulator already is: Stella has sixty-odd schemes where vcsmap
 * has nine, and hands a pushed image to its own CartDetector/CartCreator.
 * Under this define the decode below keeps only the FujiNet layout -- once
 * the claim is absent the host has re-pointed its own bus at the real
 * cartridge class and nothing here is consulted again.
 *
 * The struct members and the five call sites are the whole difference; the
 * mailbox, the bus decode and the write sampling stay shared, which is the
 * point of this header. */
#ifdef VCS_CART_HOST_MAPPER
/* Longest scheme name a .cfg sibling can carry, NUL included. */
#define VCS_CART_CFG_MAX 16
#else
#include "vcsmap.hxx"
#endif

/* What a store meant, once decoded. */
typedef enum {
    VCS_EV_NONE = 0,
    VCS_EV_REG,      /* a completed register write: ev_a = register, ev_b = value */
    VCS_EV_TX,       /* append ev_b to the TX stream                              */
    VCS_EV_BANK,     /* map bank ev_b into the low 2K                             */
    VCS_EV_SWAP,     /* serve the staged image                                    */
    VCS_EV_TROW,     /* begin text row ev_b                                       */
    VCS_EV_TCHR,     /* append character ev_b to the row being composed           */
    VCS_EV_TEND,     /* render the composed row, attribute ev_b                   */
    VCS_EV_BLIT,     /* run the staged blit, transform ev_b                       */
    VCS_EV_ARMED,    /* the decode gate just opened                               */
    VCS_EV_PATHTX,   /* emit the path buffer into the TX stream, padded to 256     */
    VCS_EV_PATHRAW,  /* emit just the path buffer's own bytes                      */
} vcs_ev_t;

typedef struct {
    /* The 4K the console sees, $1000-$1FFF. The cart paints the reply, the
     * status page and the text planes straight into it.
     *
     * Only the FIXED half is authoritative here. The banked low half is served
     * through `lowbank` instead, because a bank switch has to be complete
     * before the console's very next fetch -- 838 ns away, with no wait state
     * on this bus -- and copying 2K takes microseconds. Every real 2600 mapper
     * swaps a pointer for the same reason. */
    uint8_t win[FN_WINDOW_SIZE];

    /* The 2K currently mapped at $1000. Points into the image. */
    const uint8_t *lowbank;

    /* A BOOTED GAME is not served from the window at all: it is served by
     * whichever real cartridge board it shipped on. The mailbox is dead by
     * then -- the image did not claim it -- so the two paths never overlap. */
#ifndef VCS_CART_HOST_MAPPER
    vcsmap_t map;
#endif

    /* The Battleship board being composed. FN_BLIT_HULLS overlays onto what
     * FN_BLIT_FIELD left here, and the text planes are packed glyph pairs
     * that cannot be read back as characters. */
    uint8_t board[FN_BOARD_CELLS];

    /* Four 256-byte strings, held here because the console has nowhere to put
     * them: the working directory, the filter, a pending copy's source, and
     * the edit scratch. Selected by FN_PATH_SEL0..SEL3; see FN_HOT_PATH_CH in
     * fuji_mailbox.h for why each one has to exist. */
    uint8_t path[FN_PATH_BUFS][FN_PATH_MAX];
    uint16_t path_len[FN_PATH_BUFS];
    uint8_t path_sel;

    /* A scheme named by the pushed image's .cfg sibling, if it had one. */
#ifdef VCS_CART_HOST_MAPPER
    /* The name verbatim, for the host to map onto its own scheme enum.
     * Spent by the next vcs_set_image() exactly as the typed hint is. */
    char cfg_name[VCS_CART_CFG_MAX];
#else
    vcsmap_kind_t hint;
    bool hint_sc;
#endif

    const uint8_t *image;     /* client or booted game                          */
    uint32_t image_size;
    uint8_t bank;             /* which 2K of the image is mapped low            */

    bool mailbox;             /* the served image claimed the mailbox           */
    bool armed;               /* the decode gate (see FN_HOT_ARM1/ARM2)         */
    uint8_t arm_step;         /* how far through the two-store arming sequence  */
    bool swap_armed;          /* FN_REG_BOOTLOCK was written                    */

    /* The armed register. Arm-then-commit is the defence on this console: a
     * lone stray access carries no value (an arm ignores its data) and a lone
     * commit has nothing armed, so ONE stray mutates nothing. Disarmed after
     * a single use, exactly as fujimail.c's own regsel is. */
    uint8_t regsel;
    bool regsel_armed;

    /* Staged blit parameters, latched by the FN_HOT_BLIT_* stores. */
    uint16_t blit_src, blit_dst;
    uint8_t blit_cnt;

    /* The text row being composed, between FN_HOT_TROW and FN_HOT_TEND.
     * This lives here rather than in each caller so the cartridge and the
     * MAME device cannot drift on it -- the compositor itself is
     * vcs_render_row(), which both call with exactly these three fields. */
    uint8_t trow;
    uint8_t tbuf[FN_T_COLS];
    uint8_t tlen;
} vcs_mem_t;

/* Recover the byte a store put on the data bus.
 *
 * With no R/W line and no clock, the cart parks on the stable address and
 * samples D0-D7 in a tight loop until the address changes, keeping the
 * SECOND-TO-LAST sample -- the last is taken as the bus is already turning
 * over. Verbatim shape from ../src/cartridge_emulation.cpp:
 *
 *     while (ADDR_IN == addr) { data_prev = data; data = DATA_IN; }
 *     append(data_prev);
 *
 * This is the one part of the port MAME cannot exercise: its cart device is
 * handed a clean data byte on a write, so the sampling never runs there.
 * test_busio.c is the substitute, and the backstop is that this exact idiom
 * ships in every PlusROM game and every Superchip cartridge.
 */
static inline uint8_t vcs_recover(const uint8_t *samples, unsigned n)
{
    if (n >= 2)
        return samples[n - 2];
    return n ? samples[0] : 0xFFu;
}

/* Console address -> window offset. A12 is the whole chip select. */
static inline bool vcs_selected(uint16_t a)
{
    return (a & 0x1000u) != 0;
}

static inline uint16_t vcs_off(uint16_t a)
{
    return (uint16_t)(a & 0x0FFFu);
}

/* The control and TX pages are never driven. On real hardware a read here
 * returns the floating bus; the client must never do it, and checkrom.py
 * fails any image that does. */
static inline bool vcs_tristate(uint16_t a)
{
    uint16_t p = (uint16_t)(a & 0x0F00u);
    return p == (FN_H_REGSEL & 0x0F00u) || p == (FN_TX_BASE & 0x0F00u);
}

/* `commit` gates the side effects a booted game's mapper has on a READ --
 * every classic 2600 scheme switches banks from the address, so a debugger
 * peek would move the bank under the running game. MAME's device passes
 * !side_effects_disabled(); core1 passes true. */
static inline uint8_t vcs_read_ex(const vcs_mem_t *m, uint16_t a, bool commit)
{
    if (!vcs_selected(a))
        return 0xFF;
#ifdef VCS_CART_HOST_MAPPER
    (void)commit;             /* no read here has a side effect; see below */
    if (vcs_tristate(a))
        return 0xFF;          /* open bus; nothing here is ours to drive */
#else
    if (m->map.kind == VCSMAP_FUJI && vcs_tristate(a))
        return 0xFF;          /* open bus; nothing here is ours to drive */

    if (m->map.kind != VCSMAP_FUJI) {
        int b = vcsmap_serve((vcsmap_t *)&m->map, a, 0xFFu, commit);
        return (b >= 0) ? (uint8_t)b : 0xFFu;
    }
#endif

    uint16_t off = vcs_off(a);
    if (off < FN_BANK_SIZE)
        return m->lowbank ? m->lowbank[off] : 0xFFu;
    return m->win[off];
}

static inline uint8_t vcs_read(const vcs_mem_t *m, uint16_t a)
{
    return vcs_read_ex(m, a, true);
}

/* Republish the path length where the console can read it. Two stores in the
 * bus loop, and they buy the client the one thing it cannot work out for
 * itself: how much of a 256-byte payload the cartridge just filled. */
static inline void vcs_publish_pathlen(vcs_mem_t *m)
{
    uint16_t n = m->path_len[m->path_sel];

    m->win[FN_B_PATHLEN - FN_WINDOW_BASE] = (uint8_t)(n & 0xFFu);
    m->win[FN_B_PATHLEN + 1 - FN_WINDOW_BASE] = (uint8_t)(n >> 8);
}

/* Byte i of the path payload: the buffer, then NUL padding to exactly 256.
 *
 * The padding is not a rounding. OPEN_DIRECTORY and SET_DEVICE_FULLPATH both
 * read exactly 256 bytes and a short payload fails the server's read; every
 * port in this family has paid for that once. It lives here so core0 and the
 * MAME model cannot disagree about it. */
static inline uint8_t vcs_path_byte(const vcs_mem_t *m, unsigned i)
{
    return (i < m->path_len[m->path_sel]) ? m->path[m->path_sel][i] : 0u;
}

/* An access the cartridge does NOT answer -- A12 is low, so something else on
 * the bus is driving -- but which its board may still be watching.
 *
 * Only UA and FE need this, and vcsmap_init flags them. It costs the bus loop
 * one boolean test per low cycle, which is most cycles; anything more than
 * that here is paid on every zero-page access the game makes. */
static inline void vcs_watch(vcs_mem_t *m, uint16_t a, uint8_t data)
{
#ifdef VCS_CART_HOST_MAPPER
    /* The host's own cartridge class claims these addresses when it needs
     * them -- Stella's CartridgeUA takes $0220/$0240 and CartridgeFE page
     * $01C0 -- so there is nothing for us to watch. */
    (void)m; (void)a; (void)data;
#else
    if (m->map.watch_low)
        (void)vcsmap_serve(&m->map, a, data, true);
#endif
}

/* A store into cart space. Returns what it meant; ev_a and ev_b carry the
 * operands. Plain stores into the painted window are DROPPED rather than
 * allowed to corrupt a reply the console is halfway through reading. */
static inline vcs_ev_t vcs_write(vcs_mem_t *m, uint16_t a, uint8_t v,
                                 uint8_t *ev_a, uint8_t *ev_b)
{
#ifndef VCS_CART_HOST_MAPPER
    if (m->map.kind != VCSMAP_FUJI) {
        /* A booted game. Its mapper decodes the store -- real hardware cannot
         * tell a store from a fetch, which is exactly why every classic scheme
         * keys on the address. */
        vcsmap_write(&m->map, a, v, true);
        return VCS_EV_NONE;
    }
#endif

    if (!vcs_selected(a) || !m->mailbox)
        return VCS_EV_NONE;

    uint16_t off = vcs_off(a);
    uint16_t page = (uint16_t)(off & 0x0F00u);

    if (page == (FN_TX_BASE & 0x0FFFu)) {
        if (!m->armed)
            return VCS_EV_NONE;
        /* Anywhere in the page appends, so `sta $1E00,x` works for any X --
         * and because the base low byte is $00 the index can never carry, so
         * the dummy read STA abs,X always performs lands on the same address
         * and is inert. */
        *ev_b = v;
        return VCS_EV_TX;
    }

    if (page != (FN_H_REGSEL & 0x0FFFu))
        return VCS_EV_NONE;   /* painted window and banked ROM swallow stores */

    uint8_t low = (uint8_t)(off & 0xFFu);

    /* The arming gate. A 7800's BIOS probes cartridge space on startup, and
     * PlusCart carries a gate at $1FF4 for exactly that reason. Ours is
     * stronger: the pages are always tri-stated, so a probe reads the floating
     * bus and moves on, and the decode stays dead until an ORDERED PAIR of
     * stores carrying two specific values arrives -- which no probe produces. */
    if (low == FN_HOT_ARM1) {
        m->arm_step = (v == FN_ARM_MAGIC1) ? 1 : 0;
        return VCS_EV_NONE;
    }
    if (low == FN_HOT_ARM2) {
        if (m->arm_step == 1 && v == FN_ARM_MAGIC2) {
            m->arm_step = 0;
            if (!m->armed) {
                m->armed = true;
                return VCS_EV_ARMED;
            }
        } else {
            m->arm_step = 0;
        }
        return VCS_EV_NONE;
    }
    m->arm_step = 0;
    if (!m->armed)
        return VCS_EV_NONE;

    if (low < 0x80) {
        /* Arm register `low`. The data is deliberately ignored: the value
         * arrives with the commit, so a lone stray cannot carry one. */
        m->regsel = low;
        m->regsel_armed = true;
        return VCS_EV_NONE;
    }

    /* The bit7-set half is one-shot operations, and none of them disturbs an
     * armed register -- so a stray in this page cannot break up a legitimate
     * pair in the other half. fujimail.c states the same rule for its own. */
    if (low >= FN_HOT_BANK && low <= FN_HOT_BANK_LAST) {
        *ev_b = (uint8_t)(low - FN_HOT_BANK);
        return VCS_EV_BANK;
    }

    switch (low) {
    case FN_HOT_TROW:
        m->trow = v;
        m->tlen = 0;
        *ev_b = v;
        return VCS_EV_TROW;
    case FN_HOT_TCHR:
        /* Saturates rather than wrapping: a client that streams an over-long
         * filename gets it truncated, never the next row corrupted. */
        if (m->tlen < FN_T_COLS)
            m->tbuf[m->tlen++] = v;
        *ev_b = v;
        return VCS_EV_TCHR;
    case FN_HOT_TEND:  *ev_b = v; return VCS_EV_TEND;

    case FN_HOT_PATH_CH:
        /* Saturates rather than wrapping, for the same reason FN_HOT_TCHR
         * does: an over-long path is truncated, never a buffer corrupted. */
        if (m->path_len[m->path_sel] < FN_PATH_MAX)
            m->path[m->path_sel][m->path_len[m->path_sel]++] = v;
        vcs_publish_pathlen(m);
        return VCS_EV_NONE;

    case FN_HOT_PATH_OP:
        if (v == FN_PATH_RST) {
            m->path_len[m->path_sel] = 0;
        } else if (v == FN_PATH_POP) {
            /* "/a/b/c/" -> "/a/b/", "/a/" -> "/", "/" -> "/". A directory is
             * held WITH its trailing separator, so appending a name to it is
             * one store and needs no separator logic on the console. */
            uint16_t *n = &m->path_len[m->path_sel];
            const uint8_t *b = m->path[m->path_sel];

            if (*n > 1u && b[*n - 1u] == (uint8_t)'/')
                (*n)--;
            while (*n > 1u && b[*n - 1u] != (uint8_t)'/')
                (*n)--;
        } else if (v == FN_PATH_POPCH) {
            /* The editor's backspace. Bottoms out at empty rather than
             * wrapping to 65535 -- a client that backspaces one key too many
             * is ordinary, not an error. */
            if (m->path_len[m->path_sel] > 0u)
                m->path_len[m->path_sel]--;
        } else if (v >= FN_PATH_SEL0 && v <= FN_PATH_SEL3) {
            m->path_sel = (uint8_t)(v - FN_PATH_SEL0);
        } else if (v == FN_PATH_COMMIT) {
            /* Accept an edit: the selected buffer becomes what was typed. */
            if (m->path_sel != FN_PATH_SCRATCH) {
                unsigned n = m->path_len[FN_PATH_SCRATCH];
                unsigned i;

                for (i = 0; i < n; i++)
                    m->path[m->path_sel][i] = m->path[FN_PATH_SCRATCH][i];
                m->path_len[m->path_sel] = (uint16_t)n;
            }
        } else if (v == FN_PATH_SEED) {
            /* Begin editing the selected buffer's current value. Cancel is
             * then simply never issuing COMMIT -- the original is untouched
             * the whole time, which is what the Intellivision needed a
             * separate 256-byte scratch copy in console RAM to achieve. */
            if (m->path_sel != FN_PATH_SCRATCH) {
                unsigned n = m->path_len[m->path_sel];
                unsigned i;

                for (i = 0; i < n; i++)
                    m->path[FN_PATH_SCRATCH][i] = m->path[m->path_sel][i];
                m->path_len[FN_PATH_SCRATCH] = (uint16_t)n;
            }
        } else if (v == FN_PATH_TX) {
            return VCS_EV_PATHTX;
        } else if (v == FN_PATH_TXRAW) {
            return VCS_EV_PATHRAW;
        }
        /* A select changes which length is published, so this has to run for
         * every op, not just the mutating ones. */
        vcs_publish_pathlen(m);
        return VCS_EV_NONE;
    case FN_HOT_BLIT_SL: m->blit_src = (uint16_t)((m->blit_src & 0xFF00u) | v); return VCS_EV_NONE;
    case FN_HOT_BLIT_SH: m->blit_src = (uint16_t)((m->blit_src & 0x00FFu) | ((uint16_t)v << 8)); return VCS_EV_NONE;
    case FN_HOT_BLIT_DL: m->blit_dst = (uint16_t)((m->blit_dst & 0xFF00u) | v); return VCS_EV_NONE;
    case FN_HOT_BLIT_DH: m->blit_dst = (uint16_t)((m->blit_dst & 0x00FFu) | ((uint16_t)v << 8)); return VCS_EV_NONE;
    case FN_HOT_BLIT_CNT: m->blit_cnt = v; return VCS_EV_NONE;
    case FN_HOT_BLIT_GO: *ev_b = v; return VCS_EV_BLIT;
    case FN_HOT_SWAP:  return m->swap_armed ? VCS_EV_SWAP : VCS_EV_NONE;
    case FN_HOT_COMMIT:
        /* A commit with nothing armed is a no-op, and the arm is spent. */
        if (!m->regsel_armed)
            return VCS_EV_NONE;
        m->regsel_armed = false;
        *ev_a = m->regsel;
        *ev_b = v;
        return VCS_EV_REG;
    default:           return VCS_EV_NONE;
    }
}

/* Map bank b of the image into $1000-$17FF. The high 2K is never banked: it
 * carries the mailbox and the vectors, so $1FFC stays reachable from every
 * bank and a console RESET is survivable whatever is mapped low.
 *
 * ONE POINTER ASSIGNMENT, deliberately. This runs inline in core1's bus loop
 * because the switch must be complete before the next fetch; anything that
 * touches 2K of memory here would miss bus cycles, and a missed cycle on this
 * bus does not slow the console down, it serves it the wrong byte.
 *
 * The fixed half's last bank -- the one carrying the mailbox -- is never a
 * legal selection, so an out-of-range bank serves open bus rather than
 * folding back onto something that looks like code. */
static inline void vcs_set_bank(vcs_mem_t *m, uint8_t b)
{
    uint32_t base = (uint32_t)b * FN_BANK_SIZE;

    m->bank = b;
    if (m->image && base + FN_BANK_SIZE <= m->image_size - FN_BANK_SIZE)
        m->lowbank = m->image + base;
    else if (m->image && m->image_size == 2u * FN_BANK_SIZE && b == 0)
        m->lowbank = m->image;          /* a flat client: one bank, and it is 0 */
    else
        m->lowbank = NULL;              /* open bus */
    m->win[FN_B_BANK - FN_WINDOW_BASE] = b;
}

/* Serve `image`. The fixed half is always the LAST 2K of the image, so a
 * client is (N+1) * 2048 bytes: N banks then the fixed half. */
static inline void vcs_set_image(vcs_mem_t *m, const uint8_t *img, uint32_t len)
{
    m->image = img;
    m->image_size = len;

    memset(m->win, 0xFF, FN_WINDOW_SIZE);
    if (img && len >= FN_BANK_SIZE)
        memcpy(m->win + FN_BANK_SIZE, img + (len - FN_BANK_SIZE), FN_BANK_SIZE);

    /* An image claiming "FUJI" promises the mailbox pages are not its own
     * code, so decode may stay live after it boots. A game carries no such
     * promise and the mailbox goes dead for the session. */
    m->mailbox = img && len >= FN_BANK_SIZE &&
                 memcmp(m->win + (FN_R_CLAIM - FN_WINDOW_BASE),
                        FN_R_CLAIM_SIG, FN_R_CLAIM_LEN) == 0;
    m->armed = false;
    m->arm_step = 0;
    m->swap_armed = false;
    m->regsel_armed = false;
    vcs_set_bank(m, 0);
    m->win[FN_B_FLAGS - FN_WINDOW_BASE] = m->mailbox ? 0x02 : 0x00;

    /* The claim decides which world this image lives in. A client gets our
     * banked-low/fixed-high layout and a live mailbox; anything else is a
     * GAME and gets the real board it shipped on. */
#ifndef VCS_CART_HOST_MAPPER
    if (m->mailbox) {
        vcsmap_init(&m->map, VCSMAP_FUJI, img, len, false);
    } else {
        /* The .cfg hint wins where it exists, because size alone cannot tell
         * an 8K F8 from an 8K E0, UA or FE. Where it does not, fall back to
         * size -- which is how MAME's identify_cart_type() starts too. */
        vcsmap_kind_t k = m->hint;
        bool sc = m->hint_sc;
        if (k == VCSMAP_NONE) {
            k = vcsmap_detect(img, len);
            sc = vcsmap_has_superchip(img, len);
        }
        vcsmap_init(&m->map, k, img, len, sc);
    }
#endif
    /* Spend the hint. The ESP32 sends a .cfg only when the file exists, so a
     * mount with no sibling sends nothing at all -- and a hint left standing
     * would be applied to the NEXT image, serving an E0 board for a plain F8
     * game. Coleco learned this one the same way. */
#ifdef VCS_CART_HOST_MAPPER
    m->cfg_name[0] = '\0';
#else
    m->hint = VCSMAP_NONE;
    m->hint_sc = false;
#endif
}

/* Take the scheme from a .cfg sibling. Applies to the next image served and
 * to that one only. */
static inline void vcs_set_cfg(vcs_mem_t *m, const char *cfg, unsigned len)
{
#ifdef VCS_CART_HOST_MAPPER
    unsigned n = 0;

    if (cfg) {
        /* Trim at the first control character: the sibling is a text file and
         * arrives with whatever line ending it was written with. */
        while (n < len && n + 1 < VCS_CART_CFG_MAX &&
               (unsigned char)cfg[n] > 0x20u)
            n++;
        memcpy(m->cfg_name, cfg, n);
    }
    m->cfg_name[n] = '\0';
#else
    m->hint = (cfg && len) ? vcsmap_from_name(cfg, len) : VCSMAP_NONE;
    m->hint_sc = (m->hint != VCSMAP_NONE) && vcsmap_name_is_sc(cfg, len);
#endif
}

#endif /* VCS_CART_H */
