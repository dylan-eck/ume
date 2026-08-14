#include "dynamic_library.hpp"
#include "ume/core/logger.hpp"
#include <dlfcn.h>

// TODO: Everything currently in this file will not work on windows

namespace ume {
namespace {
std::filesystem::path absolutePath(const std::filesystem::path &path) {
    std::error_code error_code;
    std::filesystem::path result = std::filesystem::absolute(path, error_code);
    return error_code ? path : result;
}
} // namespace

void *openLibrary(const std::filesystem::path &path) {
    const std::filesystem::path full = absolutePath(path);

    ::dlerror();
    void *library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (library == nullptr) {
        const char *error = ::dlerror();
        UME_LOG_ERROR(Plugin, "failed to load '{}': {}", full.string(),
                      error != nullptr ? error : "unknown error");
        return nullptr;
    }

    return library;
}

void *findSymbol(void *library, const char *symbol) {
    if (library == nullptr || symbol == nullptr) {
        return nullptr;
    }

    ::dlerror();
    void *address = ::dlsym(library, symbol);
    if (const char *error = ::dlerror(); error != nullptr) {
        UME_LOG_ERROR(Plugin, "symbol '{}' no found: {}", symbol, error);
    }

    return address;
}
void closeLibrary(void *library) {
    if (library == nullptr) {
        return;
    }

    ::dlerror();
    if (::dlclose(library) != 0) {
        const char *error = ::dlerror();
        UME_LOG_WARN(Plugin, "dlclose failed: {}",
                     error != nullptr ? error : "unknown error");
    }
}
} // namespace ume