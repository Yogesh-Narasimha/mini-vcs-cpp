// utils.h - Generic helpers: file I/O, hex conversion, compression, errors.
#ifndef MYGIT_UTILS_H
#define MYGIT_UTILS_H

#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// Error type thrown by all commands; main() prints it as "fatal: <msg>".
struct MyGitError : public std::runtime_error {
    explicit MyGitError(const std::string &msg) : std::runtime_error(msg) {}
};

// Reads an entire file in binary mode. Throws MyGitError on failure.
std::string readFile(const fs::path &p);

// Writes (overwrites) a file in binary mode. Throws MyGitError on failure.
void writeFile(const fs::path &p, const std::string &data);

// Raw bytes <-> lowercase hex.
std::string toHex(const std::string &raw);
std::string fromHex(const std::string &hex);

// True if s is a (possibly abbreviated) lowercase/uppercase hex string.
bool isHex(const std::string &s);

// zlib wrappers.
std::string compressData(const std::string &data);
std::string decompressData(const std::string &data);

// Converts a path to a forward-slash generic string relative to base.
std::string relativeTo(const fs::path &p, const fs::path &base);

#endif
