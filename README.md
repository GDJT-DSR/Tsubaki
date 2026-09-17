# Tsubaki

A command-line checksum utility for file integrity verification, written in modern C++20.

[中文文档](README.zh-cn.md)

## Features

- **Checksums** – Compute checksums for files and directories using any digest
  supported by OpenSSL (MD4/MD5, SHA-1, SHA-2, SHA-3, SHAKE, BLAKE2).
- **Recursive scanning** – Scan directories recursively, with optional symlink
  following and permission-denied tolerance.
- **Multiple inputs** – Accept regular files, directories, a tsubaki-format
  checksum list on stdin, or a plain path list on stdin.
- **Filtering** – Exclude path prefixes and filter by minimum/maximum file size.
- **Multi-threading** – Hash files in parallel with a thread pool sized to the
  hardware; read buffers are sized dynamically per file.
- **Resumable** – Already-computed hashes from the input list are reused unless
  `--force-scan` is given, so an interrupted run can be resumed.
- **Clean output** – Results go to stdout, logs to stderr, with a summary report
  appended at the end.

## Requirements

- A C++20 compiler (GCC 13+, Clang 16+, or Apple Clang 15+)
- CMake 3.16+
- OpenSSL development files
- Linux or macOS (POSIX)

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is produced at `build/tsubaki`.

### Options

| CMake option | Default | Description |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | – | `Release` is recommended for hashing performance |
| `BUILD_TESTING` | `ON` | Build the test suite (requires GoogleTest) |
| `TSUBAKI_SANITIZE` | `OFF` | Build with AddressSanitizer and UBSan |

## Usage

```text
tsubaki <command> [options] [paths...]
```

### Commands

| Command | Description |
| --- | --- |
| `sum <algorithm> <path...>` | Compute checksums for files and directories |
| `help` | Show help (English) |

### Inputs for `sum`

| Input | Description |
| --- | --- |
| `<path>` | A regular file, or a directory scanned recursively |
| `stdin` | Read a tsubaki-format checksum list from stdin |
| `stdin-plain-list` | Read a plain list of paths from stdin, one per line |

### Algorithms

`md4`, `md5`, `sha1`, `sha224`, `sha256`, `sha384`, `sha512`, `sha512_224`,
`sha512_256`, `sha3_224`, `sha3_256`, `sha3_384`, `sha3_512`, `shake128`,
`shake256`, `blake2b512`, `blake2s256`.

### Options

| Option | Description |
| --- | --- |
| `--exclude=PATH` | Exclude paths beginning with `PATH` (repeatable) |
| `--min-size=SIZE` | Only include files not smaller than `SIZE` |
| `--max-size=SIZE` | Only include files not larger than `SIZE` |
| `--force-scan` | Recompute hashes even if present in the input list |
| `--allow-symlinks` | Follow directory symlinks while scanning |
| `--test` | Scan and report only; do not compute checksums |
| `--log-level=LEVEL` | `DEBUG`, `INFO`, `WARN`, or `ERROR` (default `INFO`) |
| `--quiet` | Deprecated. Same as `--log-level=ERROR` |
| `-v` | Deprecated. Same as `--log-level=INFO` |
| `-h`, `--help` | Show help in English |
| `--help-cn` | Show help in Chinese |

`SIZE` accepts an optional unit suffix: `b`, `k`, `m`, `g`, `t`, `p`
(binary, so `1k` = 1024 bytes).

### Output

Each processed file produces one line on stdout:

```text
<hash> <path>
<NONE> <path>          # the file could not be processed
```

A summary report is appended at the end:

```text
#
# ----------General Report----------
# Total: 3
# Succeed: 3
# Failed: 0
# Unprocessed: 0
# Time started: 2026-09-17 12:00:00
# Time finished: 2026-09-17 12:00:01
# Duration: 1.23s
# Command: tsubaki sum sha256 ./data
```

### Exit status

| Code | Meaning |
| --- | --- |
| `0` | Success |
| `1` | Invalid arguments, unsupported algorithm, or one or more failed files |

## Examples

```sh
# Checksum a single file
tsubaki sum sha256 ./archive.tar.gz

# Recursively checksum a directory, skipping a subdirectory and tiny files
tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1k

# Write results, then resume an interrupted run from the partial output
tsubaki sum sha256 ./data > partial.txt
cat partial.txt | tsubaki sum sha256 stdin > complete.txt

# Read a plain path list from stdin
printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list

# Inspect the scan/filter pipeline without hashing
tsubaki sum sha256 ./data --test

# Chinese help
tsubaki --help-cn
```

## Testing

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The suite uses GoogleTest, located via `find_package(GTest)` or fetched
automatically by CMake if it is not installed.

## Project layout

```text
core/
  common/   Argument-driven logging setup, checksum-list parsing, help text
  sum/      `sum` command: scanning, filtering and hash computation
plugins/
  arg_parser.*   Command-line parsing
  encoder.*      OpenSSL digest lookup and file hashing
  file_list.*    File metadata container
  logger.*       Leveled logger
  thread_pool.*  Worker pool used for hashing
  trie.*         Path-prefix matcher used by --exclude
tests/      GoogleTest suite
```

## Notes and limitations

- Only regular files are checksummed; directory symlinks are not followed unless
  `--allow-symlinks` is set.
- Subdirectories that cannot be accessed are skipped with a warning.
- Paths are normalized to absolute paths in the output.
- `SIGINT` stops the worker pool gracefully; files already hashed are still
  printed, which makes the output suitable for resuming.
- The file list is fully enumerated in memory before filtering, which is a
  bottleneck for very large trees.

## License

Released under the MIT License. See [LICENSE](LICENSE) for details.

## Author

- **Lawrence Charland**
