#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>

namespace cbase {

class rw_spin_lock {
public:
    rw_spin_lock() : m_flag(0) {}
    ~rw_spin_lock() {}

    rw_spin_lock(const rw_spin_lock& lock) : m_flag(lock.m_flag.load()) {}
    rw_spin_lock& operator=(const rw_spin_lock&) = delete;

    void shared_lock() {
        uint32_t flag = 0;
        do {
            flag = m_flag.load(std::memory_order_acquire) & 0x0000FFFF;
        } while (!m_flag.compare_exchange_weak(flag, flag + 1,
                                               std::memory_order_release,
                                               std::memory_order_relaxed));
    }

    void shared_unlock() { m_flag.fetch_sub(1, std::memory_order_release); }

    void lock() {
        uint32_t flag = 0;
        while (!m_flag.compare_exchange_weak(flag, 0x00010000,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)) {
            flag = 0;
        }
    }
    void unlock() { m_flag.store(0, std::memory_order_release); }

private:
    std::atomic<uint32_t> m_flag;
};

template <class Lock>
class scoped_share_guard {
public:
    explicit scoped_share_guard(Lock& spin_lock)  // NOLINT
        : m_spin_lock(spin_lock) {
        m_spin_lock.shared_lock();
    }

    ~scoped_share_guard() { m_spin_lock.shared_unlock(); }

    scoped_share_guard(const scoped_share_guard&) = delete;
    scoped_share_guard& operator=(const scoped_share_guard&) = delete;

private:
    Lock& m_spin_lock;
};

template <class Lock>
class scoped_exclusive_guard {
public:
    explicit scoped_exclusive_guard(Lock& spin_lock)  // NOLINT
        : m_spin_lock(spin_lock) {
        m_spin_lock.lock();
    }

    ~scoped_exclusive_guard() { m_spin_lock.unlock(); }

    scoped_exclusive_guard(const scoped_exclusive_guard&) = delete;
    scoped_exclusive_guard& operator=(const scoped_exclusive_guard&) = delete;

private:
    Lock& m_spin_lock;
};

template <class T>
class buffering_ptr {
    static_assert(std::is_default_constructible<T>::value,
                  "T is not default constructitable");

public:
    buffering_ptr()
        : m_read_index(0),
          m_buffering_handlers({std::make_shared<T>(), nullptr}) {}
    ~buffering_ptr() {}

    buffering_ptr(const buffering_ptr&) = delete;
    buffering_ptr& operator=(const buffering_ptr&) = delete;

    size_t reader() const noexcept {
        return static_cast<size_t>(
            m_read_index.load(std::memory_order_acquire));
    }

    template <class... Args>
    void update(Args&&... args) {
        uint8_t read_index  = m_read_index.load(std::memory_order_acquire);
        uint8_t write_index = read_index ^ 1;
        std::shared_ptr<T> handler(
            std::make_shared<T>(std::forward<Args>(args)...));

        scoped_exclusive_guard<rw_spin_lock> lock(m_spin_locks[write_index]);
        m_buffering_handlers[write_index] = std::move(handler);
        m_read_index.store(write_index, std::memory_order_release);
    }

    std::shared_ptr<T> get() const noexcept {
        uint8_t read_index = m_read_index.load(std::memory_order_acquire);

        scoped_share_guard<rw_spin_lock> lock(m_spin_locks[read_index]);
        return m_buffering_handlers[read_index];
    }

    // TODO(adaiboy): it's not thread safe, only used when init.
    T* operator->() noexcept {
        uint8_t read_index = m_read_index.load(std::memory_order_acquire);
        return m_buffering_handlers[read_index].get();
    }

private:
    std::atomic<uint8_t> m_read_index;
    mutable std::array<rw_spin_lock, 2> m_spin_locks{};
    std::array<std::shared_ptr<T>, 2> m_buffering_handlers;
};

}  // namespace cbase
