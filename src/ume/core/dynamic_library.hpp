#pragma once

#include <filesystem>

namespace ume {
[[nodiscard]] void *openLibrary(const std::filesystem::path &path);
[[nodiscard]] void *findSymbol(void *library, const char *symbol);
void closeLibrary(void *library);
} // namespace ume