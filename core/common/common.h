#ifndef _CORE_COMMON_COMMON_H_
#define _CORE_COMMON_COMMON_H_

#include "plugins/file_list.h"
#include <istream>
#include <string_view>
namespace common {

void setLogLevel();

void loadFilesFromStream(plugins::FileList &fl, std::istream &is,
                         std::string_view name, std::string_view mode);
} // namespace common

#endif // !_CORE_COMMON_COMMON_H_
