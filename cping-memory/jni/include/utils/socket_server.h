#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <string>
#include <atomic>
#include <thread>

class SocketServer
{
private:
    std::string socket_name;
    int server_socket;
    int client_socket;
    std::atomic<bool> is_running;
    std::thread server_thread;

public:
    explicit SocketServer(const std::string &name);
    ~SocketServer();

    bool start();
    void stop();
    bool send_raw(const void *data, size_t size);
    bool receive_raw(void *buffer, size_t size);

private:
    void run();
};

#endif // SOCKET_SERVER_H
