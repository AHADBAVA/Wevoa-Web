#include "utils/keyboard.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace wevoaweb {

#ifndef _WIN32
struct KeyboardInput::termiosState {
    termios value {};
};
#endif

KeyboardInput::KeyboardInput() {
#ifndef _WIN32
    originalState_ = new termiosState();
    if (isatty(STDIN_FILENO) == 0) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &originalState_->value) != 0) {
        return;
    }

    termios raw = originalState_->value;
    raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
        configured_ = true;
        hadAttributes_ = true;
    }
#endif
}

KeyboardInput::~KeyboardInput() {
#ifndef _WIN32
    if (configured_ && hadAttributes_ && originalState_ != nullptr) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalState_->value);
    }
    delete originalState_;
#endif
}

std::optional<char> KeyboardInput::pollKey() {
#ifdef _WIN32
    if (_kbhit() == 0) {
        return std::nullopt;
    }

    const int ch = _getch();
    if (ch == 0 || ch == 224) {
        if (_kbhit() != 0) {
            static_cast<void>(_getch());
        }
        return std::nullopt;
    }

    return static_cast<char>(ch);
#else
    if (!configured_) {
        return std::nullopt;
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(STDIN_FILENO, &readSet);

    timeval timeout {};
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    const int ready = select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready <= 0 || FD_ISSET(STDIN_FILENO, &readSet) == 0) {
        return std::nullopt;
    }

    char ch = '\0';
    const ssize_t readCount = read(STDIN_FILENO, &ch, 1);
    if (readCount != 1) {
        return std::nullopt;
    }

    return ch;
#endif
}

}  // namespace wevoaweb
