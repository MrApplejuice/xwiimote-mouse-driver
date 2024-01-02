#pragma once

#include <string>
#include <vector>

typedef void (*CommandHandleFunction)(const std::string& command, const std::vector<std::string>& parameters);

class ControlSocket {
public:
    void processEvents(CommandHandleFunction handler);
};
