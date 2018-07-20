#include "file_watcher.h"

#include <cassert>

namespace {
static constexpr int BUFF_SIZE = (sizeof(struct inotify_event) + 4096) * 1024;
}

namespace cbase {

FileWatcher::FileWatcher() {
    m_inotify_fd = inotify_init();
    assert(m_inotify_fd > 0 && "inotify_init failed.");
    m_time_out.tv_sec  = 0;
    m_time_out.tv_usec = 1000;
    FD_ZERO(&m_fd_set);
}

FileWatcher::~FileWatcher() { m_watchings.clear(); }

int FileWatcher::WatchDir(const std::string& dir, const ActionFunc& func,
                          uint32_t mask) {
    int wd = inotify_add_watch(m_inotify_fd, dir.c_str(), mask);
    // IN_CLOSE_WRITE | IN_MOVED_TO is focused;
    if (wd < 0) return -1;

    auto file_meta           = std::make_shared<FileMeta>();
    file_meta->m_watch_id    = wd;
    file_meta->m_dir_name    = dir;
    file_meta->m_action_func = func;

    m_watchings.insert(std::make_pair(wd, file_meta));
    return wd;
}

int FileWatcher::RmDir(const std::string& dir) {
    for (auto it : m_watchings) {
        if (dir == it.second->m_dir_name) {
            int wd = it.second->m_watch_id;
            return RmWatch(wd) ? wd : -1;
        }
    }
    return 0;
}

bool FileWatcher::RmWatch(int watchid) {
    auto it = m_watchings.find(watchid);
    if (it == m_watchings.end()) return true;

    if (inotify_rm_watch(m_inotify_fd, watchid) != 0) {
        return false;
    }
    m_watchings.erase(it);
    return true;
}

void FileWatcher::Watching() {
    FD_ZERO(&m_fd_set);
    FD_SET(m_inotify_fd, &m_fd_set);

    m_time_out.tv_sec  = 0;
    m_time_out.tv_usec = 1000;

    if (select(m_inotify_fd + 1, &m_fd_set, NULL, NULL, &m_time_out) < 0) {
        return;
    }

    if (FD_ISSET(m_inotify_fd, &m_fd_set)) {
        char buf[BUFF_SIZE] = {0};
        ssize_t length      = 0;
        while ((length = read(m_inotify_fd, buf, sizeof(buf))) < 0 &&
               errno == EINTR) {
        }

        ssize_t index = 0;
        while (index < length) {
            struct inotify_event* event = (struct inotify_event*)(buf + index);
            index += sizeof(struct inotify_event) + event->len;
            HanldeEvent(event);
        }
    }
}

void FileWatcher::HanldeEvent(const struct inotify_event* event) {
    auto file_meta = m_watchings[event->wd];
    if (!file_meta) return;

    uint32_t mask = event->mask;
    if (IN_CLOSE_WRITE & mask) {
        file_meta->m_action_func(file_meta->m_dir_name, event->name,
                                 action::FILE_MOD);
    }

    if (IN_MOVED_TO & mask) {
        file_meta->m_action_func(file_meta->m_dir_name, event->name,
                                 action::FILE_ADD);
    }

    if (IN_CREATE & mask) {
        file_meta->m_action_func(file_meta->m_dir_name, event->name,
                                 action::FILE_ADD);
    }

    if (IN_MOVED_FROM & mask) {
        file_meta->m_action_func(file_meta->m_dir_name, event->name,
                                 action::FILE_DEL);
    }

    if (IN_DELETE & mask) {
        file_meta->m_action_func(file_meta->m_dir_name, event->name,
                                 action::FILE_DEL);
    }
}

}  // namespace cbase
