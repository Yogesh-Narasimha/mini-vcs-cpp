// objects.h - Object database: blobs, trees and commits (Git-compatible format).
//
// Every object is stored as  zlib( "<type> <size>\0<content>" )
// at .mygit/objects/<first 2 hex chars>/<remaining 38 hex chars>,
// where the file name is the SHA-1 of the *uncompressed* header+content.
#ifndef MYGIT_OBJECTS_H
#define MYGIT_OBJECTS_H

#include "utils.h"

#include <string>
#include <vector>

struct GitObject {
    std::string type;     // "blob", "tree" or "commit"
    std::string content;  // raw content (header stripped)
};

struct TreeEntry {
    std::string mode;     // "100644", "100755" or "40000"
    std::string name;     // single path component
    std::string sha;      // 40-char hex
};

struct Commit {
    std::string tree;
    std::vector<std::string> parents;
    std::string author;     // "Name <email> <epoch> <tz>"
    std::string committer;
    std::string message;
};

// Computes the SHA-1 of an object; stores it in the object DB if write == true.
std::string hashObject(const fs::path &root, const std::string &type,
                       const std::string &content, bool write);

// Computes the SHA-1 an object would have without a repository (no write).
std::string computeObjectSha(const std::string &type, const std::string &content);

// Reads and decompresses an object. Throws if missing/corrupt.
GitObject readObject(const fs::path &root, const std::string &sha);

// Resolves a full SHA, a unique abbreviated SHA (>= 4 chars) or a branch name.
std::string resolveObject(const fs::path &root, const std::string &name);

// Tree (de)serialisation. Entries are sorted Git-style by serializeTree.
std::vector<TreeEntry> parseTree(const std::string &content);
std::string serializeTree(std::vector<TreeEntry> entries);

// "tree" for directory modes, "blob" otherwise.
std::string typeFromMode(const std::string &mode);

// Pads the tree mode to 6 digits for display ("40000" -> "040000").
std::string displayMode(const std::string &mode);

// Commit (de)serialisation.
Commit parseCommit(const std::string &content);
std::string serializeCommit(const Commit &c);

#endif
