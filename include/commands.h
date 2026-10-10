// commands.h - Entry points for every mygit sub-command.
// Each function receives the arguments that follow the sub-command name
// and returns the process exit code. Errors are reported by throwing MyGitError.
#ifndef MYGIT_COMMANDS_H
#define MYGIT_COMMANDS_H

#include <string>
#include <vector>

using Args = std::vector<std::string>;

int cmdInit(const Args &args);        // 1. init          (cmd_init.cpp)
int cmdHashObject(const Args &args);  // 2. hash-object   (cmd_hash_object.cpp)
int cmdCatFile(const Args &args);     // 3. cat-file      (cmd_cat_file.cpp)
int cmdWriteTree(const Args &args);   // 4. write-tree    (cmd_write_tree.cpp)
int cmdLsTree(const Args &args);      // 5. ls-tree       (cmd_ls_tree.cpp)
int cmdAdd(const Args &args);         // 6. add           (cmd_add.cpp)
int cmdCommit(const Args &args);      // 7. commit        (cmd_commit.cpp)
int cmdLog(const Args &args);         // 8. log           (cmd_log.cpp)
int cmdCheckout(const Args &args);    // 9. checkout      (cmd_checkout.cpp)
int cmdStatus(const Args &args);      // extra: status    (cmd_status.cpp)

#endif
