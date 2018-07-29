#pragma once

#include <functional>
#include <string>
#include <unordered_map>

template <class T>
class Registry {
public:
    using Function = std::function<T*()>;
    static T* Create(const std::string& name) {
        if (factorys().count(name) == 0) return nullptr;
        return factorys()[name]();
    }

    static bool Register(const std::string& name, const Function& function) {
        factorys()[name] = function;
        return true;
    }

private:
    static std::unordered_map<std::string, Function>& factorys() {
        static std::unordered_map<std::string, Function> dict;
        return dict;
    }
};

#define REGISTER_SUBCLASS(Base, Derived) \
    static bool Derived##result =        \
        Registry<Base>::Register(#Derived, []() { return new Derived(); })
