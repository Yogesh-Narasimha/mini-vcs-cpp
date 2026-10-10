// cmd_cat_file.cpp - "./mygit cat-file (-p|-s|-t) <sha>"
//   -p : pretty-print content (trees are shown like ls-tree)
//   -s : size of the object content in bytes
//   -t : object type (blob / tree / commit)
#include "commands.h"
#include "objects.h"
#include "repo.h"

#include <iostream>

int cmdCatFile(const Args &args) {
    const std::string usage = "usage: ./mygit cat-file (-p | -s | -t) <object-sha>";
    if (args.size() != 2) throw MyGitError(usage);

    const std::string &flag = args[0];
    if (flag != "-p" && flag != "-s" && flag != "-t")
        throw MyGitError("unknown flag '" + flag + "'\n" + usage);

    fs::path root = findRepoRoot();
    std::string sha = resolveObject(root, args[1]);
    GitObject obj = readObject(root, sha);

    if (flag == "-t") {
        std::cout << obj.type << "\n";
    } else if (flag == "-s") {
        std::cout << obj.content.size() << "\n";
    } else if (obj.type == "tree") {
        for (const auto &e : parseTree(obj.content))
            std::cout << displayMode(e.mode) << " " << typeFromMode(e.mode) << " " << e.sha << "\t" << e.name << "\n";
    } else {
        std::cout.write(obj.content.data(), static_cast<std::streamsize>(obj.content.size()));
        std::cout.flush();
    }
    return 0;
}
