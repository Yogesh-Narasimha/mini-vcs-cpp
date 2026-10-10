// cmd_init.cpp - "./mygit init": creates the .mygit directory structure.
//
//   .mygit/
//   ├── HEAD              "ref: refs/heads/master"
//   ├── index             staging area (empty)
//   ├── objects/          compressed blob / tree / commit objects
//   └── refs/heads/       branch pointers
#include "commands.h"
#include "repo.h"

#include <iostream>
#include <system_error>

int cmdInit(const Args &args) {
    if (!args.empty()) throw MyGitError("usage: ./mygit init");

    fs::path root = fs::current_path();
    fs::path meta = metaDir(root);
    std::error_code ec;
    if (fs::exists(meta / "HEAD", ec)) {
        std::cout << "Reinitialized existing mygit repository in " << fs::absolute(meta).string() << "/\n";
        return 0;
    }

    fs::create_directories(meta / "objects", ec);
    if (!ec) fs::create_directories(meta / "refs" / "heads", ec);
    if (ec) throw MyGitError("cannot create repository: " + ec.message());

    setHeadToRef(root, "refs/heads/" + DEFAULT_BRANCH);
    writeFile(meta / "index", "");

    std::cout << "Initialized empty mygit repository in " << fs::absolute(meta).string() << "/\n";
    return 0;
}
