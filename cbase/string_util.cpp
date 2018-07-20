#include "string_util.h"

namespace cbase {

std::vector<std::string> Tokenize(const std::string& s, char c) {
    auto end   = s.cend();
    auto start = end;

    std::vector<std::string> v;
    for (auto it = s.cbegin(); it != end; ++it) {
        if (*it != c) {
            if (start == end) start = it;
            continue;
        }
        if (start != end) {
            v.emplace_back(start, it);
            start = end;
        }
    }
    if (start != end) v.emplace_back(start, end);
    return v;
}

}  // namespace cbase
