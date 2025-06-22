#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

class SocketClient {
private:
    int sock_fd;

public:
    SocketClient() : sock_fd(-1) {}

    ~SocketClient() { close_connection(); }

    bool connect_to_server(const char *socket_name) {
        LOGI(TEST_TAG, "Creating client socket...");
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            LOGE(TEST_TAG, "socket() failed: %s", strerror(errno));
            return false;
        }

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;

        addr.sun_path[0] = '\0';  // Abstract socket prefix
        size_t name_len = strlen(socket_name);
        memcpy(addr.sun_path + 1, socket_name, name_len);

        socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + 1 + name_len;

        LOGI(TEST_TAG, "Attempting to connect to abstract socket: %s", socket_name);
        int result = connect(sock_fd, (struct sockaddr *) &addr, addr_len);
        if (result == -1) {
            LOGE(TEST_TAG, "connect() failed: %s", strerror(errno));
            close(sock_fd);
            sock_fd = -1;
            return false;
        }

        LOGI(TEST_TAG, "Successfully connected to server!");
        return true;
    }


    bool send_raw(const void *data, size_t size) const {
        const char *ptr = static_cast<const char *>(data);
        size_t sent = 0;
        while (sent < size) {
            ssize_t n = send(sock_fd, ptr + sent, size - sent, 0);
            if (n <= 0) return false;
            sent += n;
        }
        return true;
    }

    bool receive_raw(void *buffer, size_t size) const {
        char *ptr = static_cast<char *>(buffer);
        size_t received = 0;
        while (received < size) {
            ssize_t n = recv(sock_fd, ptr + received, size - received, 0);
            if (n <= 0) return false;
            received += n;
        }
        return true;
    }

    void close_connection() {
        if (sock_fd != -1) {
            close(sock_fd);
            sock_fd = -1;
        }
    }
};

#endif // SOCKET_CLIENT_H
