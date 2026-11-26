#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace ax::core {

struct AtomicWriteOptions {
    // POSIX file mode applied to the staged file prior to atomic rename.
    unsigned int file_mode = 0644;
    // Create missing parent directories when true.
    bool create_parent_dirs = true;
    // Fsync staged file content before rename when true.
    bool fsync_file = true;
    // Fsync the parent directory after rename (critical for journal-free ext4 /userdata).
    bool fsync_parent_dir = true;
};

struct AtomicWriteResult {
    bool ok = false;
    int os_error = 0;
    std::size_t bytes_written = 0;
};

// Atomically writes a byte span to `target` via same-directory staging,
// explicit content sync, atomic rename, and optional parent-directory sync.
[[nodiscard]] AtomicWriteResult WriteFileAtomic(
    const std::filesystem::path& target,
    std::span<const std::byte> data,
    const AtomicWriteOptions& options = {}) noexcept;

// Convenience overload for text or serialized byte strings.
[[nodiscard]] AtomicWriteResult WriteFileAtomic(
    const std::filesystem::path& target,
    std::string_view data,
    const AtomicWriteOptions& options = {}) noexcept;

// Removes orphaned temporary files matching `prefix` inside `directory`.
// Returns the number of removed temporary entries.
std::size_t CleanupStaleTempFiles(
    const std::filesystem::path& directory,
    std::string_view prefix = ".ax_tmp_") noexcept;

} // namespace ax::core
```[cite: 13]

Would you like to share `persist.cpp` next so we can verify it handles `fsync_file`, `fsync_parent_dir`, and `noexcept` error reporting?
