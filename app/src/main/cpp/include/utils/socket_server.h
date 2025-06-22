#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H


class SocketServer {
private:
    std::string socket_name;
    int server_socket;
    int client_socket;
    std::atomic<bool> is_running;
    std::thread server_thread;

public:
    explicit SocketServer(std::string name) : socket_name(std::move(name)), server_socket(-1),
                                              client_socket(-1),
                                              is_running(false) {}

    ~SocketServer() {
        stop();
    }

    bool start() {
        LOGI(TEST_EXP, "[Server] Creating socket...");
        server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_socket == -1) {
            LOGE(TEST_EXP, "[Server] Failed to create socket: %s", strerror(errno));
            return false;
        }

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        addr.sun_path[0] = '\0';
        memcpy(addr.sun_path + 1, socket_name.c_str(), socket_name.size());
        socklen_t
        addr_len = offsetof(
        struct sockaddr_un, sun_path) +1 + socket_name.size();

        LOGI(TEST_EXP, "[Server] Binding to abstract socket...");
        int bind_val = ::bind(server_socket, (struct sockaddr *) &addr, addr_len);
        if (bind_val == -1) {
            LOGE(TEST_EXP, "[Server] Failed to bind: %s", strerror(errno));
            close(server_socket);
            return false;
        }

        LOGI(TEST_EXP, "[Server] Listening...");
        if (listen(server_socket, 8) == -1) {
            LOGE(TEST_EXP, "[Server] Failed to listen: %s", strerror(errno));
            close(server_socket);
            return false;
        }

        LOGI(TEST_EXP, "[Server] Listening ready.");

        is_running = true;
        server_thread = std::thread(&SocketServer::run, this);
        return true;
    }

    void stop() {
        is_running = false;

        if (client_socket != -1)
            close(client_socket);
        if (server_socket != -1)
            close(server_socket);

        if (server_thread.joinable())
            server_thread.join();
    }

    bool send_raw(const void *data, size_t size) const {
        if (client_socket == -1)
            return false;
        const char *ptr = static_cast<const char *>(data);
        size_t sent = 0;
        while (sent < size) {
            ssize_t n = send(client_socket, ptr + sent, size - sent, 0);
            if (n <= 0)
                return false;
            sent += n;
        }
        return true;
    }

    bool receive_raw(void *buffer, size_t size) const {
        if (client_socket == -1)
            return false;
        char *ptr = static_cast<char *>(buffer);
        size_t received = 0;
        while (received < size) {
            ssize_t n = recv(client_socket, ptr + received, size - received, 0);
            if (n <= 0)
                return false;
            received += n;
        }
        return true;
    }

private:
    void run() {
        while (is_running) {
            LOGI(TEST_EXP, "[Server] Waiting for connection...");
            client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket == -1)
                continue;

            LOGI(TEST_EXP, "[Server] Client connected.");
            break;
        }
    }
};

#endif // SOCKET_SERVER_H