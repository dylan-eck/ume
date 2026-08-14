#pragma once

namespace ume {
[[nodiscard]] void *openLibrary(const char *path) { return nullptr; }
[[nodiscard]] void *findSymbol(void *library, const char *symbol) {
    return nullptr;
}
void closeLibrary(void *library) {}
} // namespace ume