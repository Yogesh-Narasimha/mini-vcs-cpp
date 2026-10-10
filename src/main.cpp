// main.cpp - Command-line dispatcher for mygit.
#include "commands.h"
#include "utils.h"

#include <functional>
#include <iostream>
#include <map>

static void printUsage() {
    std::cerr <<
        "usage: ./mygit <command> [<args>]\n\n"
        "Commands:\n"
        "  init                              Create an empty repository (.mygit)\n"
        "  hash-object [-w] <file>           Compute SHA-1 of a file; -w stores it as a blob\n"
        "  cat-file (-p|-s|-t) <sha>         Show content / size / type of an object\n"
        "  write-tree                        Snapshot the working directory as a tree object\n"
        "  ls-tree [--name-only] <tree-sha>  List the contents of a tree object\n"
        "  add <path>... | add .             Stage files for the next commit\n"
        "  commit [-m <message>]             Record the staged snapshot\n"
        "  log                               Show commit history (newest first)\n"
        "  checkout [-f] <commit-sha|branch> Restore the working directory to a commit\n"
        "  status                            Show staged / modified / untracked files\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    static const std::map<std::string, std::function<int(const Args &)>> commands = {
        {"init", cmdInit},         {"hash-object", cmdHashObject}, {"cat-file", cmdCatFile},
        {"write-tree", cmdWriteTree}, {"ls-tree", cmdLsTree},      {"add", cmdAdd},
        {"commit", cmdCommit},     {"log", cmdLog},                {"checkout", cmdCheckout},
        {"status", cmdStatus},
    };

    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        printUsage();
        return 0;
    }
    auto it = commands.find(cmd);
    if (it == commands.end()) {
        std::cerr << "mygit: '" << cmd << "' is not a mygit command.\n\n";
        printUsage();
        return 1;
    }

    Args args(argv + 2, argv + argc);
    try {
        return it->second(args);
    } catch (const MyGitError &e) {
        std::cerr << "fatal: " << e.what() << "\n";
    } catch (const fs::filesystem_error &e) {
        std::cerr << "fatal: filesystem error: " << e.what() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "fatal: " << e.what() << "\n";
    }
    return 1;
}
