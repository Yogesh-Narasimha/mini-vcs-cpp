// index.cpp - Staging area persistence and tree construction from the index.
#include "index.h"
#include "objects.h"
#include "repo.h"

#include <sstream>
#include <system_error>
#include <vector>

Index readIndex(const fs::path &root) {
    Index idx;
    fs::path p = metaDir(root) / "index";
    std::error_code ec;
    if (!fs::exists(p, ec)) return idx;

    std::istringstream in(readFile(p));
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        size_t s1 = line.find(' ');
        size_t s2 = (s1 == std::string::npos) ? std::string::npos : line.find(' ', s1 + 1);
        if (s2 == std::string::npos) throw MyGitError("corrupt index at line " + std::to_string(lineNo));
        IndexEntry e{line.substr(0, s1), line.substr(s1 + 1, s2 - s1 - 1)};
        idx[line.substr(s2 + 1)] = e;
    }
    return idx;
}

void writeIndex(const fs::path &root, const Index &idx) {
    std::string out;
    for (const auto &kv : idx) out += kv.second.mode + " " + kv.second.sha + " " + kv.first + "\n";
    writeFile(metaDir(root) / "index", out);
}

std::string fileModeOf(const fs::path &p) {
    std::error_code ec;
    auto perms = fs::status(p, ec).permissions();
    if (!ec && (perms & fs::perms::owner_exec) != fs::perms::none) return "100755";
    return "100644";
}

namespace {

// In-memory directory node used to turn flat paths into nested trees.
struct DirNode {
    std::map<std::string, IndexEntry> files;
    std::map<std::string, DirNode> dirs;
};

std::string writeNode(const fs::path &root, const DirNode &node) {
    std::vector<TreeEntry> entries;
    for (const auto &f : node.files) entries.push_back({f.second.mode, f.first, f.second.sha});
    for (const auto &d : node.dirs) entries.push_back({"40000", d.first, writeNode(root, d.second)});
    return hashObject(root, "tree", serializeTree(entries), true);
}

} // namespace

std::string writeTreeFromIndex(const fs::path &root, const Index &idx) {
    DirNode top;
    for (const auto &kv : idx) {
        DirNode *cur = &top;
        std::string path = kv.first;
        size_t pos;
        while ((pos = path.find('/')) != std::string::npos) {
            cur = &cur->dirs[path.substr(0, pos)];
            path = path.substr(pos + 1);
        }
        cur->files[path] = kv.second;
    }
    return writeNode(root, top);
}

void flattenTree(const fs::path &root, const std::string &treeSha,
                 const std::string &prefix, Index &out) {
    GitObject obj = readObject(root, treeSha);
    if (obj.type != "tree") throw MyGitError(treeSha + " is not a tree object");
    for (const auto &e : parseTree(obj.content)) {
        std::string path = prefix.empty() ? e.name : prefix + "/" + e.name;
        if (typeFromMode(e.mode) == "tree") flattenTree(root, e.sha, path, out);
        else out[path] = {e.mode, e.sha};
    }
}

void collectWorkingFiles(const fs::path &root, const fs::path &start,
                         std::map<std::string, fs::path> &out) {
    std::error_code ec;
    if (isIgnored(start) || fs::is_symlink(fs::symlink_status(start, ec))) return;
    if (fs::is_regular_file(start, ec)) {
        out[relativeTo(start, root)] = start;
    } else if (fs::is_directory(start, ec)) {
        for (const auto &e : fs::directory_iterator(start, ec)) collectWorkingFiles(root, e.path(), out);
        if (ec) throw MyGitError("cannot read directory '" + start.string() + "': " + ec.message());
    }
}

Index headSnapshot(const fs::path &root) {
    Index snap;
    std::string head = readHeadCommit(root);
    if (head.empty()) return snap;
    GitObject obj = readObject(root, head);
    if (obj.type != "commit") throw MyGitError("HEAD does not point to a commit");
    flattenTree(root, parseCommit(obj.content).tree, "", snap);
    return snap;
}
