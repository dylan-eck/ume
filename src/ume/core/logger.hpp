#pragma once

#include <cstdlib>
#include <format>
#include <string_view>

#define UME_LOG_CATEGORY_LIST(X)                                               \
    X(Core, "core")                                                            \
    X(Platform, "platform")                                                    \
    X(Renderer, "renderer")                                                    \
    X(Script, "script")                                                        \
    X(Plugin, "plugin")

namespace ume::logger {

enum class Category : uint8_t {
#define UME_LOG_CATEGORY_ENUMERATOR(name, str) name,
    UME_LOG_CATEGORY_LIST(UME_LOG_CATEGORY_ENUMERATOR)
#undef UME_LOG_CATEGORY_ENUMERATOR
        Count
};

enum class Level : uint8_t { Info, Warn, Error };

bool enabled(Category category, Level level) noexcept;
void logMessage(Category category, Level level,
                std::string_view message) noexcept;

template <typename... Args>
void log(Category category, Level level, std::format_string<Args...> fmt,
         Args &&...args) {
    if (!enabled(category, level)) {
        return;
    }

    try {
        logMessage(category, level,
                   std::format(fmt, std::forward<Args>(args)...));
    } catch (...) {
        logMessage(category, level, "<failed to format log message>");
    }
}

} // namespace ume::logger

#define UME_LOG_WARN(category, ...)                                            \
    ::ume::logger::log(::ume::logger::Category::category,                      \
                       ::ume::logger::Level::Warn, __VA_ARGS__)
#define UME_LOG_ERROR(category, ...)                                           \
    ::ume::logger::log(::ume::logger::Category::category,                      \
                       ::ume::logger::Level::Error, __VA_ARGS__)
#ifdef UME_ENABLE_LOGGING

#define UME_LOG_INFO(category, ...)                                            \
    ::ume::logger::log(::ume::logger::Category::category,                      \
                       ::ume::logger::Level::Info, __VA_ARGS__)
#else
#define UME_LOG_INFO(category, ...) (void)0
#endif