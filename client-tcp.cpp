// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-21

#include "client-tcp.hpp"
#include "port.hpp"
#include "port_sock.hpp"
#include "addrinfo.hpp"
#include <cstring>
#include <string>
#include <stdexcept>

namespace upnplib {

void quit_server() {
    WINSOCK_INIT_P

    // Get a socket.
    CSocket sock(AF_INET6, SOCK_STREAM, 0);

    // Get address information that should be connected.
    // -------------------------------------------------
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    // For host and port only numeric to avoid expensive name resolution.
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    // The host address must fit to the protocol family (AF_*) of the socket and
    // the addrinfo hint.
    CAddrinfo ai("::1", "4433", &hints);

    // Connect to address.
    // -------------------
    // Should be finished with shuthdown()?
    int ret = connect(sock, ai->ai_addr, ai->ai_addrlen);
    if (ret != 0) {
#ifdef _WIN32
        int err_no = WSAGetLastError();
        throw std::runtime_error(
            "[Client] ERROR! Failed to connect: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        int err_no = errno;
        throw std::runtime_error("[Client] ERROR! Failed to connect: errno(" +
                                 std::to_string(err_no) + ")=\"" +
                                 std::strerror(err_no) + "\"");
#endif
    }

    // Send Quit message to the server.
    ssize_t valsend = send(sock, "Q", 1, 0);
    if (valsend != 1) {
#ifdef _WIN32
        int err_no = WSAGetLastError();
        throw std::runtime_error("[Client] ERROR! Failed to send server Quit "
                                 "message: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        throw std::runtime_error(
            "[Client] ERROR! Failed to send server Quit message: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\"");
#endif
    }

    // closing the conection
    TRACE("[Client] \"Q\" sent (Quit server).\n");
}

} // namespace upnplib
