#pragma once

#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace cbase {

namespace action {
enum FileAction { FILE_ADD = 1, FILE_DEL = 2, FILE_MOD = 3 };
}

class FileWatcher {
public:
    using ActionFunc = std::function<int(const std::string&, const std::string&,
                                         action::FileAction)>;

    FileWatcher();
    ~FileWatcher();
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    bool IsValidWatcher() const noexcept { return m_inotify_fd > 0; }

    // TODO(adaiboy): currently, we donot support recursive and cookie
    // -1 means failed, return val > 0 means success and it is watchid
    int WatchDir(const std::string& dir, const ActionFunc& func, uint32_t mask);

    // return wd of dir, 0 if dir not exist, -1 if error
    int RmDir(const std::string& dir);
    bool RmWatch(int watchid);

    void Watching();

private:
    void HanldeEvent(const struct inotify_event* event);

private:
    struct FileMeta {
        int m_watch_id;
        std::string m_dir_name;
        ActionFunc m_action_func;
    };  // struct FileMeta

    int m_inotify_fd;
    std::unordered_map<int, std::shared_ptr<FileMeta>> m_watchings;

    struct timeval m_time_out;
    fd_set m_fd_set;
};  // class FileWatcher

}  // namespace cbase
