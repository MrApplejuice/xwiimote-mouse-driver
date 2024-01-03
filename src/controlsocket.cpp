#include "controlsocket.hpp"

#include <iostream>

#include <cstdio>

#include <poll.h>
#include <sys/socket.h>

static const std::string SOCKET_ADDR = "/tmp/wiimote-mouse.sock";

void ConnectionHandler :: threadMain() {
    char readbuffer[1024];

    struct pollfd polldata;
    polldata.fd = socket.handle();
    polldata.events = POLLERR | POLLIN | POLLPRI;

    while (alive) {
        polldata.revents = 0;
        int pollres = poll(&polldata, 1, 500);
        std::cout << "socket pollres = " << pollres << std::endl;
        if (polldata.revents || pollres) {
            std::cout << "socket revents = " << polldata.revents << std::endl;
            std::cout << "socket err = " << socket.last_error_str() << std::endl;

            if (polldata.revents & POLLHUP) {
                std::cout << "Lost connection!" << std::endl;
                alive = false;
                socket.close();
                continue;
            }
            if (polldata.revents & POLLIN) {
                const int readn = socket.read_n(readbuffer, 1024);
                if (readn < 0) {
                    std::cout << "read failed" << std::endl;
                } else if (readn > 0) {
                    readbuffer[1024 - 1] = 0;
                    if (readn < 1024) {
                        readbuffer[readn] = 0;
                    }

                    std::cout << "received: " << readbuffer << std::endl;
                }
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
    
}

void ConnectionHandler :: startShutdown() {
    alive = false;
}

ConnectionHandler :: ConnectionHandler(sockpp::unix_socket& _socket) : socket(_socket.release()) {
    alive = true;
    thread = std::thread(&ConnectionHandler::threadMain, this);
}

ConnectionHandler :: ~ConnectionHandler() {
    startShutdown();
    thread.join();
}

void ControlSocket :: threadMain() {
    timespec timeout;
    timeout.tv_nsec = 100000000L;
    timeout.tv_sec = 0L;

    sockpp::socket_t handle = acceptor->handle();
    struct pollfd polldata;
    polldata.fd = handle;
    polldata.events = POLLERR | POLLIN | POLLPRI;

    while (alive) {
        polldata.revents = 0;
        int ppollres = poll(&polldata, 1, 50);
        if (polldata.revents || ppollres) {
            std::cout << "ppollres = " << ppollres << std::endl;
            std::cout << "revents = " << polldata.revents << std::endl;
        }

        if (polldata.revents) {
            while (sockpp::unix_socket sock = acceptor->accept()) {
                std::lock_guard<std::mutex> lock(sharedResourceMutex);

                std::cout << "accept" << std::endl;
                handlerThreads.push_back(new ConnectionHandler(sock));
            }
        }
    }
}

void ControlSocket :: processEvents(CommandHandleFunction handler) {
}

ControlSocket :: ControlSocket() {
    alive = true;
    sockpp::initialize();

    acceptor = std::shared_ptr<sockpp::unix_acceptor>(new sockpp::unix_acceptor);
    int yes = 1;
    acceptor->set_option(SOL_SOCKET, SO_REUSEADDR, &yes);

    auto res = acceptor->open(sockpp::unix_address(SOCKET_ADDR));
    if (!res) {
        std::cout << "Nope!" << std::endl;
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
    mainThread.join();

    {
        std::lock_guard<std::mutex> lock(sharedResourceMutex);
        for (ConnectionHandler* h : handlerThreads) {
            delete h;
        }
        handlerThreads.clear();
    }

    if (acceptor) {
        acceptor->close();
    }
    std::remove(SOCKET_ADDR.c_str());
}
