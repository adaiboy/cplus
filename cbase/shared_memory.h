#pragma once

#include <sys/stat.h>
#include <cstdlib>
#include <string>

namespace cbase {

class SharedMemory {
public:
    SharedMemory(const std::string& name, size_t mem_size);
    ~SharedMemory();

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    void* Open();
    void* GetAddress() const noexcept;
    size_t GetSize() const noexcept;

private:
    int32_t m_fd;
    std::string m_name;
    size_t m_mem_size;
    void* m_addr;
    struct stat m_stat;
};

}  // namespace cbase
