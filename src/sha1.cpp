// sha1.cpp - SHA-1 hashing implemented from the specification.
// Processes the message in 512-bit blocks after standard padding.
#include "sha1.h"
#include "utils.h"

#include <cstdint>

namespace {

inline uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

// Compresses one 64-byte block into the running state h[0..4].
void processBlock(const unsigned char *block, uint32_t h[5]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
               (uint32_t(block[i * 4 + 2]) << 8) | uint32_t(block[i * 4 + 3]);
    }
    for (int i = 16; i < 80; ++i) w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);           k = 0x5A827999; }
        else if (i < 40) { f = b ^ c ^ d;                    k = 0x6ED9EBA1; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d);  k = 0x8F1BBCDC; }
        else             { f = b ^ c ^ d;                    k = 0xCA62C1D6; }
        uint32_t temp = rotl(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl(b, 30); b = a; a = temp;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

} // namespace

std::string sha1Raw(const std::string &data) {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    // Padding: append 0x80, zeros, then the 64-bit big-endian bit length.
    std::string msg = data;
    uint64_t bitLen = uint64_t(data.size()) * 8;
    msg.push_back(char(0x80));
    while (msg.size() % 64 != 56) msg.push_back('\0');
    for (int i = 7; i >= 0; --i) msg.push_back(char((bitLen >> (i * 8)) & 0xFF));

    for (size_t off = 0; off < msg.size(); off += 64)
        processBlock(reinterpret_cast<const unsigned char *>(msg.data() + off), h);

    std::string out;
    for (uint32_t v : h)
        for (int s = 24; s >= 0; s -= 8) out.push_back(char((v >> s) & 0xFF));
    return out;
}

std::string sha1Hex(const std::string &data) { return toHex(sha1Raw(data)); }
