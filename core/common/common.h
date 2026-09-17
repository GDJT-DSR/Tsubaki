#ifndef _CORE_COMMON_COMMON_H_
#define _CORE_COMMON_COMMON_H_

#include "plugins/file_list.h"
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
namespace common {

void setLogLevel();

// 将字节数格式化为可读字符串，例如 1536 -> "1.50 KiB"
std::string formatFileSize(uintmax_t bytes);

void loadFilesFromStream(plugins::FileList &fl, std::istream &is,
                         std::string_view name, std::string_view mode);
} // namespace common

#endif // !_CORE_COMMON_COMMON_H_
