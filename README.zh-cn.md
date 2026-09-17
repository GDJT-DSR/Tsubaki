# Tsubaki

一个用于文件完整性校验的命令行校验和工具，使用现代 C++20 编写。

[English](README.md)

## 功能特性

- **校验和计算** – 对文件与目录计算校验和，支持 OpenSSL 提供的全部摘要算法
  （MD4/MD5、SHA-1、SHA-2、SHA-3、SHAKE、BLAKE2）。
- **递归扫描** – 递归扫描目录，可选择跟随符号链接并容忍权限不足。
- **多种输入** – 支持普通文件、目录，以及从标准输入读取的 tsubaki 格式校验和
  列表或纯路径列表。
- **过滤** – 支持排除路径前缀，并按文件大小上下限过滤。
- **多线程** – 使用按硬件并发数配置的线程池并行计算，读取缓冲区按文件大小动态
  调整。
- **可续算** – 除非指定 `--force-scan`，输入列表中已有的哈希会被直接复用，因此
  中断后的任务可以续算。
- **输出清晰** – 结果输出到 stdout，日志输出到 stderr，末尾附带汇总报告。

## 环境要求

- 支持 C++20 的编译器（GCC 13+、Clang 16+ 或 Apple Clang 15+）
- CMake 3.16+
- OpenSSL 开发库
- Linux 或 macOS（POSIX）

## 构建

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

可执行文件位于 `build/tsubaki`。

### CMake 选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | – | 计算校验和推荐使用 `Release` |
| `BUILD_TESTING` | `ON` | 构建测试（需要 GoogleTest） |
| `TSUBAKI_SANITIZE` | `OFF` | 开启 AddressSanitizer 与 UBSan |

## 用法

```text
tsubaki <命令> [选项] [路径...]
```

### 命令

| 命令 | 说明 |
| --- | --- |
| `sum <算法> <路径...>` | 计算文件与目录的校验和 |
| `help` | 显示帮助（英文） |

### `sum` 的输入

| 输入 | 说明 |
| --- | --- |
| `<路径>` | 普通文件，或将被递归扫描的目录 |
| `stdin` | 从标准输入读取 tsubaki 格式的校验和列表 |
| `stdin-plain-list` | 从标准输入读取纯路径列表，每行一个 |

### 算法

`md4`、`md5`、`sha1`、`sha224`、`sha256`、`sha384`、`sha512`、`sha512_224`、
`sha512_256`、`sha3_224`、`sha3_256`、`sha3_384`、`sha3_512`、`shake128`、
`shake256`、`blake2b512`、`blake2s256`。

### 选项

| 选项 | 说明 |
| --- | --- |
| `--exclude=PATH` | 排除以 `PATH` 开头的路径（可重复） |
| `--min-size=SIZE` | 仅包含不小于 `SIZE` 的文件 |
| `--max-size=SIZE` | 仅包含不大于 `SIZE` 的文件 |
| `--force-scan` | 即使输入列表中已有哈希也重新计算 |
| `--allow-symlinks` | 扫描时跟随目录符号链接 |
| `--test` | 仅扫描与统计，不计算校验和 |
| `--log-level=LEVEL` | `DEBUG`、`INFO`、`WARN` 或 `ERROR`（默认 `INFO`） |
| `--quiet` | 已弃用，等价于 `--log-level=ERROR` |
| `-v` | 已弃用，等价于 `--log-level=INFO` |
| `-h`、`--help` | 显示英文帮助 |
| `--help-cn` | 显示中文帮助 |

`SIZE` 可带单位后缀：`b`、`k`、`m`、`g`、`t`、`p`（二进制，即 `1k` = 1024 字节）。

### 输出

每个已处理的文件在 stdout 输出一行：

```text
<哈希> <路径>
<NONE> <路径>          # 该文件无法处理
```

末尾附带汇总报告：

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

### 退出码

| 退出码 | 含义 |
| --- | --- |
| `0` | 成功 |
| `1` | 参数错误、算法不支持，或存在处理失败的文件 |

## 示例

```sh
# 计算单个文件
tsubaki sum sha256 ./archive.tar.gz

# 递归计算目录，排除子目录并忽略过小的文件
tsubaki sum sha256 ./data --exclude=./data/.cache --min-size=1k

# 输出结果，并在中断后从部分结果续算
tsubaki sum sha256 ./data > partial.txt
cat partial.txt | tsubaki sum sha256 stdin > complete.txt

# 从标准输入读取纯路径列表
printf 'a.txt\nb.txt\n' | tsubaki sum sha256 stdin-plain-list

# 只查看扫描/过滤结果，不计算哈希
tsubaki sum sha256 ./data --test

# 中文帮助
tsubaki --help-cn
```

## 测试

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

测试基于 GoogleTest：优先通过 `find_package(GTest)` 查找，未安装时由 CMake
自动下载。

## 目录结构

```text
core/
  common/   日志级别配置、校验和列表解析、帮助文本
  sum/      `sum` 命令：扫描、过滤与哈希计算
plugins/
  arg_parser.*   命令行解析
  encoder.*      OpenSSL 摘要查找与文件哈希
  file_list.*    文件元数据容器
  logger.*       分级日志
  thread_pool.*  用于哈希计算的线程池
  trie.*         `--exclude` 使用的路径前缀匹配
tests/      GoogleTest 测试套件
```

## 说明与限制

- 仅对普通文件计算校验和；除非指定 `--allow-symlinks`，否则不跟随目录符号链接。
- 无法访问的子目录会被跳过并给出警告。
- 输出中的路径会被规范化为绝对路径。
- `SIGINT` 会优雅地停止线程池；已完成的哈希仍会输出，因此结果可用于续算。
- 文件列表在过滤前会完整载入内存，超大目录树会成为瓶颈。

## 许可证

基于 MIT 许可证发布，详见 [LICENSE](LICENSE)。

## 作者

- **Lawrence Charland**
