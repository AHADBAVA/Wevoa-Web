#pragma once

#include <optional>

namespace wevoaweb {

class KeyboardInput {
  public:
    KeyboardInput();
    ~KeyboardInput();

    KeyboardInput(const KeyboardInput&) = delete;
    KeyboardInput& operator=(const KeyboardInput&) = delete;

    std::optional<char> pollKey();

  private:
#ifndef _WIN32
    bool configured_ = false;
    bool hadAttributes_ = false;
    struct termiosState;
    termiosState* originalState_ = nullptr;
#endif
};

}  // namespace wevoaweb
