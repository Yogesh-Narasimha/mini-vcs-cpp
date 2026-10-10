// cmd_ls_tree.cpp - "./mygit ls-tree [--name-only] <tree-sha>"
// Also accepts a commit SHA (lists that commit's root tree), like Git.
#include "commands.h"
#include "objects.h"
#include "repo.h"

#include <iostream>

int cmdLsTree(const Args &args) {
    const std::string usage = "usage: ./mygit ls-tree [--name-only] <tree-sha>";
    bool nameOnly = false;
    std::string target;
    for (const auto &a : args) {
        if (a == "--name-only") nameOnly = true;
        else if (!a.empty() && a[0] == '-') throw MyGitError("unknown option '" + a + "'\n" + usage);
        else if (target.empty()) target = a;
        else throw MyGitError(usage);
    }
    if (target.empty()) throw MyGitError(usage);

    fs::path root = findRepoRoot();
    std::string sha = resolveObject(root, target);
    GitObject obj = readObject(root, sha);
    if (obj.type == "commit") {                   // tree-ish: dereference commit -> tree
        sha = parseCommit(obj.content).tree;
        obj = readObject(root, sha);
    }
    if (obj.type != "tree") throw MyGitError(target + " is not a tree object (it is a " + obj.type + ")");

    for (const auto &e : parseTree(obj.content)) {
        if (nameOnly) std::cout << e.name << (typeFromMode(e.mode) == "tree" ? "/" : "") << "\n";   // dirs shown as "name/" (as in the assignment example)
        else std::cout << displayMode(e.mode) << " " << typeFromMode(e.mode) << " " << e.sha << "\t" << e.name << "\n";
    }
    return 0;
}
