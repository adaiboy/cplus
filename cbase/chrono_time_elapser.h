#pragma once

#include <chrono>  // NOLINT
#include <cstdint>
#include <string>

namespace cbase {

class ChronoTimeElapser final {
public:
    ChronoTimeElapser() : m_t_start(Clock::now()) {}
    ~ChronoTimeElapser() {}

    ChronoTimeElapser(const ChronoTimeElapser&) = delete;
    ChronoTimeElapser& operator=(const ChronoTimeElapser&) = delete;

    uint64_t ElapsedTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   Clock::now() - m_t_start)
            .count();
    }

private:
    using Clock = std::chrono::steady_clock;
    const Clock::time_point m_t_start;
};
}  // namespace cbase
