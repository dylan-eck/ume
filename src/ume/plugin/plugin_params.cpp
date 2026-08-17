#include "plugin_params.hpp"

namespace ume {
namespace {
double numberLookup(const void *impl, const char *key,
                    double fallback) noexcept {
    if (impl == nullptr || key == nullptr) {
        return fallback;
    }

    const auto *values = static_cast<const NumberMap *>(impl);

    try {
        auto it = values->find(key);
        return it != values->end() ? it->second : fallback;
    } catch (...) {
        return fallback;
    }
}
} // namespace

UmeParams makeNumberParams(const NumberMap &values) {
    return UmeParams{
        .struct_size = sizeof(UmeParams),
        .impl = &values,
        .number = &numberLookup,
    };
}
} // namespace ume