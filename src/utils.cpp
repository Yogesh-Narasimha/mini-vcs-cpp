// utils.cpp - Implementation of generic helpers.
#include "utils.h"

#include <fstream>
#include <iterator>
#include <zlib.h>

std::string readFile(const fs::path &p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw MyGitError("cannot open '" + p.string() + "' for reading");
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeFile(const fs::path &p, const std::string &data) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out) throw MyGitError("cannot open '" + p.string() + "' for writing");
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!out) throw MyGitError("failed while writing '" + p.string() + "'");
}

std::string toHex(const std::string &raw) {
    static const char *digits = "0123456789abcdef";
    std::string out;
    out.reserve(raw.size() * 2);
    for (unsigned char c : raw) {
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 0x0F]);
    }
    return out;
}

static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool isHex(const std::string &s) {
    if (s.empty()) return false;
    for (char c : s)
        if (hexVal(c) < 0) return false;
    return true;
}

std::string fromHex(const std::string &hex) {
    if (hex.size() % 2 != 0 || !isHex(hex)) throw MyGitError("invalid hex string: " + hex);
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2)
        out.push_back(char((hexVal(hex[i]) << 4) | hexVal(hex[i + 1])));
    return out;
}

std::string compressData(const std::string &data) {
    uLongf destLen = compressBound(static_cast<uLong>(data.size()));
    std::string out(destLen, '\0');
    int rc = compress2(reinterpret_cast<Bytef *>(&out[0]), &destLen,
                       reinterpret_cast<const Bytef *>(data.data()),
                       static_cast<uLong>(data.size()), Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) throw MyGitError("zlib compression failed");
    out.resize(destLen);
    return out;
}

std::string decompressData(const std::string &data) {
    // Output size is unknown, so inflate in chunks.
    z_stream zs{};
    if (inflateInit(&zs) != Z_OK) throw MyGitError("zlib init failed");
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = static_cast<uInt>(data.size());

    std::string out;
    char buf[16384];
    int rc;
    do {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            inflateEnd(&zs);
            throw MyGitError("object is corrupt (zlib inflate failed)");
        }
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (rc != Z_STREAM_END);
    inflateEnd(&zs);
    return out;
}

std::string relativeTo(const fs::path &p, const fs::path &base) {
    return p.lexically_normal().lexically_relative(base.lexically_normal()).generic_string();
}
