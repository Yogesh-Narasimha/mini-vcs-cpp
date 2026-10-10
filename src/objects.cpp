// objects.cpp - Reading/writing blob, tree and commit objects.
#include "objects.h"
#include "repo.h"
#include "sha1.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <system_error>

static std::string frame(const std::string &type, const std::string &content) {
    std::string data = type + " " + std::to_string(content.size());
    data.push_back('\0');
    data += content;
    return data;
}

static fs::path objectPath(const fs::path &root, const std::string &sha) {
    return metaDir(root) / "objects" / sha.substr(0, 2) / sha.substr(2);
}

std::string computeObjectSha(const std::string &type, const std::string &content) {
    return sha1Hex(frame(type, content));
}

std::string hashObject(const fs::path &root, const std::string &type,
                       const std::string &content, bool write) {
    std::string data = frame(type, content);
    std::string sha = sha1Hex(data);
    if (write) {
        fs::path p = objectPath(root, sha);
        std::error_code ec;
        if (!fs::exists(p, ec)) {                 // content-addressed: identical data stored once
            fs::create_directories(p.parent_path(), ec);
            if (ec) throw MyGitError("cannot create object directory: " + ec.message());
            writeFile(p, compressData(data));
        }
    }
    return sha;
}

GitObject readObject(const fs::path &root, const std::string &sha) {
    if (sha.size() != 40 || !isHex(sha)) throw MyGitError("not a valid object name: " + sha);
    fs::path p = objectPath(root, sha);
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) throw MyGitError("object not found: " + sha);

    std::string data = decompressData(readFile(p));
    size_t nul = data.find('\0');
    size_t sp = data.find(' ');
    if (nul == std::string::npos || sp == std::string::npos || sp > nul)
        throw MyGitError("corrupt object header: " + sha);

    GitObject obj;
    obj.type = data.substr(0, sp);
    obj.content = data.substr(nul + 1);
    size_t declared = 0;
    try {
        declared = std::stoul(data.substr(sp + 1, nul - sp - 1));
    } catch (...) {
        throw MyGitError("corrupt object size: " + sha);
    }
    if (declared != obj.content.size()) throw MyGitError("object size mismatch: " + sha);
    // Integrity check: the stored data must hash back to its name.
    if (sha1Hex(data) != sha) throw MyGitError("SHA-1 mismatch, object is corrupt: " + sha);
    return obj;
}

std::string resolveObject(const fs::path &root, const std::string &name) {
    // Branch name?
    std::string branchSha = readBranch(root, name);
    if (!branchSha.empty()) return branchSha;
    if (name == "HEAD") {
        std::string h = readHeadCommit(root);
        if (h.empty()) throw MyGitError("HEAD does not point to any commit yet");
        return h;
    }

    std::string s = name;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (!isHex(s) || s.size() < 4 || s.size() > 40)
        throw MyGitError("not a valid object name: '" + name + "'");

    fs::path dir = metaDir(root) / "objects" / s.substr(0, 2);
    std::error_code ec;
    if (s.size() == 40) {
        if (fs::is_regular_file(dir / s.substr(2), ec)) return s;
        throw MyGitError("object not found: " + name);
    }

    // Abbreviated SHA: scan the fan-out directory for a unique match.
    std::vector<std::string> matches;
    if (fs::is_directory(dir, ec)) {
        std::string rest = s.substr(2);
        for (const auto &e : fs::directory_iterator(dir, ec)) {
            std::string fn = e.path().filename().string();
            if (fn.compare(0, rest.size(), rest) == 0) matches.push_back(s.substr(0, 2) + fn);
        }
    }
    if (matches.empty()) throw MyGitError("object not found: " + name);
    if (matches.size() > 1) throw MyGitError("short SHA '" + name + "' is ambiguous");
    return matches[0];
}

std::string typeFromMode(const std::string &mode) {
    return (mode == "40000" || mode == "040000") ? "tree" : "blob";
}

std::string displayMode(const std::string &mode) {
    return mode.size() < 6 ? std::string(6 - mode.size(), '0') + mode : mode;
}

// Tree format: repeated  "<mode> <name>\0<20 raw sha bytes>"
std::vector<TreeEntry> parseTree(const std::string &content) {
    std::vector<TreeEntry> entries;
    size_t i = 0;
    while (i < content.size()) {
        size_t sp = content.find(' ', i);
        size_t nul = content.find('\0', i);
        if (sp == std::string::npos || nul == std::string::npos || sp > nul || nul + 21 > content.size())
            throw MyGitError("corrupt tree object");
        TreeEntry e;
        e.mode = content.substr(i, sp - i);
        e.name = content.substr(sp + 1, nul - sp - 1);
        e.sha = toHex(content.substr(nul + 1, 20));
        entries.push_back(e);
        i = nul + 21;
    }
    return entries;
}

std::string serializeTree(std::vector<TreeEntry> entries) {
    // Git sorts entries by name, treating directories as if they end in '/'.
    auto key = [](const TreeEntry &e) { return typeFromMode(e.mode) == "tree" ? e.name + "/" : e.name; };
    std::sort(entries.begin(), entries.end(),
              [&](const TreeEntry &a, const TreeEntry &b) { return key(a) < key(b); });
    std::string out;
    for (const auto &e : entries) {
        out += e.mode + " " + e.name;
        out.push_back('\0');
        out += fromHex(e.sha);
    }
    return out;
}

// Commit format: header lines, a blank line, then the message.
Commit parseCommit(const std::string &content) {
    Commit c;
    std::istringstream in(content);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) break;                    // end of headers
        size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string key = line.substr(0, sp), val = line.substr(sp + 1);
        if (key == "tree") c.tree = val;
        else if (key == "parent") c.parents.push_back(val);
        else if (key == "author") c.author = val;
        else if (key == "committer") c.committer = val;
    }
    std::string msg((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    while (!msg.empty() && msg.back() == '\n') msg.pop_back();
    c.message = msg;
    if (c.tree.empty()) throw MyGitError("corrupt commit object (no tree)");
    return c;
}

std::string serializeCommit(const Commit &c) {
    std::string out = "tree " + c.tree + "\n";
    for (const auto &p : c.parents) out += "parent " + p + "\n";
    out += "author " + c.author + "\n";
    out += "committer " + c.committer + "\n\n";
    out += c.message + "\n";
    return out;
}
