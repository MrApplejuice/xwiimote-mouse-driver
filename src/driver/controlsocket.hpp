/*
This file is part of xwiimote-mouse-driver.

xwiimote-mouse-driver is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

xwiimote-mouse-driver is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
xwiimote-mouse-driver. If not, see <https://www.gnu.org/licenses/>. 
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <exception>

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

class SocketFailed : public std::exception {};
class SocketCreationFailed : public SocketFailed {
private:
    std::string error;
public:
    SocketCreationFailed(const std::string& error) : error(error) {}
    SocketCreationFailed(const SocketCreationFailed& other) = default;
    
    const char* what() const noexcept override {
        return error.c_str();
    }
};

class ConnectionHandler;

static const std::string DEFAULT_SOCKET_ADDR = "./wiimote-mouse.sock";

struct Command {
    std::string name;
    std::vector<std::string> parameters;
    ConnectionHandler* handler;
};

typedef std::function<std::string(const std::string& command, const std::vector<std::string>& parameters)> CommandHandleFunction;
typedef std::function<void(const Command& command)> PushCommandFunction;

class ConnectionHandler {
private:
    bool alive;
    sockpp::unix_socket socket;
    PushCommandFunction pushCommand;
    std::thread thread;

    void threadMain();
public:
    bool isAlive() const;

    void sendMessage(const std::string& msg);
    void startShutdown();

    ConnectionHandler(sockpp::unix_socket& _socket, PushCommandFunction _pushCommand);
    ~ConnectionHandler();
};

class ControlSocket {
private:
    std::string socketAddr;

    bool alive;
    std::mutex sharedResourceMutex;
    std::thread mainThread;

    std::shared_ptr<sockpp::unix_acceptor> acceptor;
    std::vector<ConnectionHandler*> handlerThreads;

    std::vector<Command> commands;

    void threadMain();
public:
    void processEvents(CommandHandleFunction handler);
    void broadcastMessage(const std::string& msg);

    ControlSocket(std::string socketAddr);
    ~ControlSocket();
};
