// cmd_checkout.cpp - "./mygit checkout [-f] <commit-sha | branch>"
// Restores the working directory to the snapshot of a commit:
//   1. safety checks (uncommitted work would be lost -> refuse unless -f)
//   2. delete tracked files that do not exist in the target commit
//   3. write every blob of the target tree to disk
//   4. reset the index to the target tree and move HEAD
#include "commands.h"
#include "index.h"
#include "objects.h"
#include "repo.h"

#include <iostream>
#include <set>
#include <system_error>

// Removes now-empty directories from `dir` up to (not including) the repository root.
static void pruneEmptyDirs(const fs::path &root, fs::path dir) {
    std::error_code ec;
    while (dir != root && dir.string().size() > root.string().size()) {
        if (!fs::is_directory(dir, ec) || !fs::is_empty(dir, ec)) break;
        fs::remove(dir, ec);
        dir = dir.parent_path();
    }
}

int cmdCheckout(const Args &args) {
    const std::string usage = "usage: ./mygit checkout [-f] <commit-sha | branch>";
    bool force = false;
    std::string target;
    for (const auto &a : args) {
        if (a == "-f" || a == "--force") force = true;
        else if (!a.empty() && a[0] == '-') throw MyGitError("unknown option '" + a + "'\n" + usage);
        else if (target.empty()) target = a;
        else throw MyGitError(usage);
    }
    if (target.empty()) throw MyGitError(usage);

    fs::path root = findRepoRoot();
    bool isBranch = !readBranch(root, target).empty();
    std::string sha;
    try {
        sha = resolveObject(root, target);
    } catch (const MyGitError &) {
        throw MyGitError("invalid or nonexistent commit/branch '" + target + "'");
    }
    GitObject obj = readObject(root, sha);
    if (obj.type != "commit") throw MyGitError("'" + target + "' is a " + obj.type + ", not a commit");
    Commit commit = parseCommit(obj.content);

    Index targetFiles;
    flattenTree(root, commit.tree, "", targetFiles);
    Index idx = readIndex(root);
    Index headFiles = headSnapshot(root);

    // ---- 1. Safety checks ----
    if (!force) {
        std::vector<std::string> problems;
        if (idx != headFiles) problems.push_back("  (staged changes that are not committed)");
        for (const auto &kv : idx) {
            fs::path abs = root / kv.first;
            std::error_code ec;
            if (fs::is_regular_file(abs, ec) && computeObjectSha("blob", readFile(abs)) != kv.second.sha)
                problems.push_back("  modified: " + kv.first);
        }
        for (const auto &kv : targetFiles) {                  // untracked files that would be clobbered
            if (idx.count(kv.first)) continue;
            fs::path abs = root / kv.first;
            std::error_code ec;
            if (fs::is_regular_file(abs, ec) && computeObjectSha("blob", readFile(abs)) != kv.second.sha)
                problems.push_back("  untracked (would be overwritten): " + kv.first);
        }
        if (!problems.empty()) {
            std::string msg = "your local changes would be lost by checkout:\n";
            for (const auto &p : problems) msg += p + "\n";
            msg += "Commit them first, or use './mygit checkout -f " + target + "' to discard them.";
            throw MyGitError(msg);
        }
    }

    // ---- 2. Remove tracked files that are absent from the target ----
    std::set<std::string> tracked;
    for (const auto &kv : idx) tracked.insert(kv.first);
    for (const auto &kv : headFiles) tracked.insert(kv.first);
    for (const auto &p : tracked) {
        if (targetFiles.count(p)) continue;
        fs::path abs = root / p;
        std::error_code ec;
        if (fs::is_regular_file(fs::symlink_status(abs, ec))) fs::remove(abs, ec);
        pruneEmptyDirs(root, abs.parent_path());
    }

    // ---- 3. Materialise the target tree ----
    for (const auto &kv : targetFiles) {
        fs::path abs = root / kv.first;
        std::error_code ec;
        if (fs::is_directory(abs, ec)) {
            if (!fs::is_empty(abs, ec)) throw MyGitError("cannot overwrite non-empty directory '" + kv.first + "' with a file");
            fs::remove(abs, ec);
        }
        fs::create_directories(abs.parent_path(), ec);
        if (ec) throw MyGitError("cannot create directory for '" + kv.first + "': " + ec.message());

        GitObject blob = readObject(root, kv.second.sha);
        if (blob.type != "blob") throw MyGitError("corrupt tree: " + kv.first + " is not a blob");
        writeFile(abs, blob.content);

        fs::perms pm = fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read;
        if (kv.second.mode == "100755")
            pm |= fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
        fs::permissions(abs, pm, fs::perm_options::replace, ec);
    }

    // ---- 4. Update index and HEAD ----
    writeIndex(root, targetFiles);
    if (isBranch) {
        setHeadToRef(root, "refs/heads/" + target);
        std::cout << "Switched to branch '" << target << "' (" << sha.substr(0, 7) << " " << commit.message << ")\n";
    } else {
        setHeadDetached(root, sha);
        std::cout << "HEAD is now at " << sha.substr(0, 7) << " " << commit.message << "\n"
                  << "(detached HEAD; run './mygit checkout " << DEFAULT_BRANCH << "' to return to the latest commit)\n";
    }
    return 0;
}
