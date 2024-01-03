#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

typedef void (*CommandHandleFunction)(const std::string& command, const std::vector<std::string>& parameters);

class ConnectionHandler {
public:
    typedef std::shared_ptr<ConnectionHandler> Ptr;
private:
    sockpp::unix_socket socket;
    bool alive;
    std::thread thread;

    void threadMain();
public:
    bool isAlive() const;

    void sendMessage(const std::string& msg);
    void startShutdown();

    ConnectionHandler(sockpp::unix_socket& _socket);
    ~ConnectionHandler();
};

class ControlSocket {
private:
    bool alive;
    std::thread mainThread;
    std::shared_ptr<sockpp::unix_acceptor> acceptor;
    std::vector<ConnectionHandler*> handlerThreads;

    std::mutex sharedResourceMutex;

    void threadMain();
public:
    void processEvents(CommandHandleFunction handler);

    ControlSocket();
    ~ControlSocket();
};
