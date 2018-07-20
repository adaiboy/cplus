#include "stack_trace.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <sstream>

namespace cbase {

std::string StackTrace(uint32_t max_frames) {
    void* addrlist[max_frames + 1];
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
    if (addrlen == 0) {
        return "no stack get.";
    }

    std::stringstream stream;

    char** symbollist    = backtrace_symbols(addrlist, addrlen);
    size_t funcname_size = 256;
    char* funcname       = (char*)malloc(funcname_size);  // NOLINT

    for (int i = 1; i < addrlen; ++i) {
        char* begin_name   = 0;
        char* begin_offset = 0;
        char* end_offset   = 0;

        for (char* p = symbollist[i]; *p; ++p) {
            if (*p == '(') {
                begin_name = p;
            } else if (*p == '+') {
                begin_offset = p;
            } else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset &&
            begin_name < begin_offset) {
            *begin_name++   = '\0';
            *begin_offset++ = '\0';
            *end_offset++   = '\0';

            int status = 0;
            char* ret  = abi::__cxa_demangle(begin_name, funcname,
                                            &funcname_size, &status);
            if (status == 0) {
                funcname = ret;
                stream << symbollist[i] << " : " << funcname << "+"
                       << begin_offset << "\n";
            } else {
                stream << symbollist[i] << " : " << begin_name << "()+"
                       << begin_offset << "\n";
            }
        } else {
            stream << symbollist[i] << "\n";
        }
    }

    free(funcname);
    free(symbollist);

    return stream.str();
}

}  // namespace cbase
