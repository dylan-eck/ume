#pragma once

#include <cstdlib>
#include <spdlog/spdlog.h>

namespace spdlog {
class logger;
}

namespace ume::logger {
enum class Category : uint8_t { Core, Renderer, Platform, Count };

spdlog::logger &get(Category category);

} // namespace ume::logger

#ifdef UME_ENABLE_LOGGING
#define UME_LOG_INFO(category, ...)                                            \
    ::ume::logger::get(::ume::logger::Category::category).info(__VA_ARGS__)
#define UME_LOG_WARN(category, ...)                                            \
    ::ume::logger::get(::ume::logger::Category::category).warn(__VA_ARGS__)
#define UME_LOG_ERROR(category, ...)                                           \
    ::ume::logger::get(::ume::logger::Category::category).error(__VA_ARGS__)
#else
#define UME_LOG_INFO(category, ...) (void)0
#define UME_LOG_WARN(category, ...) (void)0
#define UME_LOG_ERROR(category, ...) (void)0
#endif