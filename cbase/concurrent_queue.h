#pragma once

#include <condition_variable>  // NOLINT
#include <mutex>               // NOLINT
#include <queue>
#include <utility>

namespace cbase {

template <class Data>
class concurrent_queue {
public:
    concurrent_queue() {}
    ~concurrent_queue() {}

    concurrent_queue(const concurrent_queue&) = delete;
    concurrent_queue& operator=(const concurrent_queue&) = delete;

    void push(const Data& data) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(data);
        }
        m_condition_variable.notify_one();
    }

    void push(Data&& data) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(data));
        }
        m_condition_variable.notify_one();
    }

    void pop(Data& data) {  // NOLINT
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition_variable.wait(lock,
                                  [this] { return !this->m_queue.empty(); });
        data = std::move(m_queue.front());
        m_queue.pop();
    }

    bool pop(Data& data, int timeout_ms) {  // NOLINT
        std::unique_lock<std::mutex> lock(m_mutex);
        bool ret = m_condition_variable.wait_for(
            lock, std::chrono::milliseconds(timeout_ms),
            [this] { return !this->m_queue.empty(); });

        if (ret) {
            data = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

        return false;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

protected:
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition_variable;
    std::queue<Data> m_queue;
};
}  // namespace cbase
