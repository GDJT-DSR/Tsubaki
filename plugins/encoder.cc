#include "encoder.h"
#include <_stdio.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <openssl/evp.h>
#include <stdexcept>
#include <sys/_types/_off_t.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace plugins;

namespace {

size_t getBufferSize(off_t size) {

    if (size < 0x400000) {
        if (size < 0x40000) {
            // <256k -> 4k
            return 0x1000;
        } else {
            // [256k,4m) -> 8k
            return 0x2000;
        }
    } else {
        if (size < 0x4000000) {
            // [4m,64m) -> 16k
            return 0x4000;
        } else {
            // >=64m -> 64k
            return 0x10000;
        }
    }
}

}; // namespace

const EVP_MD *Encoder::getMdByName(std::string_view name) const {
    if (const auto it = map.find(name); it != map.end()) {
        return it->second;
    }
    return nullptr;
}
std::string
Encoder::encodeFile(std::string_view path, uintmax_t file_size,
                    const EVP_MD *md,
                    std::string (*parse)(std::vector<unsigned char>)) {
    // 获取文件
    FILE *fp = std::fopen(path.data(), "rb");
    if (!fp)
        throw std::runtime_error(std::format("Sum: open file {} error.", path));

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, nullptr);

    // 获取文件大小并计算缓冲区大小
    const size_t buffer_size = getBufferSize(file_size);

    // 创建缓冲区并读取hash，线程复用
    thread_local static std::vector<unsigned char> buf;
    buf.clear();
    buf.resize(buffer_size);

    size_t read_size;
    while ((read_size = std::fread(buf.data(), sizeof(unsigned char),
                                   buffer_size, fp)) > 0) {
        EVP_DigestUpdate(ctx, buf.data(), read_size);
    }

    // 判断是否成功
    if (std::ferror(fp)) {
        std::fclose(fp);
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error(std::format("Sum: read file {} error.", path));
    }
    std::fclose(fp);

    std::vector<unsigned char> digest(EVP_MAX_MD_SIZE);
    unsigned int len = 0;
    EVP_DigestFinal_ex(ctx, digest.data(), &len);

    EVP_MD_CTX_free(ctx);
    digest.resize(len);
    return parse(std::move(digest));
}

std::string Encoder::toHex(std::vector<unsigned char> digest) {
    static const char *map = "0123456789abcdef";
    std::string res;
    res.reserve(digest.size() << 1);
    for (unsigned char c : digest) {
        res += (map[c >> 4]);
        res += (map[c & 15]);
    }
    return res;
}
