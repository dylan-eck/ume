#pragma once

#include "ume/core/logger.hpp"

#include <stdexcept>

namespace ume {
// this error class is only to used for unrecoverable startup errors
// these errors will be caught exclusively in main()

class Error final : public std::runtime_error {
public:
    template <typename... Args>
    Error(logger::Category category, std::format_string<Args...> fmt,
          Args &&...args)
        : std::runtime_error(std::format(fmt, std::forward<Args>(args)...)),
          category_(category) {}

    [[nodiscard]] logger::Category category() const noexcept {
        return category_;
    }

private:
    logger::Category category_;
};
} // namespace ume