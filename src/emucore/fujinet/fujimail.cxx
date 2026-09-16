#include <string.h>

#include "fujimail.hxx"
#include "fuji_mailbox.hxx"

#define TIMEOUT_MS        5000
#define TIMEOUT_MOUNT_MS 60000
#define LINK_WAIT_MS      3000

static const fujimail_port_t *port;

/* Hotspot-side state, rebuilt from the console's reads. */
#define REGSEL_INERT 0xFF
static uint8_t regsel;          /* REGSEL_INERT between REGSEL/REGDATA pairs */
static uint8_t prev_regnum;     /* last register a REGDATA pair completed    */
static uint8_t mb_device, mb_cmd, mb_nparam;
static uint8_t txbuf[FN_TX_MAX];
static unsigned txptr;
static uint8_t lastseq;

/* Read-side state. */
static uint8_t rxbuf[FUJIMAIL_RX_MAX];
static unsigned rxlen;
static uint8_t rxslice;

/* DBC push state; the bytes themselves live in the port's store. */
static int dbc_stream = -1;
static uint32_t stream_got;
static uint32_t stage_expect;
static bool boot_ready;

static void poke(unsigned offset, uint8_t v)
{
    port->poke(offset, v);
}

void fujimail_init(const fujimail_port_t *p)
{
    port = p;
    regsel = REGSEL_INERT;
    prev_regnum = REGSEL_INERT;
    mb_device = mb_cmd = mb_nparam = 0;
    txptr = 0;
    lastseq = 0;
    rxlen = 0;
    rxslice = 0;
    dbc_stream = -1;
    stream_got = 0;
    stage_expect = 0;
    boot_ready = false;
}

static void publish_slice(void)
{
    unsigned base = (unsigned) rxslice * FN_R_SLICE_LEN;
    unsigned i;

    for (i = 0; i < FN_R_SLICE_LEN; i++)
        poke(FN_R_DATA + i, (base + i < rxlen) ? rxbuf[base + i] : 0);
    /* Published LAST: the client selects a slice and polls this until it
     * echoes; only then is the slice above known to be whole. */
    poke(FN_R_SLICE_ECHO, rxslice);
}

void fujimail_paint(void)
{
    unsigned a;

    /* Painting republishes FN_R_ACKSEQ as 0, so the interlock has to start
     * over with it. Leaving lastseq behind would make the client's first
     * request -- ACKSEQ + 1, per the rule in fuji_mailbox.h -- collide with a
     * sequence already answered, and run_transaction would drop it in
     * silence. */
    lastseq = 0;
    for (a = FN_R_DATA; a < FN_R_PAINT_END; a++)
        poke(a, 0);
    poke(FN_R_MAGIC0, 'F');
    poke(FN_R_MAGIC1, 'N');
    poke(FN_R_PROTO_VER, FN_PROTO_VER);
    poke(FN_R_STATUS, port->link_up() ? FN_R_STATUS_LINK : 0);
}

/* ---- the DBC ROM push ----
 *
 * MediaTypeROM::mount() streams the cartridge image to device $FF while our
 * MOUNT_IMAGE is still outstanding, so these frames arrive unsolicited in the
 * middle of a transaction and each needs its own ACK.
 */

bool fujimail_inbound(const fb_reply_t *req)
{
    if (req->device != FUJI_DEVICEID_DBC)
        return false;

    if (req->command == CMD_NET_OPEN) {
        unsigned stream = (req->data_len > 0) ? req->data[0] : 0;
        uint8_t err;

        stage_expect = 0;
        if (req->data_len >= 5)
            stage_expect = (uint32_t) req->data[1]
                         | ((uint32_t) req->data[2] << 8)
                         | ((uint32_t) req->data[3] << 16)
                         | ((uint32_t) req->data[4] << 24);
        dbc_stream = (stream == 1) ? 1 : 0;
        stream_got = 0;

        if (port->on_dbc)
            port->on_dbc(FUJIMAIL_DBC_OPEN, dbc_stream, stage_expect, 0, false);

        err = port->stream_open(dbc_stream, stage_expect);
        if (err != 0) {
            /* Say so now rather than after the ESP32 has dragged the whole
             * file over TNFS -- and refusing is the only way to avoid
             * silently booting a truncated image. */
            if (dbc_stream == 0) {
                boot_ready = false;
                poke(FN_R_BOOT_STATE, FN_BOOT_FAILED);
                poke(FN_R_BOOT_ERR, err);
            }
            dbc_stream = -1;
            port->send_bare(FUJI_DEVICEID_DBC, CMD_FUJI_NAK, NULL, 0);
            return true;
        }
        if (dbc_stream == 0) {
            /* Stream 1 (the .cfg sibling, pushed first) never touches the
             * boot state: only the ROM stream's progress is the client's. */
            poke(FN_R_BOOT_STATE, FN_BOOT_XFER);
            poke(FN_R_BOOT_PCT, 0);
            poke(FN_R_BOOT_ERR, 0);
        }
        port->send_bare(FUJI_DEVICEID_DBC, CMD_FUJI_ACK, NULL, 0);
        return true;
    }

    if (req->command == CMD_NET_WRITE) {
        if (dbc_stream >= 0) {
            port->stream_write(dbc_stream, req->data, req->data_len);
            stream_got += req->data_len;
            if (dbc_stream == 0 && stage_expect)
                poke(FN_R_BOOT_PCT,
                     (uint8_t)((stream_got * 100u) / stage_expect));
        }
        port->send_bare(FUJI_DEVICEID_DBC, CMD_FUJI_ACK, NULL, 0);
        return true;
    }

    if (req->command == CMD_NET_CLOSE) {
        /* Bare CLOSE commits; payload 0x01 aborts, so partial data is never
         * booted and older peers stay compatible. */
        bool aborted = (req->data_len > 0 && req->data[0] == 0x01);
        int stream = dbc_stream;
        uint8_t err = 0;

        dbc_stream = -1;
        if (port->on_dbc)
            port->on_dbc(FUJIMAIL_DBC_CLOSE, stream, stage_expect, stream_got,
                         aborted);

        if (stream >= 0)
            err = port->stream_close(stream, stream_got, aborted);

        if (aborted) {
            boot_ready = false;
            poke(FN_R_BOOT_STATE, FN_BOOT_FAILED);
            poke(FN_R_BOOT_ERR, FN_BOOT_ERR_TRUNCATED);
        } else if (stream == 0) {
            if (err != 0) {
                /* Mapping failed at commit: never publish READY over it. */
                boot_ready = false;
                poke(FN_R_BOOT_STATE, FN_BOOT_FAILED);
                poke(FN_R_BOOT_ERR, err);
            } else {
                boot_ready = true;
                poke(FN_R_BOOT_PCT, 100);
                poke(FN_R_BOOT_STATE, FN_BOOT_READY);
            }
        }
        stream_got = 0;
        port->send_bare(FUJI_DEVICEID_DBC, CMD_FUJI_ACK, NULL, 0);
        return true;
    }

    return false;
}

/* ---- the transaction ---- */

static void run_transaction(uint8_t seq)
{
    fb_param_t params[8];
    unsigned nparam = mb_nparam > 8 ? 8 : mb_nparam;
    unsigned i, p = 0;
    fb_reply_t reply;
    fb_status_t st = FB_OK;
    uint8_t reply_cmd = 0;

    /* Unpack the TX stream: NPARAM x {size, value LE}, then payload. */
    for (i = 0; i < nparam && p < txptr; i++) {
        unsigned sz = txbuf[p++], k;

        if (sz != 1 && sz != 2 && sz != 4) {
            st = FB_EBADFRAME;
            break;
        }
        params[i].size = (uint8_t) sz;
        params[i].value = 0;
        for (k = 0; k < sz && p < txptr; k++)
            params[i].value |= (uint32_t) txbuf[p++] << (8 * k);
    }

    if (st == FB_OK) {
        /* The console reaches its first transaction long before the ESP32-S3
         * finishes enumerating, and this service runs only once per SEQ bump,
         * so a single "no link" answer here would be permanent. */
        if (port->wait_link_ms && !port->link_up())
            port->wait_link_ms(LINK_WAIT_MS);

        /* COPY_FILE gets the mount budget too: its ACK arrives only when the
         * host-side copy finishes, which for a big file over TNFS is far
         * past the ordinary window. */
        st = port->transact(mb_device, mb_cmd, params, nparam,
                            txbuf + p, (uint16_t)(txptr - p),
                            (mb_cmd == CMD_FUJI_MOUNT_IMAGE
                             || mb_cmd == CMD_FUJI_COPY_FILE) ? TIMEOUT_MOUNT_MS
                                                              : TIMEOUT_MS,
                            &reply);
    }

    rxlen = 0;
    if (st == FB_OK) {
        rxlen = reply.data_len > FUJIMAIL_RX_MAX ? FUJIMAIL_RX_MAX
                                                 : reply.data_len;
        memcpy(rxbuf, reply.data, rxlen);
        reply_cmd = reply.command;
    }

    poke(FN_R_REPLY_CMD, reply_cmd);
    poke(FN_R_ERR, (uint8_t) st);
    poke(FN_R_RXLEN_LO, (uint8_t)(rxlen & 0xFF));
    poke(FN_R_RXLEN_HI, (uint8_t)(rxlen >> 8));
    poke(FN_R_STATUS, port->link_up() ? FN_R_STATUS_LINK : 0);
    rxslice = 0;
    publish_slice();

    /* Published LAST. This single store is the whole interlock: everything
     * the console reads is already in place before it can see the sequence
     * match. */
    poke(FN_R_ACKSEQ, seq);

    if (port->on_txn) {
        fujimail_txn_t t;
        t.device = mb_device;
        t.command = mb_cmd;
        t.nparam = (uint8_t) nparam;
        t.seq = seq;
        t.reply_cmd = reply_cmd;
        t.txlen = (uint16_t)(txptr - p);
        t.rxlen = (uint16_t) rxlen;
        t.status = (int) st;
        t.rx = rxbuf;
        port->on_txn(&t);
    }
}

static void reg_write(uint8_t reg, uint8_t data)
{
    switch (reg) {
    case FN_REG_DEVICE:   mb_device = data; break;
    case FN_REG_CMD:      mb_cmd = data; break;
    case FN_REG_NPARAM:   mb_nparam = data; break;
    case FN_REG_DATA_RST: txptr = 0; break;
    case FN_REG_RXSLICE:
        rxslice = (uint8_t)(data % FN_R_NSLICES);
        publish_slice();
        break;

    case FN_REG_SEQ:
        /* Sequence 0 is reserved and an unchanged number is a no-op, so a
         * client that timed out and retried can never make us replay a
         * command -- and a refresh stray (which cannot complete a REGSEL
         * pair, let alone with a fresh value) cannot launch one. */
        if (data != 0 && data != lastseq) {
            lastseq = data;
            run_transaction(data);
        }
        break;

    case FN_REG_BOOTLOCK:
        if (data == FN_BOOTLOCK_MAGIC && boot_ready)
            port->arm_swap();
        break;

    case FN_REG_BOOTSEL_2:
        /* Only as the second half of the two-pair sequence; one stray read
         * can never reboot the cart out from under the console. */
        if (data == FN_BOOTSEL_MAGIC2 && prev_regnum == FN_REG_BOOTSEL_1
            && port->bootsel)
            port->bootsel();
        break;

    case FN_REG_BOOTSEL_1:
        /* Recorded via prev_regnum below; the magic value is checked here so
         * a pair with the wrong value does not count as the first half. */
        if (data != FN_BOOTSEL_MAGIC1)
            reg = REGSEL_INERT;
        break;

    default:
        break;
    }
    prev_regnum = reg;
}

void fujimail_read_hotspot(uint16_t offset)
{
    uint8_t page = (uint8_t)(offset >> 8);
    uint8_t low = (uint8_t)(offset & 0xFF);

    switch (page) {
    case (FN_H_REGSEL >> 8):
        if (low < 0x80)
            regsel = low;
        /* 0x80-0xFF: special ops, and every defined one belongs to the
         * bus-serving layer, inline -- FN_HOT_SWAP and the FN_HOT_BANK page
         * selects (which must take effect before the next read). They are
         * no-ops here, and they deliberately do NOT disturb an armed regsel,
         * so a stray in this page cannot break up a legitimate pair in the
         * other. */
        break;

    case (FN_H_REGDATA >> 8):
        /* A REGDATA with no immediately-preceding REGSEL is a no-op: this
         * disarm-after-one-use is the core stray-read defense. */
        if (regsel != REGSEL_INERT) {
            uint8_t reg = regsel;

            regsel = REGSEL_INERT;
            reg_write(reg, low);
        }
        break;

    case (FN_H_DATA >> 8):
        if (txptr < sizeof txbuf)
            txbuf[txptr++] = low;
        break;

    default:
        break;
    }
}
