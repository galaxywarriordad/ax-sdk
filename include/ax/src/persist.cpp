#include "ax/persist.hpp"

#include <cerrno>
#include <cstdio>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace ax::core {
namespace {

#if !defined(_WIN32)
bool FsyncAll(int fd, int& out_errno) noexcept {
    while (::fsync(fd) != 0) {
        if (errno == EINTR) {
            continue;
        }
        out_errno = errno;
        return false;
    }
    return true;
}

bool FsyncDirectory(const std::filesystem::path& dir, int& out_errno) noexcept {
    const std::string dir_str = dir.string();
    const int dir_fd = ::open(dir_str.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dir_fd < 0) {
        out_errno = errno;
        return false;
    }
    const bool synced = FsyncAll(dir_fd, out_errno);
    while (::close(dir_fd) != 0) {
        if (errno != EINTR) {
            break;
        }
    }
    return synced;
}

AtomicWriteResult WriteFileAtomicPosix(
    const std::filesystem::path& target,
    std::span<const std::byte> data,
    const AtomicWriteOptions& options) noexcept {
    AtomicWriteResult result{};
    std::error_code ec;

    const std::filesystem::path parent =
        target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();

    if (options.create_parent_dirs) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            result.os_error = ec.value();
            return result;
        }
    }

    const std::string filename = target.filename().string();
    if (filename.empty()) {
        result.os_error = EINVAL;
        return result;
    }

    std::string tmpl = (parent / (".ax_tmp_" + filename + "_XXXXXX")).string();
    std::vector<char> tmpl_buf(tmpl.begin(), tmpl.end());
    tmpl_buf.push_back('\0');

    const int fd = ::mkstemp(tmpl_buf.data());
    if (fd < 0) {
        result.os_error = errno;
        return result;
    }

    const std::string tmp_path(tmpl_buf.data());
    auto unlink_tmp = [&]() noexcept {
        ::unlink(tmp_path.c_str());
    };
    auto close_and_unlink = [&]() noexcept {
        while (::close(fd) != 0 && errno == EINTR) {
        }
        unlink_tmp();
    };

    const mode_t mode = static_cast<mode_t>(options.file_mode & 07777u);
    if (::fchmod(fd, mode) != 0) {
        result.os_error = errno;
        close_and_unlink();
        return result;
    }

    const auto* ptr = reinterpret_cast<const unsigned char*>(data.data());
    std::size_t remaining = data.size();
    std::size_t total_written = 0;

    while (remaining > 0) {
        const ssize_t n = ::write(fd, ptr, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            result.os_error = errno;
            close_and_unlink();
            return result;
        }
        const auto written_chunk = static_cast<std::size_t>(n);
        ptr += written_chunk;
        remaining -= written_chunk;
        total_written += written_chunk;
    }

    int sync_err = 0;
    if (!FsyncAll(fd, sync_err)) {
        result.os_error = sync_err;
        close_and_unlink();
        return result;
    }

    while (::close(fd) != 0) {
        if (errno != EINTR) {
            result.os_error = errno;
            unlink_tmp();
            return result;
        }
    }

    const std::string target_str = target.string();
    if (::rename(tmp_path.c_str(), target_str.c_str()) != 0) {
        result.os_error = errno;
        unlink_tmp();
        return result;
    }

    if (options.fsync_parent_dir) {
        int dir_err = 0;
        if (!FsyncDirectory(parent, dir_err)) {
            result.os_error = dir_err;
            return result;
        }
    }

    result.ok = true;
    result.bytes_written = total_written;
    return result;
}
#else
AtomicWriteResult WriteFileAtomicWin32(
    const std::filesystem::path& target,
    std::span<const std::byte> data,
    const AtomicWriteOptions& options) noexcept {
    AtomicWriteResult result{};
    std::error_code ec;

    const std::filesystem::path parent =
        target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();

    if (options.create_parent_dirs) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            result.os_error = ec.value();
            return result;
        }
    }

    const std::wstring parent_w = parent.wstring();
    const std::wstring target_w = target.wstring();
    wchar_t temp_name[MAX_PATH]{};

    if (::GetTempFileNameW(parent_w.c_str(), L"AXT", 0, temp_name) == 0) {
        result.os_error = static_cast<int>(::GetLastError());
        return result;
    }

    const HANDLE hFile = ::CreateFileW(
        temp_name,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        result.os_error = static_cast<int>(::GetLastError());
        ::DeleteFileW(temp_name);
        return result;
    }

    const auto* ptr = reinterpret_cast<const unsigned char*>(data.data());
    std::size_t remaining = data.size();
    std::size_t total_written = 0;

    while (remaining > 0) {
        const DWORD chunk = static_cast<DWORD>(
            remaining > static_cast<std::size_t>(0x7fffffffUL) ? 0x7fffffffUL : remaining);
        DWORD written = 0;
        if (!::WriteFile(hFile, ptr, chunk, &written, nullptr)) {
            result.os_error = static_cast<int>(::GetLastError());
            ::CloseHandle(hFile);
            ::DeleteFileW(temp_name);
            return result;
        }
        ptr += written;
        remaining -= written;
        total_written += written;
    }

    if (!::FlushFileBuffers(hFile)) {
        result.os_error = static_cast<int>(::GetLastError());
        ::CloseHandle(hFile);
        ::DeleteFileW(temp_name);
        return result;
    }

    ::CloseHandle(hFile);

    if (!::MoveFileExW(
            temp_name,
            target_w.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        result.os_error = static_cast<int>(::GetLastError());
        ::DeleteFileW(temp_name);
        return result;
    }

    result.ok = true;
    result.bytes_written = total_written;
    return result;
}
#endif

} // namespace

AtomicWriteResult WriteFileAtomic(
    const std::filesystem::path& target,
    std::span<const std::byte> data,
    const AtomicWriteOptions& options) noexcept {
#if defined(_WIN32)
    return WriteFileAtomicWin32(target, data, options);
#else
    return WriteFileAtomicPosix(target, data, options);
#endif
}

AtomicWriteResult WriteFileAtomic(
    const std::filesystem::path& target,
    std::string_view data,
    const AtomicWriteOptions& options) noexcept {
    const auto* bytes = reinterpret_cast<const std::byte*>(data.data());
    return WriteFileAtomic(target, std::span<const std::byte>(bytes, data.size()), options);
}

std::size_t CleanupStaleTempFiles(
    const std::filesystem::path& directory,
    std::string_view prefix) noexcept {
    std::size_t removed = 0;
    std::error_code ec;

    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec)) {
        return 0;
    }

    for (std::filesystem::directory_iterator it(directory, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!it->is_regular_file(ec)) {
            continue;
        }
        const std::string name = it->path().filename().string();
        if (name.rfind(prefix, 0) == 0) {
            if (std::filesystem::remove(it->path(), ec) && !ec) {
                ++removed;
            }
            ec.clear();
        }
    }

    return removed;
}

} // namespace ax::core
