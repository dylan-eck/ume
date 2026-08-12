#include "logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <array>
#include <memory>

namespace ume::logger {

namespace {
constexpr std::array<std::string_view, static_cast<size_t>(Category::Count)>
    kCategoryIdentifiers{
#define UME_LOG_CATEGORY_IDENTIFIER(name, str) std::string_view{str},
        UME_LOG_CATEGORY_LIST(UME_LOG_CATEGORY_IDENTIFIER)
#undef UME_LOG_CATEGORY_IDENTIFIER
    };

spdlog::level::level_enum toSpdlogLevel(Level level) {
    switch (level) {
    case Level::Warn:
        return spdlog::level::warn;
    case Level::Error:
        return spdlog::level::err;
    default:
        return spdlog::level::info;
    }
}

using LoggerArray = std::array<std::shared_ptr<spdlog::logger>,
                               static_cast<size_t>(Category::Count)>;

LoggerArray &loggers() {
    static LoggerArray instance = [] {
        LoggerArray arr;
        for (std::size_t i = 0; i < static_cast<size_t>(Category::Count); ++i) {
            arr[i] =
                spdlog::stdout_color_mt(std::string(kCategoryIdentifiers[i]));
        }
        return arr;
    }();
    return instance;
}

spdlog::logger &get(Category category) {
    return *loggers()[static_cast<std::size_t>(category)];
}

} // namespace

bool enabled(Category category, Level level) noexcept {
    try {
        return get(category).should_log(toSpdlogLevel(level));
    } catch (...) {
        return false;
    }
}

void logMessage(Category category, Level level,
                std::string_view message) noexcept {
    try {
        get(category).log(
            toSpdlogLevel(level),
            spdlog::string_view_t{message.data(), message.size()});
    } catch (...) {
        fputs("exception thrown in logger\n", stderr);
    }
}

} // namespace ume::logger