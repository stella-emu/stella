// See fujibus.h. Ported line-for-line against
// lib/bus/rs232/FujiBusPacket.cpp in the main fujinet-firmware tree -- that
// file is the source of truth; comments below cite it by function name.
#include "fujibus.hxx"
#include <string.h>

enum {
    SLIP_END     = 0xC0,
    SLIP_ESCAPE  = 0xDB,
    SLIP_ESC_END = 0xDC,
    SLIP_ESC_ESC = 0xDD,
};

// FujiBusPacket.cpp: fieldSizeTable / numFieldsTable, indexed by descr&0x07.
static const uint8_t fb_field_size[8]  = {0, 1, 1, 1, 1, 2, 2, 4};
static const uint8_t fb_field_count[8] = {0, 1, 2, 3, 4, 1, 2, 1};

#define FB_DESCR_ADDTL_MASK 0x80
#define FB_DESCR_COUNT_MASK 0x07
#define FB_MAX_DESCRIPTORS  16 /* defensive cap on a malformed/hostile reply */

// FujiBusPacket.cpp: calcChecksum() -- 8-bit sum with end-around carry fold,
// computed with the header's checksum byte set to 0.
static uint8_t fb_checksum(const uint8_t *buf, size_t n)
{
    uint16_t c = 0;
    for (size_t i = 0; i < n; i++) {
        c += buf[i];
        c = (uint16_t)((c >> 8) + (c & 0xFF));
    }
    return (uint8_t)c;
}

size_t fujibus_build_request(uint8_t device, uint8_t command,
                              const fb_param_t *params, unsigned nparams,
                              const uint8_t *payload, uint16_t payload_len,
                              uint8_t *out, size_t out_cap)
{
    // Decoded (unescaped) packet is assembled here first, then SLIP-encoded
    // into `out` below -- encoding can grow the data, so it can't be done
    // in place the way parse()'s decode can.
    uint8_t pkt[384];
    size_t off = 6; // header size

    if (nparams > 0) {
        if (nparams > FB_MAX_DESCRIPTORS)
            return 0;

        // FujiBusPacket.cpp serialize(): the reference encoder greedily
        // groups same-size params up to 4 bytes per descriptor. parse() on
        // the far end accepts any legal descriptor chain, so we simplify:
        // one descriptor per param (fieldDescr = count(1) + EXCEEDS_U8 +
        // EXCEEDS_U16, per fieldSize).
        uint8_t descr[FB_MAX_DESCRIPTORS];
        for (unsigned i = 0; i < nparams; i++) {
            uint8_t size = params[i].size;
            if (size != 1 && size != 2 && size != 4)
                return 0;
            uint8_t d = 1;
            if (size > 1) d += 0x04;
            if (size > 2) d += 0x02;
            descr[i] = (uint8_t)(d | ((i + 1 < nparams) ? FB_DESCR_ADDTL_MASK : 0));
        }

        pkt[5] = descr[0]; // first descriptor lives in the header
        for (unsigned i = 1; i < nparams; i++) {
            if (off >= sizeof(pkt)) return 0;
            pkt[off++] = descr[i];
        }
        for (unsigned i = 0; i < nparams; i++) {
            uint8_t size = params[i].size;
            uint32_t v = params[i].value;
            if (off + size > sizeof(pkt)) return 0;
            for (uint8_t b = 0; b < size; b++)
                pkt[off++] = (uint8_t)((v >> (8 * b)) & 0xFF);
        }
    } else {
        pkt[5] = 0;
    }

    if (payload_len) {
        if (off + payload_len > sizeof(pkt)) return 0;
        memcpy(&pkt[off], payload, payload_len);
        off += payload_len;
    }

    if (off > 0xFFFF) return 0;
    pkt[0] = device;
    pkt[1] = command;
    pkt[2] = (uint8_t)(off & 0xFF);
    pkt[3] = (uint8_t)((off >> 8) & 0xFF);
    pkt[4] = 0;
    pkt[4] = fb_checksum(pkt, off);

    // FujiBusPacket.cpp: encodeSLIP() -- leading END, escape 0xC0/0xDB,
    // trailing END.
    size_t w = 0;
    if (w >= out_cap) return 0;
    out[w++] = SLIP_END;
    for (size_t i = 0; i < off; i++) {
        uint8_t v = pkt[i];
        if (v == SLIP_END || v == SLIP_ESCAPE) {
            if (w + 2 > out_cap) return 0;
            out[w++] = SLIP_ESCAPE;
            out[w++] = (v == SLIP_END) ? SLIP_ESC_END : SLIP_ESC_ESC;
        } else {
            if (w + 1 > out_cap) return 0;
            out[w++] = v;
        }
    }
    if (w + 1 > out_cap) return 0;
    out[w++] = SLIP_END;

    return w;
}

bool fujibus_parse_reply(const uint8_t *in_const, size_t in_len, fb_reply_t *reply)
{
    // Decoding only ever removes bytes (escape collapse), so it's safe to
    // decode in place into the caller's buffer; reply->data then points
    // back into it. This does mean `in` is mutated -- documented in the
    // header.
    uint8_t *in = (uint8_t *)in_const;

    // FujiBusPacket.cpp: parse() finds the first SLIP_END and treats the
    // frame as starting there; the caller is expected to have already
    // framed the buffer to end at the matching closing END (that's what
    // the ESP32's readBusPacket()/two-END accumulation does, and what our
    // own USB read loop must do too).
    size_t start = 0;
    while (start < in_len && in[start] != SLIP_END)
        start++;
    if (start == in_len)
        return false;
    if (in_len - start < 6 + 2) // sizeof(fujibus_header) + 2
        return false;
    if (in[in_len - 1] != SLIP_END)
        return false;

    size_t r = start + 1;
    size_t w = 0;
    while (r < in_len && in[r] != SLIP_END) {
        uint8_t v = in[r];
        if (v == SLIP_ESCAPE) {
            r++;
            if (r >= in_len)
                break; // truncated escape
            v = in[r];
            if (v == SLIP_ESC_END)
                in[w++] = SLIP_END;
            else if (v == SLIP_ESC_ESC)
                in[w++] = SLIP_ESCAPE;
            // else: ignore malformed escape (matches decodeSLIP())
        } else {
            in[w++] = v;
        }
        r++;
    }
    size_t decoded_len = w;
    if (decoded_len < 6)
        return false;

    uint16_t length = (uint16_t)(in[2] | (in[3] << 8));
    if (length != decoded_len)
        return false;

    uint8_t ck1 = in[4];
    in[4] = 0;
    uint8_t ck2 = fb_checksum(in, decoded_len);
    if (ck1 != ck2)
        return false;

    uint8_t device = in[0];
    uint8_t command = in[1];

    // Descriptor chain: first descriptor in the header, additional ones
    // (while bit 7 is set) immediately follow the header, then param value
    // bytes, then whatever's left is payload. Real ACK/NAK replies always
    // carry descr==0 (no params) -- this still walks the general case so
    // the codec is spec-complete and testable against the reference
    // multi-param vector.
    uint8_t descr[FB_MAX_DESCRIPTORS];
    unsigned ndescr = 0;
    uint8_t dsc = in[5];
    descr[ndescr++] = dsc;
    size_t offset = 6;
    while (dsc & FB_DESCR_ADDTL_MASK) {
        if (ndescr >= FB_MAX_DESCRIPTORS)
            return false;
        if (offset >= decoded_len)
            return false;
        dsc = in[offset++];
        descr[ndescr++] = dsc;
    }

    for (unsigned i = 0; i < ndescr; i++) {
        unsigned idx = descr[i] & FB_DESCR_COUNT_MASK;
        unsigned count = fb_field_count[idx];
        unsigned size = fb_field_size[idx];
        for (unsigned f = 0; f < count; f++) {
            if (offset + size > decoded_len)
                return false;
            offset += size;
        }
    }

    reply->device = device;
    reply->command = command;
    reply->data = (offset < decoded_len) ? &in[offset] : NULL;
    reply->data_len = (uint16_t)(decoded_len - offset);
    return true;
}

// ---------------------------------------------------------------------
// Self-test: known-good vectors, no USB/hardware involved.
// ---------------------------------------------------------------------

static bool fb_bytes_eq(const uint8_t *a, const uint8_t *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

bool fujibus_selftest(void)
{
    uint8_t buf[512];

    // GET_ADAPTERCONFIG_EXTENDED to Fuji, no params. Verified against
    // lib/bus/rs232/FujiBusPacket.cpp by hand:
    //   decoded: 70 C4 06 00 3B 00  (device, cmd, len=6 LE, checksum, descr=0)
    //   on wire: C0 70 C4 06 00 3B 00 C0
    static const uint8_t want_getcfg[] = {
        0xC0, 0x70, 0xC4, 0x06, 0x00, 0x3B, 0x00, 0xC0,
    };
    size_t n = fujibus_build_request(FUJI_DEVICEID_FUJINET,
                                      CMD_FUJI_GET_ADAPTERCONFIG_EXTENDED,
                                      NULL, 0, NULL, 0, buf, sizeof(buf));
    if (n != sizeof(want_getcfg) || !fb_bytes_eq(buf, want_getcfg, n))
        return false;

    // Bare ACK reply: C0 70 06 06 00 7C 00 C0 (device 0x70 echoed, no data).
    {
        uint8_t frame[] = {0xC0, 0x70, 0x06, 0x06, 0x00, 0x7C, 0x00, 0xC0};
        fb_reply_t reply;
        if (!fujibus_parse_reply(frame, sizeof(frame), &reply))
            return false;
        if (reply.device != FUJI_DEVICEID_FUJINET || reply.command != CMD_FUJI_ACK)
            return false;
        if (reply.data_len != 0)
            return false;
    }

    // Bare NAK reply: C0 70 15 06 00 8B 00 C0.
    {
        uint8_t frame[] = {0xC0, 0x70, 0x15, 0x06, 0x00, 0x8B, 0x00, 0xC0};
        fb_reply_t reply;
        if (!fujibus_parse_reply(frame, sizeof(frame), &reply))
            return false;
        if (reply.command != CMD_FUJI_NAK)
            return false;
    }

    // Round-trip a synthetic 246-byte ACK carrying the 240-byte
    // AdapterConfigExtended payload (240 sentinel bytes of 0xA5, chosen
    // because it also exercises SLIP escaping -- 0xA5 needs none, but the
    // point is the framing/length/checksum machinery at this size).
    {
        uint8_t decoded[6 + FUJI_ADAPTERCONFIG_EXTENDED_SIZE];
        decoded[0] = FUJI_DEVICEID_FUJINET;
        decoded[1] = CMD_FUJI_ACK;
        uint16_t len = sizeof(decoded);
        decoded[2] = (uint8_t)(len & 0xFF);
        decoded[3] = (uint8_t)(len >> 8);
        decoded[4] = 0;
        decoded[5] = 0;
        for (int i = 0; i < FUJI_ADAPTERCONFIG_EXTENDED_SIZE; i++)
            decoded[6 + i] = 0xA5;
        decoded[4] = fb_checksum(decoded, sizeof(decoded));

        uint8_t frame[2 + 2 * sizeof(decoded)];
        size_t w = 0;
        frame[w++] = SLIP_END;
        for (size_t i = 0; i < sizeof(decoded); i++) {
            uint8_t v = decoded[i];
            if (v == SLIP_END || v == SLIP_ESCAPE) {
                frame[w++] = SLIP_ESCAPE;
                frame[w++] = (v == SLIP_END) ? SLIP_ESC_END : SLIP_ESC_ESC;
            } else {
                frame[w++] = v;
            }
        }
        frame[w++] = SLIP_END;

        fb_reply_t reply;
        if (!fujibus_parse_reply(frame, w, &reply))
            return false;
        if (reply.data_len != FUJI_ADAPTERCONFIG_EXTENDED_SIZE)
            return false;
        for (int i = 0; i < FUJI_ADAPTERCONFIG_EXTENDED_SIZE; i++)
            if (reply.data[i] != 0xA5)
                return false;
    }

    // Reference vector (FujiBusPacketTests.cpp): 5xu8 + 1xu16 params, no
    // payload. Groups [4xu8],[1xu8],[1xu16] -> descriptors 0x84,0x81,0x05;
    // decoded: 70 C4 0F 00 <ck> 84 81 05 01 02 03 04 05 CD AB (len=15).
    // This exercises the multi-descriptor walk in fujibus_parse_reply(),
    // even though real ACK/NAK replies never carry params.
    {
        uint8_t decoded[] = {
            0x70, 0xC4, 0x0F, 0x00, 0x00, 0x84,
            0x81, 0x05,
            0x01, 0x02, 0x03, 0x04, 0x05, 0xCD, 0xAB,
        };
        decoded[4] = fb_checksum(decoded, sizeof(decoded));

        uint8_t frame[sizeof(decoded) + 2];
        frame[0] = SLIP_END;
        memcpy(&frame[1], decoded, sizeof(decoded));
        frame[sizeof(frame) - 1] = SLIP_END;

        fb_reply_t reply;
        if (!fujibus_parse_reply(frame, sizeof(frame), &reply))
            return false;
        if (reply.data_len != 0) // all 15 bytes were header+descriptors+params
            return false;
    }

    // Corrupted checksum must be rejected.
    {
        uint8_t frame[] = {0xC0, 0x70, 0x06, 0x06, 0x00, 0x00, 0x00, 0xC0};
        fb_reply_t reply;
        if (fujibus_parse_reply(frame, sizeof(frame), &reply))
            return false;
    }

    return true;
}
