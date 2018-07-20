#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace cbase {

class Procedure {
public:
    explicit Procedure(const std::function<bool()>& action_func)
        : m_commited(false),
          m_action_func(action_func),
          m_error_func(nullptr) {}

    ~Procedure() {
        // TODO(adaiboy): Warning here, as m_error_func may raise exception
        // and this will make process terminate
        if (!m_commited && m_error_func != nullptr) {
            m_error_func();
        }
    }

    Procedure(const Procedure& other)
        : m_commited(other.m_commited),
          m_action_func(other.m_action_func),
          m_error_func(other.m_error_func) {}

    Procedure& operator=(const Procedure& other) {
        if (this == &other) return *this;
        m_commited    = other.m_commited;
        m_action_func = other.m_action_func;
        m_error_func  = other.m_error_func;
        return *this;
    }

    Procedure(Procedure&& other)
        : m_commited(other.m_commited),
          m_action_func(std::move(other.m_action_func)),
          m_error_func(std::move(other.m_error_func)) {
        other.m_commited = true;
    }

    Procedure& operator=(Procedure&& other) {
        m_commited       = other.m_commited;
        other.m_commited = true;
        m_action_func    = std::move(other.m_action_func);
        m_error_func     = std::move(other.m_error_func);
        return *this;
    }

    // TODO(adaiboy): as we cannot assume that obj's life time
    // is longer than procedure
    template <typename Obj, typename MemFun, typename... Args>
    void AddErrorFunc(Obj* obj, MemFun memfun, Args&&... args) {
        m_error_func = std::bind(memfun, obj, std::forward<Args>(args)...);
    }

    bool Invoke() { return m_action_func(); }
    void Commit() noexcept { m_commited = true; }

private:
    bool m_commited;
    std::function<bool()> m_action_func;
    std::function<void()> m_error_func;
};  // class Procedure

template <typename T>
class Transaction {
public:
    Transaction() {}
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    static void* operator new(std::size_t count)   = delete;
    static void* operator new[](std::size_t count) = delete;
    static void operator delete(void* ptr)         = delete;
    static void operator delete[](void* ptr)       = delete;

    bool InvokeCurrentProcedures();

    // TODO(adaiboy): warning we cannot assume obj's lifetime
    // is longer than transaction
    template <class Obj, typename MemFun, typename... Args>
    Procedure& AddProcedure(Obj* obj, MemFun memfun, Args&&... args);

    std::string GetErrMsg() const noexcept { return m_err_msg; }

private:
    std::list<Procedure> m_procedures;
    std::string m_err_msg;
};  // class Transaction

template <typename T>
Transaction<T>::~Transaction() {
    while (!m_procedures.empty()) {
        m_procedures.pop_back();
    }
}

template <typename T>
template <class Obj, typename MemFun, typename... Args>
Procedure& Transaction<T>::AddProcedure(Obj* obj, MemFun memfun,
                                        Args&&... args) {
    // (obj.*memfun)(std::forward<Args>(args)...);
    // decltype((std::declval<Obj>().*memfun)(std::forward<Args>(args)...)) a;
    static_assert(std::is_same<decltype((std::declval<Obj>().*
                                         memfun)(std::forward<Args>(args)...)),
                               bool>::value,
                  "function has no bool return type");
    static_assert(std::is_same<T, typename std::remove_cv<Obj>::type>::value,
                  "T and Obj should be same type");

    std::function<bool()> action_func =
        std::bind(memfun, obj, std::forward<Args>(args)...);
    Procedure procedure(action_func);
    // move procedure into list
    m_procedures.emplace_back(std::move(procedure));
    return m_procedures.back();
}

template <typename T>
bool Transaction<T>::InvokeCurrentProcedures() {
    for (auto& procedure : m_procedures) {
        if (!procedure.Invoke()) {
            return false;
        }
    }

    for (auto& procedure : m_procedures) {
        procedure.Commit();
    }
    return true;
}

}  // namespace cbase
