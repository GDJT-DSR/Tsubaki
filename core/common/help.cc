#include "help.h"
#include <iostream>

namespace {

constexpr const char *kHelpEn = R"(Tsubaki - a checksum utility for file integrity, comparison and duplicate detection.

Usage:
  tsubaki <command> [options] [paths...]

Commands:
  sum <algorithm> <path...>   Compute checksums for files and directories
  help                        Show this help (English)

Inputs for 'sum':
  <path>              A regular file or a directory (scanned recursively)
  stdin               Read a tsubaki-format checksum list from stdin
  stdin-plain-list    Read a plain list of paths from stdin, one per line

Algorithms:
  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

Options:
  --exclude=PATH      Exclude paths beginning with PATH (repeatable)
  --min-size=SIZE     Only include files not smaller than SIZE
  --max-size=SIZE     Only include files not larger than SIZE
  --force-scan        Recompute hashes even if present in the input list
  --allow-symlinks    Follow directory symlinks while scanning
  --test              Scan and report only; do not compute checksums
  --log-level=LEVEL   Log verbosity: DEBUG, INFO, WARN, ERROR (default: INFO)
  --quiet             Deprecated. Same as --log-level=ERROR
  -v                  Deprecated. Same as --log-level=INFO
  -h, --help          Show this help in English
  --help-cn           Show this help in Chinese

SIZE units: b, k, m, g, t, p (binary, 1k = 1024)

Output:
  <hash> <path>       One line per processed file
  <NONE> <path>       The file could not be processed
  # ...               Summary report appended at the end

Exit status:
  0   success
  1   invalid arguments, unsupported algorithm, or one or more failed files

Examples:
  tsubaki sum sha256 ./data
  tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1m
  cat partial.txt | tsubaki sum sha256 stdin
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
)";

constexpr const char *kHelpCn = R"(Tsubaki - 用于文件完整性校验、比较与重复文件检测的校验和工具。

用法：
  tsubaki <命令> [选项] [路径...]

命令：
  sum <算法> <路径...>    计算文件/目录的校验和
  help                    显示帮助（英文）

sum 的输入：
  <路径>              普通文件或目录（递归扫描）
  stdin               从标准输入读取 tsubaki 格式的校验和列表
  stdin-plain-list    从标准输入读取纯路径列表，每行一个

算法：
  md4 md5 sha1 sha224 sha256 sha384 sha512 sha512_224 sha512_256
  sha3_224 sha3_256 sha3_384 sha3_512 shake128 shake256 blake2b512 blake2s256

选项：
  --exclude=PATH      排除以 PATH 开头的路径（可重复）
  --min-size=SIZE     仅包含不小于 SIZE 的文件
  --max-size=SIZE     仅包含不大于 SIZE 的文件
  --force-scan        即使输入列表中已有哈希也重新计算
  --allow-symlinks    扫描时跟随目录符号链接
  --test              仅扫描与统计，不计算校验和
  --log-level=LEVEL   日志级别：DEBUG、INFO、WARN、ERROR（默认 INFO）
  --quiet             已弃用，等价于 --log-level=ERROR
  -v                  已弃用，等价于 --log-level=INFO
  -h, --help          显示英文帮助
  --help-cn           显示中文帮助

SIZE 单位：b、k、m、g、t、p（二进制，1k = 1024）

输出：
  <哈希> <路径>       每个已处理文件一行
  <NONE> <路径>       该文件无法处理
  # ...              末尾附带的汇总报告

退出码：
  0   成功
  1   参数错误、算法不支持，或存在处理失败的文件

示例：
  tsubaki sum sha256 ./data
  tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1m
  cat partial.txt | tsubaki sum sha256 stdin
  printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list
)";

} // namespace

void common::printHelp(bool chinese) {
    std::cout << (chinese ? kHelpCn : kHelpEn);
}
