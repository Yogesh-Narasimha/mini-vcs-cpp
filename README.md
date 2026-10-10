# mygit — Mini Version Control System (AOS Assignment 4)

**Roll No:** 2025201080

A small Git-like version control system written in C++17. It stores files as
content-addressed, zlib-compressed objects identified by their SHA-1 hash, using
the **same object format as Git** (so `mygit hash-object` / `write-tree` produce
the same hashes Git does for identical content).

---

## 1. Build & Run

Requirements: `g++` (C++17), `make`, and the zlib development headers
(`sudo apt install zlib1g-dev` on Ubuntu/Debian).

```bash
make            # builds ./mygit
make clean      # removes build/ and the binary
```

Run commands from inside the directory you want to version:

```bash
./mygit init
echo -n "hello world" > test.txt
./mygit hash-object -w test.txt          # 95d09f2b10159347eece71399a7e2e907ea3df4f
./mygit cat-file -p 95d09f2b10159347eece71399a7e2e907ea3df4f
./mygit write-tree
./mygit ls-tree <tree_sha>
./mygit add .
./mygit commit -m "first commit"
./mygit log
./mygit checkout <commit_sha>
./mygit checkout master                  # return to the latest commit on master
```

Optional: set `MYGIT_AUTHOR_NAME` and `MYGIT_AUTHOR_EMAIL` to control the
committer shown in commits/log (defaults to `$USER` and `$USER@localhost`).

---

## 2. Directory Structure

```
2025201080_Assignment4
├── README.md
├── makefile
├── include/
│   ├── sha1.h        SHA-1 interface
│   ├── utils.h       file I/O, hex, zlib, error type
│   ├── repo.h        repository discovery, HEAD/refs, ignore rules
│   ├── objects.h     blob/tree/commit object database
│   ├── index.h       staging area + tree helpers
│   └── commands.h    one entry point per command
└── src/
    ├── main.cpp            command dispatcher + top-level error handling
    ├── sha1.cpp            SHA-1 implemented from the spec (no external library)
    ├── utils.cpp           helpers (zlib compress/decompress, hex, file I/O)
    ├── repo.cpp            .mygit discovery, HEAD/branch handling
    ├── objects.cpp         object storage, tree & commit (de)serialisation
    ├── index.cpp           index file, building trees from index, flattening trees
    ├── cmd_init.cpp        1. init
    ├── cmd_hash_object.cpp 2. hash-object
    ├── cmd_cat_file.cpp    3. cat-file
    ├── cmd_write_tree.cpp  4. write-tree
    ├── cmd_ls_tree.cpp     5. ls-tree
    ├── cmd_add.cpp         6. add
    ├── cmd_commit.cpp      7. commit
    ├── cmd_log.cpp         8. log
    ├── cmd_checkout.cpp    9. checkout
    └── cmd_status.cpp      extra: status
```

---

## 3. Repository Layout (`.mygit`)

```
.mygit/
├── HEAD            "ref: refs/heads/master"  (or a raw commit SHA when detached)
├── index           staging area: one line per file "<mode> <sha> <path>"
├── objects/xx/yyy… compressed objects (xx = first 2 hex chars of the SHA)
└── refs/heads/master   SHA of the latest commit on master
```

### Object format (identical to Git)
Every object is stored as `zlib("<type> <size>\0<content>")` and its name is the
SHA-1 of the **uncompressed** header + content.

| Type   | Content |
|--------|---------|
| blob   | raw file bytes |
| tree   | repeated `<mode> <name>\0<20-byte raw SHA>`, sorted by name |
| commit | `tree <sha>`, `parent <sha>`, `author …`, `committer …`, blank line, message |

Modes: `100644` normal file, `100755` executable file, `40000` directory.

On every read the object is decompressed and **re-hashed**; if the SHA-1 doesn't
match its name, it is reported as corrupt (integrity check).

---

## 4. Working Procedure of Each Command

### 1. `./mygit init`  — `cmd_init.cpp`
Creates `.mygit/`, `objects/`, `refs/heads/`, an empty `index`, and `HEAD`
pointing to `refs/heads/master`. Running it again prints *Reinitialized …* and
changes nothing.

### 2. `./mygit hash-object [-w] <file>` — `cmd_hash_object.cpp`
Reads the file, builds `blob <size>\0<data>`, and prints its SHA-1. With `-w`
the data is zlib-compressed and saved under `.mygit/objects`. Identical content
always gives the same SHA, so it is stored only once. Works without a repository
when `-w` is not given.

### 3. `./mygit cat-file <-p|-s|-t> <sha>` — `cmd_cat_file.cpp`
Locates the object (full SHA, unique abbreviated SHA of ≥4 chars, or a branch
name), decompresses it and verifies it.
- `-p` prints the content (trees are printed in ls-tree format)
- `-s` prints the content size in bytes
- `-t` prints the type (`blob`, `tree`, `commit`)

### 4. `./mygit write-tree` — `cmd_write_tree.cpp`
Recursively walks the working directory starting at the repository root. Each
file becomes a blob; each sub-directory becomes a tree (built bottom-up). Empty
directories are skipped (like Git). Prints the SHA of the root tree.

### 5. `./mygit ls-tree [--name-only] <tree_sha>` — `cmd_ls_tree.cpp`
Parses a tree object and prints `<mode> <type> <sha>\t<name>` for every entry,
or just names with `--name-only` (directories get a trailing `/`, e.g. `build/`,
as in the assignment example). A commit SHA is also accepted (its tree is
listed).

### 6. `./mygit add . | ./mygit add <file/dir>...` — `cmd_add.cpp`
All pathspecs are validated first (must exist or be tracked, must be inside the
repository, cannot be `.mygit`). Then, for each pathspec, the index entries under
that path are refreshed from disk: new/modified files are stored as blobs and
recorded in the index (`mode sha path`), and tracked files that were deleted
are removed from the index (staged deletion). Paths are relative to the
repository root, so `add` also works from sub-directories.

### 7. `./mygit commit [-m "message"]` — `cmd_commit.cpp`
Builds nested tree objects from the index, then a commit object containing the
tree SHA, parent SHA (current HEAD, if any), author/committer with Unix timestamp
and timezone, and the message (default: `Default commit message`). The branch
HEAD points to (or HEAD itself when detached) is updated. Prints the commit SHA.
Refuses to commit when nothing is staged or the snapshot equals the parent's.

### 8. `./mygit log` — `cmd_log.cpp`
Starts at HEAD and follows parent links, printing for each commit (newest →
oldest): commit SHA, parent SHA, committer, formatted date, tree SHA and
message.

### 9. `./mygit checkout [-f] <commit_sha | branch>` — `cmd_checkout.cpp`
1. Resolves the SHA (full/abbreviated) or branch; errors for invalid,
   nonexistent or non-commit objects.
2. Safety check: refuses if there are uncommitted staged changes, modified
   tracked files, or untracked files that would be overwritten (`-f` overrides).
3. Deletes tracked files not present in the target commit and removes
   directories that become empty.
4. Writes every blob of the target tree to disk (restoring executable bits).
5. Resets the index to the target snapshot and moves HEAD (detached for a SHA,
   attached for a branch name such as `master`).

### Extra: `./mygit status` — `cmd_status.cpp`
Shows staged changes (index vs HEAD), unstaged changes (working dir vs index)
and untracked files. Useful for demonstrating the other commands.

---

## 5. Assumptions

- Only regular files and directories are tracked; symlinks and special files are ignored.
- `.mygit`, `.git` and the running `mygit` executable itself are never tracked.
- `write-tree` snapshots the whole working directory from the repository root
  (as the assignment describes), while `commit` snapshots the **index** (like Git).
- Only one branch (`master`) is created; checking out a commit SHA gives a
  detached HEAD. New commits made while detached advance HEAD only;
  `./mygit checkout master` returns to the latest master commit.
- `log` follows the first parent (no merges exist in this system).
- The index is a simple text file (not Git's binary format); paths containing
  newline characters are not supported.
- File modes stored: `100644` / `100755` (based on the owner execute bit) and `40000`.

---

## 6. Libraries Used

- C++17 STL and `<filesystem>`
- **zlib** (`-lz`) for compression (allowed: compression algorithm)
- SHA-1 is implemented by hand in `sha1.cpp` (no OpenSSL)
