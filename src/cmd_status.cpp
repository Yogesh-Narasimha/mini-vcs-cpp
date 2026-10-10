// cmd_status.cpp - "./mygit status" (extra helper command)
// Compares HEAD, the index and the working directory.
#include "commands.h"
#include "index.h"
#include "objects.h"
#include "repo.h"

#include <iostream>

int cmdStatus(const Args &args) {
    if (!args.empty()) throw MyGitError("usage: ./mygit status");
    fs::path root = findRepoRoot();

    std::string ref = headRef(root);
    std::string head = readHeadCommit(root);
    if (ref.empty()) std::cout << "HEAD detached at " << head.substr(0, 7) << "\n";
    else std::cout << "On branch " << ref.substr(ref.rfind('/') + 1) << (head.empty() ? " (no commits yet)" : "") << "\n";

    Index headFiles = headSnapshot(root);
    Index idx = readIndex(root);
    std::map<std::string, fs::path> work;
    collectWorkingFiles(root, root, work);

    std::vector<std::string> staged, unstaged, untracked;
    for (const auto &kv : idx) {
        auto h = headFiles.find(kv.first);
        if (h == headFiles.end()) staged.push_back("new file:  " + kv.first);
        else if (h->second != kv.second) staged.push_back("modified:  " + kv.first);
    }
    for (const auto &kv : headFiles)
        if (!idx.count(kv.first)) staged.push_back("deleted:   " + kv.first);

    for (const auto &kv : idx) {
        auto w = work.find(kv.first);
        if (w == work.end()) unstaged.push_back("deleted:   " + kv.first);
        else if (computeObjectSha("blob", readFile(w->second)) != kv.second.sha) unstaged.push_back("modified:  " + kv.first);
    }
    for (const auto &kv : work)
        if (!idx.count(kv.first)) untracked.push_back(kv.first);

    auto section = [](const std::string &title, const std::vector<std::string> &items) {
        if (items.empty()) return;
        std::cout << "\n" << title << ":\n";
        for (const auto &s : items) std::cout << "\t" << s << "\n";
    };
    section("Changes to be committed", staged);
    section("Changes not staged for commit", unstaged);
    section("Untracked files", untracked);
    if (staged.empty() && unstaged.empty() && untracked.empty()) std::cout << "nothing to commit, working tree clean\n";
    return 0;
}
