#include "watcher/file_watcher.h"

#include <thread>

namespace wevoaweb {

FileWatcher::FileWatcher(std::vector<std::filesystem::path> directories,
                         std::chrono::milliseconds interval,
                         Callback callback)
    : directories_(std::move(directories)), interval_(interval), callback_(std::move(callback)) {}

FileWatcher::~FileWatcher() {
    stop();
}

void FileWatcher::start() {
    if (running_.exchange(true)) {
        return;
    }

    snapshot_ = captureSnapshot();
    worker_ = std::thread([this]() { run(); });
}

void FileWatcher::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    if (worker_.joinable()) {
        worker_.join();
    }
}

void FileWatcher::run() {
    while (running_) {
        std::this_thread::sleep_for(interval_);
        if (!running_) {
            break;
        }

        publishChanges(captureSnapshot());
    }
}

std::unordered_map<std::string, FileWatcher::SnapshotEntry> FileWatcher::captureSnapshot() const {
    namespace fs = std::filesystem;

    std::unordered_map<std::string, SnapshotEntry> nextSnapshot;

    for (const auto& directory : directories_) {
        std::error_code error;
        if (!fs::exists(directory, error) || !fs::is_directory(directory, error)) {
            continue;
        }

        for (fs::recursive_directory_iterator iterator(directory, fs::directory_options::skip_permission_denied, error),
             end;
             iterator != end;
             iterator.increment(error)) {
            if (error) {
                break;
            }

            if (!iterator->is_regular_file(error)) {
                continue;
            }

            const fs::path path = iterator->path();
            const auto absolute = fs::absolute(path, error);
            if (error) {
                continue;
            }

            const auto writeTime = fs::last_write_time(path, error);
            if (error) {
                continue;
            }

            const auto size = fs::file_size(path, error);
            if (error) {
                continue;
            }

            nextSnapshot[absolute.string()] = SnapshotEntry {writeTime, size};
        }
    }

    return nextSnapshot;
}

void FileWatcher::publishChanges(const std::unordered_map<std::string, SnapshotEntry>& nextSnapshot) {
    namespace fs = std::filesystem;

    for (const auto& [path, entry] : nextSnapshot) {
        const auto found = snapshot_.find(path);
        if (found == snapshot_.end()) {
            callback_(FileChangeEvent {fs::path(path), FileChangeType::Added});
            continue;
        }

        if (found->second.writeTime != entry.writeTime || found->second.size != entry.size) {
            callback_(FileChangeEvent {fs::path(path), FileChangeType::Modified});
        }
    }

    for (const auto& [path, entry] : snapshot_) {
        static_cast<void>(entry);
        if (nextSnapshot.find(path) == nextSnapshot.end()) {
            callback_(FileChangeEvent {fs::path(path), FileChangeType::Removed});
        }
    }

    snapshot_ = nextSnapshot;
}

}  // namespace wevoaweb
