// repo.cpp - Repository discovery, HEAD management and ignore rules.
#include "repo.h"

#include <system_error>

fs::path findRepoRoot() {
    fs::path cur = fs::current_path();
    while (true) {
        std::error_code ec;
        if (fs::is_directory(cur / MYGIT_DIR, ec)) return cur;
        if (cur == cur.root_path() || cur.parent_path() == cur) break;
        cur = cur.parent_path();
    }
    throw MyGitError("not a mygit repository (or any of the parent directories): " + MYGIT_DIR +
                     "\nRun './mygit init' first.");
}

fs::path metaDir(const fs::path &root) { return root / MYGIT_DIR; }

// Removes trailing whitespace/newlines.
static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
    return s;
}

static std::string readHeadRaw(const fs::path &root) {
    fs::path head = metaDir(root) / "HEAD";
    if (!fs::exists(head)) throw MyGitError("HEAD file missing; repository is corrupt");
    return trim(readFile(head));
}

std::string headRef(const fs::path &root) {
    std::string raw = readHeadRaw(root);
    const std::string prefix = "ref: ";
    if (raw.compare(0, prefix.size(), prefix) == 0) return raw.substr(prefix.size());
    return "";
}

std::string readHeadCommit(const fs::path &root) {
    std::string ref = headRef(root);
    if (ref.empty()) return readHeadRaw(root);      // detached HEAD holds the SHA itself
    fs::path refPath = metaDir(root) / ref;
    if (!fs::exists(refPath)) return "";             // branch with no commits yet
    return trim(readFile(refPath));
}

void advanceHead(const fs::path &root, const std::string &commitSha) {
    std::string ref = headRef(root);
    if (ref.empty()) {
        setHeadDetached(root, commitSha);
    } else {
        fs::path refPath = metaDir(root) / ref;
        fs::create_directories(refPath.parent_path());
        writeFile(refPath, commitSha + "\n");
    }
}

void setHeadToRef(const fs::path &root, const std::string &ref) {
    writeFile(metaDir(root) / "HEAD", "ref: " + ref + "\n");
}

void setHeadDetached(const fs::path &root, const std::string &commitSha) {
    writeFile(metaDir(root) / "HEAD", commitSha + "\n");
}

std::string readBranch(const fs::path &root, const std::string &branch) {
    fs::path p = metaDir(root) / "refs" / "heads" / branch;
    std::error_code ec;
    if (!fs::is_regular_file(p, ec)) return "";
    return trim(readFile(p));
}

bool isIgnored(const fs::path &absPath) {
    std::string name = absPath.filename().string();
    if (name == MYGIT_DIR || name == ".git") return true;

    // Never track the running mygit executable if it sits inside the repository.
    std::error_code ec;
    static const fs::path self = fs::canonical("/proc/self/exe", ec);
    if (!self.empty() && fs::equivalent(absPath, self, ec)) return true;
    return false;
}
