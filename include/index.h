// index.h - The staging area (.mygit/index) and tree <-> flat-map helpers.
//
// Index file format (plain text, one entry per line, sorted by path):
//     <mode> <sha> <path relative to repo root>
#ifndef MYGIT_INDEX_H
#define MYGIT_INDEX_H

#include "utils.h"

#include <map>
#include <string>

struct IndexEntry {
    std::string mode;  // "100644" or "100755"
    std::string sha;   // blob SHA
};

inline bool operator==(const IndexEntry &a, const IndexEntry &b) {
    return a.mode == b.mode && a.sha == b.sha;
}
inline bool operator!=(const IndexEntry &a, const IndexEntry &b) { return !(a == b); }

// path (forward slashes, relative to root) -> entry
using Index = std::map<std::string, IndexEntry>;

Index readIndex(const fs::path &root);
void writeIndex(const fs::path &root, const Index &idx);

// "100755" if the owner-execute bit is set, otherwise "100644".
std::string fileModeOf(const fs::path &p);

// Builds (and stores) nested tree objects from a flat index. Returns the root tree SHA.
std::string writeTreeFromIndex(const fs::path &root, const Index &idx);

// Recursively expands a tree object into a flat path -> entry map.
void flattenTree(const fs::path &root, const std::string &treeSha,
                 const std::string &prefix, Index &out);

// Collects every trackable regular file at or below `start` (a file or directory)
// into out[path relative to root] = absolute path. Ignored paths are skipped.
void collectWorkingFiles(const fs::path &root, const fs::path &start,
                         std::map<std::string, fs::path> &out);

// Returns the flat file map of the commit HEAD points to (empty if no commits).
Index headSnapshot(const fs::path &root);

#endif
