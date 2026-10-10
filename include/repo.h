// repo.h - Repository layout, discovery, HEAD/ref handling and ignore rules.
#ifndef MYGIT_REPO_H
#define MYGIT_REPO_H

#include "utils.h"

#include <string>

// Name of the metadata directory.
const std::string MYGIT_DIR = ".mygit";
const std::string DEFAULT_BRANCH = "master";

// Walks up from the current directory until a folder containing .mygit is found.
// Throws MyGitError if the current directory is not inside a repository.
fs::path findRepoRoot();

// <root>/.mygit
fs::path metaDir(const fs::path &root);

// Returns the commit SHA HEAD currently points to, or "" if there are no commits yet.
std::string readHeadCommit(const fs::path &root);

// Returns "refs/heads/<name>" if HEAD is attached to a branch, "" if detached.
std::string headRef(const fs::path &root);

// Moves HEAD (or the branch HEAD points to) to the given commit.
void advanceHead(const fs::path &root, const std::string &commitSha);

// Points HEAD at a branch ref (attached) or directly at a commit (detached).
void setHeadToRef(const fs::path &root, const std::string &ref);
void setHeadDetached(const fs::path &root, const std::string &commitSha);

// Returns the commit SHA stored in refs/heads/<branch>, or "" if it doesn't exist.
std::string readBranch(const fs::path &root, const std::string &branch);

// True for paths that must never be tracked: .mygit, .git and the mygit binary itself.
bool isIgnored(const fs::path &absPath);

#endif
