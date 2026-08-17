#pragma once

#include "ume/plugin/plugin_api.h"

#include <string>
#include <unordered_map>

namespace ume {

using NumberMap = std::unordered_map<std::string, double>;

[[nodiscard]] UmeParams makeNumberParams(const NumberMap &values);
} // namespace ume