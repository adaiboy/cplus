#pragma once

#include <cassert>
#include <chrono>  // NOLINT
#include <cstdint>
#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>
#include "shared_memory.h"
#include "utils.h"

namespace cbase {

template <class Data>
class SharedLockFreeQueue {
public:
    SharedLockFreeQueue(size_t max_cnt, const std::string& name)
        : m_max_cnt(max_cnt),
          m_name(name),
          m_shared_memory(nullptr),
          m_queue(nullptr),
          m_blocks(nullptr) {}
    ~SharedLockFreeQueue() {}

    bool Init();

    int AddData(const Data& data);
    int AddData(const std::vector<Data>& datas);
    int GetData(Data* data);

    uint64_t GetIdx(uint64_t idx) const noexcept { return idx % m_max_cnt; }

    uint64_t UsedCnt(uint64_t head, uint64_t tail) const noexcept {
        assert(head <= tail && "head is larger than tail.");
        return tail - head;
    }

private:
    struct Block {
        union {
            struct {
                uint8_t m_write_fin;
            };
            char m_reserved[8];
        };
        Data m_data;

        bool IsReadable() const noexcept {
            return __atomic_load_n(&m_write_fin, __ATOMIC_ACQUIRE) == 1;
        }
        void SetData(const Data& data) {
            m_data = data;
            __atomic_store_n(&m_write_fin, 1, __ATOMIC_RELEASE);
        }

        Data GetData() noexcept {
            __atomic_store_n(&m_write_fin, 0, __ATOMIC_RELEASE);
            return m_data;
        }
    };

    struct Queue {
        union {
            struct {
                uint64_t m_mem_size;
                uint64_t m_max_cnt;
                uint64_t m_head;
                uint64_t m_tail;
            };
            char m_reserved[32];
        };
        Block m_blocks[0];
    };

private:
    const size_t m_max_cnt;
    const std::string m_name;
    std::unique_ptr<SharedMemory> m_shared_memory;
    Queue* m_queue;
    Block* m_blocks;
};

template <class Data>
bool SharedLockFreeQueue<Data>::Init() {
    size_t total_size = sizeof(Queue) + sizeof(Block) * m_max_cnt;
    m_shared_memory.reset(new SharedMemory(m_name, total_size));
    m_queue  = reinterpret_cast<Queue*>(m_shared_memory->Open());
    m_blocks = m_queue->m_blocks;

    if (m_queue->m_mem_size == 0) {
        m_queue->m_mem_size = static_cast<uint64_t>(total_size);
        m_queue->m_max_cnt  = m_max_cnt;
        m_queue->m_head     = 0;
        m_queue->m_tail     = 0;
    }

    assert(m_queue->m_mem_size == static_cast<uint64_t>(total_size) &&
           "unexpect total size.");
    assert(m_queue->m_max_cnt == m_max_cnt && "unexpect max cnt.");
    return true;
}

template <class Data>
int SharedLockFreeQueue<Data>::AddData(const Data& data) {
    uint64_t new_tail = 0;
    do {
        // RELAXED is enough
        new_tail      = __atomic_load_n(&(m_queue->m_tail), __ATOMIC_RELAXED);
        uint64_t head = __atomic_load_n(&(m_queue->m_head), __ATOMIC_RELAXED);
        uint64_t used_cnt = UsedCnt(head, new_tail);
        if (RSUnLikely(used_cnt + 1 > m_queue->m_max_cnt)) return -1;
    } while (!__atomic_compare_exchange_n(&(m_queue->m_tail), &new_tail,
                                          new_tail + 1, true, __ATOMIC_RELEASE,
                                          __ATOMIC_RELAXED));
    new_tail     = GetIdx(new_tail);
    Block* block = &(m_blocks[new_tail]);
    block->SetData(data);
    return 0;
}

template <class Data>
int SharedLockFreeQueue<Data>::AddData(const std::vector<Data>& datas) {
    uint64_t cnt = static_cast<uint64_t>(datas.size());
    if (cnt == 0) return 0;
    uint64_t new_tail = 0;
    do {
        // RELAXED is enough
        new_tail      = __atomic_load_n(&(m_queue->m_tail), __ATOMIC_RELAXED);
        uint64_t head = __atomic_load_n(&(m_queue->m_head), __ATOMIC_RELAXED);
        uint64_t used_cnt = UsedCnt(head, new_tail);
        if (RSUnLikely(used_cnt + cnt > m_queue->m_max_cnt)) return -1;
    } while (!__atomic_compare_exchange_n(&(m_queue->m_tail), &new_tail,
                                          new_tail + cnt, true,
                                          __ATOMIC_RELEASE, __ATOMIC_RELAXED));
    // new_tail 对应的idx就是用来放data
    new_tail = GetIdx(new_tail);
    for (uint64_t i = 0; i < cnt; ++i) {
        Block* block = &(m_blocks[new_tail + i]);
        block->SetData(datas[i]);
    }
    return 0;
}

template <class Data>
int SharedLockFreeQueue<Data>::GetData(Data* data) {
    uint64_t new_head = 0;
    do {
        new_head      = __atomic_load_n(&(m_queue->m_head), __ATOMIC_RELAXED);
        uint64_t tail = __atomic_load_n(&(m_queue->m_tail), __ATOMIC_RELAXED);
        uint64_t used_cnt = UsedCnt(new_head, tail);
        if (used_cnt == 0) return -1;
    } while (!__atomic_compare_exchange_n(&(m_queue->m_head), &new_head,
                                          new_head + 1, true, __ATOMIC_RELEASE,
                                          __ATOMIC_RELAXED));
    // new_head 对应的idx就是可以返回的data
    new_head     = GetIdx(new_head);
    Block* block = &(m_blocks[new_head]);

    // TODO(adaiboy): when write process failed when store write fin
    // then read process will block forever, so try drop it
    int retry_cnt = 0;
    while (retry_cnt < 5) {
        if (!block->IsReadable()) {
            ++retry_cnt;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            break;
        }
    }
    if (retry_cnt >= 5) {
        block->GetData();
        return -2;
    } else {
        *data = block->GetData();
    }
    return 0;
}

}  // namespace cbase
