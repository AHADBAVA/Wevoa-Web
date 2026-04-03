#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace wevoaweb {

enum class FileChangeType {
    Added,
    Modified,
    Removed
};

struct FileChangeEvent {
    std::filesystem::path path;
    FileChangeType type = FileChangeType::Modified;
};

class FileWatcher {
  public:
    using Callback = std::function<void(const FileChangeEvent&)>;

    FileWatcher(std::vector<std::filesystem::path> directories,
                std::chrono::milliseconds interval,
                Callback callback);
    ~FileWatcher();

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    void start();
    void stop();

  private:
    struct SnapshotEntry {
        std::filesystem::file_time_type writeTime {};
        std::uintmax_t size = 0;
    };

    void run();
    std::unordered_map<std::string, SnapshotEntry> captureSnapshot() const;
    void publishChanges(const std::unordered_map<std::string, SnapshotEntry>& nextSnapshot);

    std::vector<std::filesystem::path> directories_;
    std::chrono::milliseconds interval_;
    Callback callback_;
    std::atomic<bool> running_ = false;
    std::thread worker_;
    std::unordered_map<std::string, SnapshotEntry> snapshot_;
};

}  // namespace wevoaweb
