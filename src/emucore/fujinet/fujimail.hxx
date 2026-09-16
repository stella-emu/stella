/* fujimail.h -- the mailbox service: hotspot decode, the SEQ/ACKSEQ
 * interlock, and the DBC ROM-push receiver.
 *
 * Shared by the RP2040 cartridge and the MAME cart model, behind a small port
 * interface, for the same reason fujibus.c and astromap.c are shared: this is
 * the protocol, and two copies of a protocol drift. Because the emulator
 * drives this code, every browse-and-boot run against a real fujinet-pc
 * exercises what the cartridge actually ships rather than a lookalike.
 *
 * The port supplies what genuinely differs between a cartridge and an
 * emulator: where a published byte goes, how a frame reaches the ESP32-S3,
 * what to do with a finished push stream, how the armed swap is signalled,
 * and how to reboot into BOOTSEL.
 */

#ifndef FUJIMAIL_H
#define FUJIMAIL_H

#include <stdbool.h>
#include <stdint.h>

#include "fujibus.hxx"

#define FUJIMAIL_RX_MAX    1024

/* Reported to the port purely so it can log; nothing depends on it. */
typedef struct {
    uint8_t device, command, nparam, seq, reply_cmd;
    uint16_t txlen, rxlen;
    int status;                 /* fb_status_t */
    const uint8_t *rx;
} fujimail_txn_t;

typedef enum {
    FUJIMAIL_DBC_OPEN,
    FUJIMAIL_DBC_CLOSE,
} fujimail_dbc_ev_t;

typedef struct {
    /* Publish one mailbox byte into the served cartridge window. */
    void (*poke)(unsigned offset, uint8_t value);

    bool (*link_up)(void);

    fb_status_t (*transact)(uint8_t device, uint8_t command,
                            const fb_param_t *params, unsigned nparams,
                            const uint8_t *payload, uint16_t payload_len,
                            uint32_t timeout_ms, fb_reply_t *reply);

    /* Send a frame with no reply expected -- used to ACK push frames. */
    void (*send_bare)(uint8_t device, uint8_t command,
                      const uint8_t *payload, uint16_t payload_len);

    /* A push stream is open(size) -> write* -> close, and its storage lives
     * in the port (a banked image can be far bigger than any buffer this
     * service could own). `stream` is 0 for the ROM image and 1 for the
     * .cfg sibling. open returns 0 to accept or the FN_BOOT_ERR_* to refuse
     * -- refusal happens HERE, before the ESP32 drags the file over TNFS.
     * close returns 0 when a committed stream-0 image was mapped and staged,
     * else the FN_BOOT_ERR_* to report; READY is published only on 0, so a
     * close-time failure can never read as bootable. The staged image must
     * NOT be served yet -- the console is still executing the client out of
     * the window. */
    uint8_t (*stream_open)(int stream, uint32_t size);
    void (*stream_write)(int stream, const uint8_t *chunk, unsigned len);
    uint8_t (*stream_close)(int stream, uint32_t got, bool aborted);

    /* The client wrote FN_REG_BOOTLOCK = FN_BOOTLOCK_MAGIC: from now on a
     * read of the FN_HOT_SWAP hotspot serves the staged image. */
    void (*arm_swap)(void);

    /* Optional. */
    void (*wait_link_ms)(uint32_t ms);
    void (*bootsel)(void);
    void (*on_txn)(const fujimail_txn_t *txn);
    void (*on_dbc)(fujimail_dbc_ev_t ev, int stream, uint32_t expect,
                   unsigned got, bool aborted);
} fujimail_port_t;

void fujimail_init(const fujimail_port_t *port);

/* Paint the magic bytes and clear the rest of the status pages. */
void fujimail_paint(void);

/* One console read of a hotspot page: a cart offset in
 * [FN_H_REGSEL, 0x1FFF]. The caller has already filtered by range and by
 * fuji_mailbox_active; FN_HOT_SWAP is the caller's to handle (the swap must
 * happen inline in whatever serves the bus, not here). */
void fujimail_read_hotspot(uint16_t offset);

/* Offer an inbound frame; true if it was a push frame and was consumed. */
bool fujimail_inbound(const fb_reply_t *frame);

#endif /* FUJIMAIL_H */
