// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-17

#include "client-tcp.hpp"
#include "port.hpp"
#include "port_sock.hpp"
#include <cstring>
#include <string>
#include <stdexcept>

namespace upnplib {

void quit_server() {
#ifdef _WIN32
    // Initialize Windows sochets
    // --------------------------
    // Should be finished with WSACleanup().
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != NO_ERROR) {
        throw std::runtime_error("[Client] ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() return code = " +
                                 std::to_string(rc));
    }
#endif
    // Get a socket.
    // -------------
    // Should be finished with CLOSE_SOCKET_P()
    SOCKET client_sfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_sfd == INVALID_SOCKET) {
#ifdef _WIN32
        int err_no = WSAGetLastError();
        WSACleanup();
        throw std::runtime_error(
            "[Client] ERROR! Failed to create socket: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        throw std::runtime_error(
            "[Client] ERROR! Failed to create socket: errno(" +
            std::to_string(errno) + ")=\"" + std::strerror(errno) + "\"");
#endif
    }

    // Get address information that should be connected.
    // -------------------------------------------------
    // Should be finished with freeaddrinfo()
    addrinfo* ai;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    // For host and port only numeric to avoid expensive name resolution.
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    // The host address must fit to the protocol family (AF_*) of the socket and
    // the addrinfo hint.
    int ret = getaddrinfo("::1", "4433", &hints, &ai);
    if (ret != 0) {
        // Cleanup already allocated resources.
        CLOSE_SOCKET_P(client_sfd);
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error(
            "[Client] ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + gai_strerror(ret) + "\"");
    }

    // Connect to address.
    // -------------------
    ret = connect(client_sfd, ai->ai_addr, ai->ai_addrlen);
    if (ret != 0) {
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(ai);
        CLOSE_SOCKET_P(client_sfd);
        WSACleanup();
        throw std::runtime_error(
            "[Client] ERROR! Failed to connect: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(ai);
        CLOSE_SOCKET_P(client_sfd);
        throw std::runtime_error("[Client] ERROR! Failed to connect: errno(" +
                                 std::to_string(err_no) + ")=\"" +
                                 std::strerror(err_no) + "\"");
#endif
    }

    // Send Quit message to the server.
    ssize_t valsend = send(client_sfd, "Q", 1, 0);

    if (valsend != 1) {
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(ai);
        CLOSE_SOCKET_P(client_sfd);
        WSACleanup();
        throw std::runtime_error("[Client] ERROR! Failed to send server Quit "
                                 "message: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(ai);
        CLOSE_SOCKET_P(client_sfd);
        throw std::runtime_error(
            "[Client] ERROR! Failed to send server Quit message: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\"");
#endif
    }

    // closing the conection
    freeaddrinfo(ai);
    CLOSE_SOCKET_P(client_sfd);
#ifdef _WIN32
    WSACleanup();
#endif
    TRACE("[Client] \"Q\" sent (Quit server).\n");
}

} // namespace upnplib
