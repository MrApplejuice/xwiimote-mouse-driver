#include "controlsocket.hpp"

#include <iostream>
#include <exception>

#include <cstdio>

#include <poll.h>
#include <sys/socket.h>

class SocketFailed : public std::exception {};
class SocketCreationFailed : public SocketFailed {};

static const std::string SOCKET_ADDR = "/tmp/wiimote-mouse.sock";

static std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> result;
    std::string current = "";
    for (char c : str) {
        if (c == delim) {
            result.push_back(current);
            current = "";
        } else {
            current += c;
        }
    }
    if (current.length()) {
        result.push_back(current);
    }
    return result;
}

static constexpr bool isWhitespace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

static std::string trim(const std::string& str) {
    int start = 0;
    int end = str.length() - 1;
    while ((start < end) && isWhitespace(str[start])) {
        start++;
    }
    while ((start < end) && isWhitespace(str[end])) {
        end--;
    }
    return str.substr(start, end - start + 1);
}

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

        std::cout << "Committing command to handler: " << command.name << std::endl;
        std::string result = handler(command.name, command.parameters);
        command.handler->sendMessage(result + "\n");
    }
    commands.clear();
}

void ControlSocket :: broadcastMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(sharedResourceMutex);

    for (ConnectionHandler* ch : handlerThreads) {
        ch->sendMessage(msg);
    }
}

ControlSocket :: ControlSocket() {
    alive = true;
    sockpp::initialize();

    acceptor = std::shared_ptr<sockpp::unix_acceptor>(new sockpp::unix_acceptor);
    int yes = 1;
    acceptor->set_option(SOL_SOCKET, SO_REUSEADDR, &yes);

    auto res = acceptor->open(sockpp::unix_address(SOCKET_ADDR));
    if (!res) {
        throw SocketCreationFailed();
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

    std::remove(SOCKET_ADDR.c_str());
}
