// cmd_write_tree.cpp - "./mygit write-tree"
// Recursively snapshots the working directory (from the repository root) into tree
// objects. Every file is stored as a blob; empty directories are skipped (as in Git).
#include "commands.h"
#include "objects.h"
#include "index.h"
#include "repo.h"

#include <iostream>
#include <system_error>
#include <vector>

// Returns the SHA of the tree for `dir`, or "" if it contains nothing trackable.
static std::string writeTreeForDir(const fs::path &root, const fs::path &dir) {
    std::vector<TreeEntry> entries;
    std::error_code ec;
    for (const auto &e : fs::directory_iterator(dir, ec)) {
        const fs::path &p = e.path();
        if (isIgnored(p)) continue;
        std::string name = p.filename().string();

        if (e.is_directory(ec) && !e.is_symlink(ec)) {
            std::string sub = writeTreeForDir(root, p);
            if (!sub.empty()) entries.push_back({"40000", name, sub});
        } else if (e.is_regular_file(ec) && !e.is_symlink(ec)) {
            std::string sha = hashObject(root, "blob", readFile(p), true);
            entries.push_back({fileModeOf(p), name, sha});
        }
        // symlinks / sockets / devices are not tracked
    }
    if (ec) throw MyGitError("cannot read directory '" + dir.string() + "': " + ec.message());
    if (entries.empty() && dir != root) return "";
    return hashObject(root, "tree", serializeTree(entries), true);
}

int cmdWriteTree(const Args &args) {
    if (!args.empty()) throw MyGitError("usage: ./mygit write-tree");
    fs::path root = findRepoRoot();
    std::cout << writeTreeForDir(root, root) << "\n";
    return 0;
}
