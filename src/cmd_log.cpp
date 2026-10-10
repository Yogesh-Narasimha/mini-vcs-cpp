// cmd_log.cpp - "./mygit log"
// Walks the parent chain from HEAD and prints every commit, newest first.
#include "commands.h"
#include "objects.h"
#include "repo.h"

#include <ctime>
#include <iostream>

// Splits "Name <email> <epoch> <tz>" and formats the date in the commit's own timezone.
static void splitIdentity(const std::string &raw, std::string &who, std::string &date) {
    who = raw;
    date = "(unknown date)";
    size_t tzPos = raw.rfind(' ');
    if (tzPos == std::string::npos) return;
    size_t tsPos = raw.rfind(' ', tzPos - 1);
    if (tsPos == std::string::npos) return;

    who = raw.substr(0, tsPos);
    std::string tz = raw.substr(tzPos + 1);
    try {
        long long epoch = std::stoll(raw.substr(tsPos + 1, tzPos - tsPos - 1));
        long off = 0;
        if (tz.size() == 5 && (tz[0] == '+' || tz[0] == '-')) {
            off = std::stol(tz.substr(1, 2)) * 3600 + std::stol(tz.substr(3, 2)) * 60;
            if (tz[0] == '-') off = -off;
        }
        std::time_t t = static_cast<std::time_t>(epoch + off);
        std::tm tmv{};
        gmtime_r(&t, &tmv);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", &tmv);
        date = std::string(buf) + " " + tz;
    } catch (...) {
        // leave the defaults if the timestamp is malformed
    }
}

int cmdLog(const Args &args) {
    if (!args.empty()) throw MyGitError("usage: ./mygit log");
    fs::path root = findRepoRoot();

    std::string sha = readHeadCommit(root);
    if (sha.empty()) throw MyGitError("your current branch does not have any commits yet");

    std::string ref = headRef(root);
    std::string headLabel = ref.empty() ? "HEAD" : "HEAD -> " + ref.substr(ref.rfind('/') + 1);

    bool first = true;
    while (!sha.empty()) {
        GitObject obj = readObject(root, sha);
        if (obj.type != "commit") throw MyGitError(sha + " is not a commit object");
        Commit c = parseCommit(obj.content);

        std::string who, date;
        splitIdentity(c.committer, who, date);

        std::cout << "commit " << sha << (first ? " (" + headLabel + ")" : "") << "\n";
        std::cout << "Parent:    " << (c.parents.empty() ? "(none - root commit)" : c.parents[0]) << "\n";
        std::cout << "Committer: " << who << "\n";
        std::cout << "Date:      " << date << "\n";
        std::cout << "Tree:      " << c.tree << "\n\n";
        std::cout << "    " << c.message << "\n\n";

        first = false;
        sha = c.parents.empty() ? "" : c.parents[0];
    }
    return 0;
}
