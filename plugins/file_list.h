#ifndef _PLUGINS_FILE_LIST_H_
#define _PLUGINS_FILE_LIST_H_

#include <cstdint>
#include <future>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unordered_map>
// #include <unordered_set>

namespace plugins {

class FileList {
    struct FileInfo {
        uintmax_t size;
        std::string hash;
        std::future<std::string> fut;
    };

  public:
    using filesum_t = std::unordered_map<std::string, FileInfo>;

  private:
    filesum_t m_filesums;
    // std::unordered_map<std::string_view,
    // std::unordered_set<std::string_view>>
    //     m_file_by_sums;

  public:
    FileList() {}
    ~FileList() {}

    // 添加文件，返回原先是否存在
    bool add(std::string);
    // 设置hash
    void set(std::string_view, std::string_view);

    std::optional<std::string_view> getHash(const std::string &);
    // void build_reverse();
    void initSizes();
    inline filesum_t *operator->() { return &m_filesums; }
    inline filesum_t &operator*() { return m_filesums; }
    uintmax_t getTotalSize(bool);
    // std::unordered_set<std::string_view> getFilesByHash(const std::string &);
};
} // namespace plugins

#endif // !_PLUGINS_FILE_LIST_H_
