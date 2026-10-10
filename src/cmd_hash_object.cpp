// cmd_hash_object.cpp - "./mygit hash-object [-w] <file>"
// Prints the blob SHA-1 of a file; with -w the compressed blob is stored in .mygit/objects.
#include "commands.h"
#include "objects.h"
#include "repo.h"

#include <iostream>
#include <system_error>

int cmdHashObject(const Args &args) {
    bool write = false;
    std::vector<std::string> files;
    for (const auto &a : args) {
        if (a == "-w") write = true;
        else if (!a.empty() && a[0] == '-') throw MyGitError("unknown option '" + a + "'\nusage: ./mygit hash-object [-w] <file>");
        else files.push_back(a);
    }
    if (files.empty()) throw MyGitError("usage: ./mygit hash-object [-w] <file>");

    // A repository is only required when writing.
    fs::path root;
    if (write) root = findRepoRoot();

    for (const auto &f : files) {
        std::error_code ec;
        if (!fs::exists(f, ec)) throw MyGitError("could not open '" + f + "': No such file");
        if (!fs::is_regular_file(f, ec)) throw MyGitError("'" + f + "' is not a regular file");
        std::string content = readFile(f);
        std::string sha = write ? hashObject(root, "blob", content, true)
                                : computeObjectSha("blob", content);
        std::cout << sha << "\n";
    }
    return 0;
}
