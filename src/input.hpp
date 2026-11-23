#include "ax/input.hpp"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#ifndef BTN_TRIGGER_HAPPY5
#define BTN_TRIGGER_HAPPY5 0x2c4
#endif

namespace ax::input {

EvdevNavReader::~EvdevNavReader() {
    CloseAll();
}

void EvdevNavReader::OpenAll(std::size_t max_devices) {
    CloseAll();
#ifdef __linux__
    DIR* d = ::opendir("/dev/input");
    if (!d) return;

    struct dirent* ent = nullptr;
    while ((ent = ::readdir(d)) != nullptr && fds_.size() < max_devices) {
        if (std::strncmp(ent->d_name, "event", 5) != 0) continue;
        char path[512];
        std::snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
        int fd = ::open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd >= 0) {
            fds_.push_back(fd);
        }
    }
    ::closedir(d);
#else
    (void)max_devices;
#endif
}

void EvdevNavReader::CloseAll() {
#ifdef __linux__
    for (int fd : fds_) {
        if (fd >= 0) ::close(fd);
    }
#endif
    fds_.clear();
}

NavAction EvdevNavReader::PollNonBlocking() {
#ifdef __linux__
    NavAction result = NavAction::None;
    for (int fd : fds_) {
        if (fd < 0) continue;
        struct input_event ev {};
        while (::read(fd, &ev, sizeof(ev)) == static_cast<ssize_t>(sizeof(ev))) {
            if (result != NavAction::None) continue;
            if (ev.type == EV_KEY && ev.value == 1) {
                switch (ev.code) {
                    case BTN_DPAD_UP:
                    case KEY_UP:
                        result = NavAction::Up;
                        break;
                    case BTN_DPAD_DOWN:
                    case KEY_DOWN:
                        result = NavAction::Down;
                        break;
                    case BTN_DPAD_LEFT:
                    case KEY_LEFT:
                        result = NavAction::Left;
                        break;
                    case BTN_DPAD_RIGHT:
                    case KEY_RIGHT:
                        result = NavAction::Right;
                        break;
                    case BTN_SOUTH:
                    case BTN_EAST:
                    case BTN_START:
                        result = NavAction::Confirm;
                        break;
                    case BTN_WEST:
                    case BTN_SELECT:
                        result = NavAction::Back;
                        break;
                    case BTN_MODE:
                    case BTN_TRIGGER_HAPPY5:
                        result = NavAction::Menu;
                        break;
                    default:
                        break;
                }
            } else if (ev.type == EV_ABS) {
                if (ev.code == ABS_HAT0Y) {
                    if (ev.value < 0) result = NavAction::Up;
                    else if (ev.value > 0) result = NavAction::Down;
                } else if (ev.code == ABS_HAT0X) {
                    if (ev.value < 0) result = NavAction::Left;
                    else if (ev.value > 0) result = NavAction::Right;
                }
            }
        }
    }
    return result;
#else
    return NavAction::None;
#endif
}

} // namespace ax::input
