#include "utils/socket_server.h"
#include "debug/logger.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

SocketServer::SocketServer(const std::string &name)
    : socket_name(name), server_socket(-1), client_socket(-1), is_running(false)
{
}

SocketServer::~SocketServer()
{
    stop();
}

bool SocketServer::start()
{
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        Logger::e("[Server] Failed to create socket: %s", strerror(errno));
        return false;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    memcpy(addr.sun_path + 1, socket_name.c_str(), socket_name.size());
    socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + 1 + socket_name.size();

    if (bind(server_socket, (struct sockaddr *)&addr, addr_len) == -1)
    {
        Logger::e("[Server] Failed to bind: %s", strerror(errno));
        close(server_socket);
        return false;
    }

    if (listen(server_socket, 1) == -1)
    {
        Logger::e("[Server] Failed to listen: %s", strerror(errno));
        close(server_socket);
        return false;
    }

    is_running = true;
    server_thread = std::thread(&SocketServer::run, this);
    return true;
}

void SocketServer::stop()
{
    is_running = false;

    if (client_socket != -1)
        close(client_socket);
    if (server_socket != -1)
        close(server_socket);

    if (server_thread.joinable())
        server_thread.join();
}

bool SocketServer::send_raw(const void *data, size_t size)
{
    if (client_socket == -1)
        return false;

    const char *ptr = static_cast<const char *>(data);
    size_t sent = 0;
    while (sent < size)
    {
        ssize_t n = send(client_socket, ptr + sent, size - sent, 0);
        if (n <= 0)
            return false;
        sent += n;
    }
    return true;
}

bool SocketServer::receive_raw(void *buffer, size_t size)
{
    if (client_socket == -1)
        return false;

    char *ptr = static_cast<char *>(buffer);
    size_t received = 0;
    while (received < size)
    {
        ssize_t n = recv(client_socket, ptr + received, size - received, 0);
        if (n <= 0)
            return false;
        received += n;
    }
    return true;
}

void SocketServer::run()
{
    while (is_running)
    {
        client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == -1)
            continue;

        while (is_running)
        {
            struct Request
            {
                int a;
                float b;
            } req;

            if (!receive_raw(&req, sizeof(req)))
                break;

            struct Response
            {
                bool ok;
                char message[32];
            } resp = {true, "Acknowledged"};

            if (!send_raw(&resp, sizeof(resp)))
                break;
        }

        close(client_socket);
        client_socket = -1;
    }
}
