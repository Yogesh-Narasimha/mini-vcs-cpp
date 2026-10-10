// sha1.h - Self-contained SHA-1 implementation (FIPS 180-1)
#ifndef MYGIT_SHA1_H
#define MYGIT_SHA1_H

#include <string>

// Returns the raw 20-byte SHA-1 digest of the given data.
std::string sha1Raw(const std::string &data);

// Returns the 40-character lowercase hex SHA-1 digest of the given data.
std::string sha1Hex(const std::string &data);

#endif
