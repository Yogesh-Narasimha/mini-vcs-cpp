// cmd_commit.cpp - "./mygit commit [-m <message>]"
// Builds tree objects from the index, creates a commit object that references the
// tree and the previous HEAD commit (parent), and moves HEAD forward.
#include "commands.h"
#include "index.h"
#include "objects.h"
#include "repo.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

// Identity comes from MYGIT_AUTHOR_NAME / MYGIT_AUTHOR_EMAIL, falling back to $USER.
static std::string identity() {
    const char *name = std::getenv("MYGIT_AUTHOR_NAME");
    if (!name || !*name) name = std::getenv("USER");
    if (!name || !*name) name = std::getenv("LOGNAME");
    std::string n = (name && *name) ? name : "unknown";
    const char *email = std::getenv("MYGIT_AUTHOR_EMAIL");
    std::string e = (email && *email) ? email : n + "@localhost";
    return n + " <" + e + ">";
}

// "<epoch seconds> <+hhmm>" in local time.
static std::string timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    long off = local.tm_gmtoff;
    char sign = off < 0 ? '-' : '+';
    if (off < 0) off = -off;
    char tz[32];
    std::snprintf(tz, sizeof(tz), "%c%02ld%02ld", sign, off / 3600, (off % 3600) / 60);
    return std::to_string(static_cast<long long>(now)) + " " + tz;
}

int cmdCommit(const Args &args) {
    std::string message = "Default commit message";
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-m") {
            if (i + 1 >= args.size()) throw MyGitError("switch 'm' requires a value");
            message = args[++i];
            if (message.empty()) throw MyGitError("aborting commit due to empty commit message");
        } else {
            throw MyGitError("unknown argument '" + args[i] + "'\nusage: ./mygit commit [-m <message>]");
        }
    }

    fs::path root = findRepoRoot();
    Index idx = readIndex(root);
    std::string parent = readHeadCommit(root);
    if (idx.empty() && parent.empty())
        throw MyGitError("nothing to commit (create/copy files and use './mygit add' to track)");

    std::string tree = writeTreeFromIndex(root, idx);
    if (!parent.empty()) {
        Commit p = parseCommit(readObject(root, parent).content);
        if (p.tree == tree) throw MyGitError("nothing to commit, staged snapshot is identical to HEAD");
    }

    Commit c;
    c.tree = tree;
    if (!parent.empty()) c.parents.push_back(parent);
    std::string who = identity() + " " + timestamp();
    c.author = c.committer = who;
    c.message = message;

    std::string sha = hashObject(root, "commit", serializeCommit(c), true);
    advanceHead(root, sha);

    std::string ref = headRef(root);
    std::string where = ref.empty() ? "detached HEAD" : ref.substr(ref.rfind('/') + 1);
    std::cout << "[" << where << (parent.empty() ? " (root-commit)" : "") << " " << sha.substr(0, 7)
              << "] " << message << "\n";
    std::cout << sha << "\n";
    return 0;
}
