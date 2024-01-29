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

#include "controlsocket.hpp"

#include <iostream>

#include <cstdio>

#include <poll.h>
#include <sys/socket.h>

#include "stringtools.hpp"

void ConnectionHandler :: threadMain() {
    char readbuffer[1024];

    struct pollfd polldata;
    polldata.fd = socket.handle();
    polldata.events = POLLERR | POLLIN | POLLPRI | POLLHUP;

    while (alive) {
        polldata.revents = 0;
        int pollres = poll(&polldata, 1, 500);
        if (polldata.revents || pollres) {
            if (polldata.revents & POLLIN) {
                const int readn = socket.read_n(readbuffer, 1024);
                if (readn < 0) {
                    std::cout << "read failed" << std::endl;
                } else if (readn > 0) {
                    readbuffer[1024 - 1] = 0;
                    if (readn < 1024) {
                        readbuffer[readn] = 0;
                    }

                    const std::string messages(readbuffer);
                    auto messageList = split(messages, '\n');
                    for (std::string msg : messageList) {
                        msg = trim(msg);
                        if (!msg.length()) {
                            continue;
                        }

                        auto parts = split(msg, ':');
                        if (parts.size() < 1) {
                            continue;
                        }

                        pushCommand(Command{
                            parts[0],
                            std::vector<std::string>(parts.begin() + 1, parts.end()),
                            this
                        });
                    }
                }
            }
            if (polldata.revents & POLLHUP) {
                alive = false;
                socket.close();
                continue;
            }
        }

        if (!socket.is_open()) {
            alive = false;
        }
    }
}

bool ConnectionHandler :: isAlive() const {
    return alive && thread.joinable() && socket.is_open();
}

void ConnectionHandler :: sendMessage(const std::string& msg) {
    int msgLen = msg.length();
    if (msgLen > 1024) {
        msgLen = 1024;
    }
    socket.write_n(msg.c_str(), msgLen);
}

void ConnectionHandler :: startShutdown() {
    alive = false;
}

ConnectionHandler :: ConnectionHandler(
    sockpp::unix_socket& _socket,
    PushCommandFunction _pushCommand
) : socket(_socket.release()), pushCommand(_pushCommand) {
    alive = true;
    thread = std::thread(&ConnectionHandler::threadMain, this);
}

ConnectionHandler :: ~ConnectionHandler() {
    startShutdown();
    thread.join();
}

void ControlSocket :: threadMain() {
    struct pollfd polldata;
    polldata.fd = acceptor->handle();
    polldata.events = POLLERR | POLLIN | POLLPRI | POLLHUP;

    while (alive) {
        polldata.revents = 0;
        int pollres = poll(&polldata, 1, 50);
        if (polldata.revents || pollres) {
            if (polldata.revents & POLLHUP) {
                alive = false;
                acceptor->close();
                continue;
            } else if (sockpp::unix_socket sock = acceptor->accept()) {
                std::lock_guard<std::mutex> lock(sharedResourceMutex);

                std::vector<ConnectionHandler*> toRemove;
                for (ConnectionHandler* ht : handlerThreads) {
                    if (!ht->isAlive()) {
                        toRemove.push_back(ht);
                    }
                }
                for (ConnectionHandler* ht : toRemove) {
                    handlerThreads.erase(
                        std::find(handlerThreads.begin(), handlerThreads.end(), ht)
                    );
                    delete ht;
                }

                handlerThreads.push_back(new ConnectionHandler(
                    sock,
                    [this](const Command& command) {
                        std::lock_guard<std::mutex> lock(sharedResourceMutex);
                        commands.push_back(command);
                    }
                ));
            }

        }
    }
}

void ControlSocket :: processEvents(CommandHandleFunction handler) {
    std::lock_guard<std::mutex> lock(sharedResourceMutex);
    if (commands.size() == 0) {
        return;
    }
    
    for (Command command : commands) {
        auto found = std::find(handlerThreads.begin(), handlerThreads.end(), command.handler);
        if ((found == handlerThreads.end()) || (!(*found)->isAlive())) {
            continue;
        }

        std::string result = handler(command.name, command.parameters);
        try {
            command.handler->sendMessage(result + "\n");
        }
        catch (const std::exception& e) {
            // Please find out what the actual exception type is
            std::cerr << "Error sending message: " << e.what() << std::endl;
        }
    }
    commands.clear();
}

void ControlSocket :: broadcastMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(sharedResourceMutex);

    for (ConnectionHandler* ch : handlerThreads) {
        try {
            ch->sendMessage(msg);
        }
        catch (const std::exception& e) {
            // Please find out what the actual exception type is
            std::cerr << "Error sending message: " << e.what() << std::endl;
        }
    }
}

ControlSocket :: ControlSocket(std::string socketAddr) : socketAddr(socketAddr) {
    alive = true;
    sockpp::initialize();

    acceptor = std::shared_ptr<sockpp::unix_acceptor>(new sockpp::unix_acceptor);
    int yes = 1;
    acceptor->set_option(SOL_SOCKET, SO_REUSEADDR, &yes);

    auto res = acceptor->open(sockpp::unix_address(socketAddr));
    if (!res) {
        throw SocketCreationFailed(acceptor->last_error_str());
    }
    mainThread = std::thread(&ControlSocket::threadMain, this);
}

ControlSocket :: ~ControlSocket() {
    {
        std::lock_guard<std::mutex> lock(sharedResourceMutex);
        for (ConnectionHandler* h : handlerThreads) {
            h->startShutdown();
        }
    }
    alive = false;

    {
        std::lock_guard<std::mutex> lock(sharedResourceMutex);
        for (ConnectionHandler* h : handlerThreads) {
            delete h;
        }
        handlerThreads.clear();
    }

    mainThread.join();

    if (acceptor) {
        acceptor->close();
    }

    std::remove(socketAddr.c_str());
}
