#pragma once

#include <cstdint>
#include <vector>

namespace ax::input {

enum class NavAction : uint8_t {
    None = 0,
    Up,
    Down,
    Left,
    Right,
    Confirm, // BTN_SOUTH (A) / BTN_EAST (B) / BTN_START
    Back,    // BTN_WEST / BTN_SELECT
    Menu     // BTN_MODE / BTN_TRIGGER_HAPPY5 (FN)
};

class EvdevNavReader {
public:
    EvdevNavReader() = default;
    ~EvdevNavReader();

    EvdevNavReader(const EvdevNavReader&) = delete;
    EvdevNavReader& operator=(const EvdevNavReader&) = delete;

    void OpenAll(std::size_t max_devices = 8);
    void CloseAll();

    const std::vector<int>& fds() const noexcept { return fds_; }

    // Drains pending evdev events across all open devices and returns the first mapped NavAction
    NavAction PollNonBlocking();

private:
    std::vector<int> fds_;
};

} // namespace ax::input
