#pragma once

#include <cstdint>

namespace cbase {

class TimeElapser {
public:
    TimeElapser();
    ~TimeElapser() {}
    TimeElapser(const TimeElapser&) = delete;
    TimeElapser& operator=(const TimeElapser&) = delete;

    uint64_t ElapseMicros() const noexcept;

private:
    static uint64_t s_frequency;
    uint64_t m_start_tp{0};
};  // class TimeElapser

}  // namespace cbase
