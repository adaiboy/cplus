#include "shared_memory.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>

namespace cbase {

SharedMemory::SharedMemory(const std::string& name, size_t mem_size)
    : m_fd(-1), m_name(name), m_mem_size(mem_size), m_addr(nullptr) {}

SharedMemory::~SharedMemory() {}

void* SharedMemory::Open() {
    if (m_fd > 0 && m_addr != nullptr) return m_addr;

    m_fd = shm_open(m_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
    if (m_fd < 0) {
        m_fd     = shm_open(m_name.c_str(), O_RDWR, 0666);
        auto ret = fstat(m_fd, &m_stat);
        assert(ret == 0 && "get stat of file failed");
    } else {
        auto ret = ftruncate(m_fd, m_mem_size);
        assert(ret == 0 && "set size of shared memory failed.");
        ret = fstat(m_fd, &m_stat);
        assert(ret == 0 && "get stat of file failed");
    }
    assert(m_fd > 0 && "shm open on file failed.");
    assert(m_mem_size == static_cast<size_t>(m_stat.st_size) &&
           "shm size unexpected.");
    m_addr =
        mmap(nullptr, m_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    assert(m_addr != MAP_FAILED && "mmap fd failed.");
    return m_addr;
}

void* SharedMemory::GetAddress() const noexcept { return m_addr; }

size_t SharedMemory::GetSize() const noexcept {
    return m_fd < 0 ? 0 : m_mem_size;
}

}  // namespace cbase
