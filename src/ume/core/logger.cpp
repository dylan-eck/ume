#include "logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <array>
#include <memory>

namespace ume::logger {

namespace {
const char *categoryName(Category c) {
    switch (c) {
    case Category::Core:
        return "core";
    case Category::Renderer:
        return "renderer";
    case Category::Platform:
        return "platform";
    }
    return "unknown";
}
} // namespace

using LoggerArray = std::array<std::shared_ptr<spdlog::logger>, 3>;

LoggerArray &loggers() {
    static LoggerArray instance = [] {
        LoggerArray arr;
        for (std::size_t i = 0; i < arr.size(); ++i) {
            arr[i] =
                spdlog::stdout_color_mt(categoryName(static_cast<Category>(i)));
        }
        return arr;
    }();
    return instance;
}

spdlog::logger &get(Category category) {
    return *loggers()[static_cast<std::size_t>(category)];
}

} // namespace ume::logger