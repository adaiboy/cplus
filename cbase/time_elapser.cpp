#include "time_elapser.h"

#include <cassert>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include "string_util.h"

namespace cbase {

static inline uint64_t rdtscp() {
    uint64_t rax, rdx;
    __asm__ volatile("rdtscp"
                     : "=a"(rax), "=d"(rdx)
                     : /* no input */
                     : "rcx");
    return (rdx << 32) | (rax & 0xffffffff);
}

static uint64_t frequency() {
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen("/proc/cpuinfo", "rb"),
                                                &fclose);
    if (fp == nullptr) return 0;

    char buf[1024];
    while (fgets(buf, sizeof(buf), fp.get()) != NULL) {
        std::vector<std::string> items = Tokenize(buf, ':');
        if (items.size() != 2) continue;
        if (items[0].compare(0, 7, "cpu MHz") == 0) {
            return static_cast<uint64_t>(std::stod(items[1]) * 1000000.0);
        }
    }
    return 0;
}

uint64_t TimeElapser::s_frequency = frequency();

TimeElapser::TimeElapser() : m_start_tp(rdtscp()) {}

uint64_t TimeElapser::ElapseMicros() const noexcept {
    assert(s_frequency > 0 && "s_frequency is zero.");
    uint64_t current_tp = rdtscp();
    uint64_t ret        = current_tp - m_start_tp;
    return ret * 1000000 / s_frequency;
}

}  // namespace cbase
