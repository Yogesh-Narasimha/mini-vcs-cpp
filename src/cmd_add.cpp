// cmd_add.cpp - "./mygit add ." / "./mygit add <path>..."
// For every path given (file or directory), the index entries under that path are
// replaced by the current contents of the working directory:
//   * new / modified files are hashed, stored as blobs and staged
//   * tracked files that were deleted from disk are removed from the index
#include "commands.h"
#include "index.h"
#include "objects.h"
#include "repo.h"

#include <iostream>
#include <system_error>

// True if index path `p` is equal to or lies under the pathspec `spec` ("" = everything).
static bool coveredBy(const std::string &p, const std::string &spec) {
    if (spec.empty()) return true;
    return p == spec || (p.size() > spec.size() && p.compare(0, spec.size(), spec) == 0 && p[spec.size()] == '/');
}

int cmdAdd(const Args &args) {
    if (args.empty()) throw MyGitError("Nothing specified, nothing added.\nusage: ./mygit add <path>... | ./mygit add .");

    fs::path root = findRepoRoot();
    fs::path cwd = fs::current_path();
    Index idx = readIndex(root);

    // Pass 1: validate every pathspec before touching the index.
    std::vector<std::pair<std::string, fs::path>> specs;   // (repo-relative spec, absolute path)
    for (const auto &a : args) {
        if (!a.empty() && a[0] == '-') throw MyGitError("unknown option '" + a + "'");
        fs::path abs = (cwd / a).lexically_normal();
        if (abs.has_filename() == false) abs = abs.parent_path();   // strip trailing '/'
        std::string rel = relativeTo(abs, root);
        if (rel == ".") rel = "";
        if (rel == ".." || rel.compare(0, 3, "../") == 0)
            throw MyGitError("'" + a + "' is outside repository");
        if (rel == MYGIT_DIR || rel.compare(0, MYGIT_DIR.size() + 1, MYGIT_DIR + "/") == 0)
            throw MyGitError("cannot add files inside " + MYGIT_DIR);

        std::error_code ec;
        bool existsOnDisk = fs::exists(abs, ec);
        bool tracked = false;
        for (const auto &kv : idx)
            if (coveredBy(kv.first, rel)) { tracked = true; break; }
        if (!existsOnDisk && !tracked) throw MyGitError("pathspec '" + a + "' did not match any files");
        specs.push_back({rel, abs});
    }

    // Pass 2: refresh the index for each pathspec.
    size_t staged = 0, removed = 0;
    for (const auto &spec : specs) {
        std::map<std::string, fs::path> files;
        std::error_code ec;
        if (fs::exists(spec.second, ec)) collectWorkingFiles(root, spec.second, files);

        for (auto it = idx.begin(); it != idx.end();) {              // stage deletions
            if (coveredBy(it->first, spec.first) && !files.count(it->first)) {
                it = idx.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        for (const auto &f : files) {                                 // stage additions/changes
            IndexEntry e{fileModeOf(f.second), hashObject(root, "blob", readFile(f.second), true)};
            auto old = idx.find(f.first);
            if (old == idx.end() || old->second != e) ++staged;
            idx[f.first] = e;
        }
    }
    writeIndex(root, idx);
    std::cout << "Staged " << staged << " new/modified file(s)";
    if (removed) std::cout << ", " << removed << " deletion(s)";
    std::cout << ". Index now tracks " << idx.size() << " file(s).\n";
    return 0;
}
