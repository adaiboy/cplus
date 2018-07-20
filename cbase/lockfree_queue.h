#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace cbase {

template <class Data, std::size_t N = 10000>
class lockfree_queue {
public:
    lockfree_queue() : m_max_cnt(N), m_head(0), m_tail(0) {}
    ~lockfree_queue() {}

    lockfree_queue(const lockfree_queue&) = delete;
    lockfree_queue& operator=(const lockfree_queue&) = delete;

    int push(const Data& data);
    int pop(Data& data);  // NOLINT

    size_t size() const noexcept {
        return used_cnt(m_head.load(std::memory_order_relaxed),
                        m_tail.load(std::memory_order_relaxed));
    }

private:
    uint64_t get_idx(uint64_t idx) const noexcept { return idx % m_max_cnt; }

    uint64_t used_cnt(uint64_t head, uint64_t tail) const noexcept {
        assert(head <= tail && "head is larger than tail.");
        return tail - head;
    }

private:
    struct Block {
        std::atomic<int> write_fin = {0};
        Data data;

        bool Readable() const noexcept {
            return write_fin.load(std::memory_order_acquire) == 1;
        }

        void SetData(const Data& data) noexcept {
            this->data = data;
            write_fin.store(1, std::memory_order_release);
        }

        void GetData(Data& data) noexcept {  // NOLINT
            write_fin.store(0, std::memory_order_release);
            data = this->data;
        }
    };

    const uint64_t m_max_cnt;
    std::atomic<uint64_t> m_head;
    std::atomic<uint64_t> m_tail;
    std::array<Block, N> m_blocks;
};

template <class Data, std::size_t N>
int lockfree_queue<Data, N>::push(const Data& data) {
    uint64_t tail = 0;
    do {
        tail          = m_tail.load(std::memory_order_relaxed);
        uint64_t head = m_head.load(std::memory_order_relaxed);
        uint64_t cnt  = used_cnt(head, tail + 1);
        if (cnt + 1 > m_max_cnt) return -1;
    } while (!m_tail.compare_exchange_weak(
        tail, tail + 1, std::memory_order_release, std::memory_order_relaxed));

    // tail-1 对应的idx就是用来放data
    tail = get_idx(tail - 1);
    m_blocks[tail].SetData(data);
    return 0;
}

template <class Data, std::size_t N>
int lockfree_queue<Data, N>::pop(Data& data) {  // NOLINT
    uint64_t head = 0;
    do {
        head          = m_head.load(std::memory_order_relaxed);
        uint64_t tail = m_tail.load(std::memory_order_relaxed);
        uint64_t cnt  = used_cnt(head, tail);
        if (cnt == 0) return -1;
    } while (!m_head.compare_exchange_weak(
        head, head + 1, std::memory_order_release, std::memory_order_relaxed));

    head = get_idx(head - 1);
    while (!m_blocks[head].Readable()) {
    }
    m_blocks[head].GetData(data);
    return 0;
}

}  // namespace cbase
