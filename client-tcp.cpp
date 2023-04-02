// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-11

#include "client-tcp.hpp"
#include "port.hpp"
#include "addrinfo.hpp"
#include "socket.hpp"

#include <cstring>
#include <stdexcept>

namespace upnplib {

void quit_server() {
    TRACE("[Client] Executing upnplib::quit_server().")
    WINSOCK_INIT_P

    // Get a socket.
    CSocket sock(AF_INET6, SOCK_STREAM);

    // Get address information that should be connected.
    // -------------------------------------------------
    // The host address must fit to the protocol family (AF_*) of the socket
    // and the addrinfo hint. Host and port flags set to numeric use to avoid
    // expensive name resolution. With empty node the loopback interface is
    // selected.
    CAddrinfo ai("", "4433", AF_UNSPEC, SOCK_STREAM,
                 AI_NUMERICHOST | AI_NUMERICSERV);

    // Connect to address.
    // -------------------
    // Should be finished with shuthdown()
    int ret = ::connect(sock, ai->ai_addr, ai->ai_addrlen);
    if (ret != 0) {
#ifdef _WIN32
        throw std::runtime_error(
            "[Client] ERROR! Failed to connect: WSAGetLastError()=" +
            std::to_string(WSAGetLastError()));
#else
        throw std::runtime_error("[Client] ERROR! Failed to connect: errno(" +
                                 std::to_string(errno) + ")=\"" +
                                 std::strerror(errno) + "\"");
#endif
    }

    // Send Quit message to the server.
    ssize_t valsend = ::send(sock, "Q", 1, 0);
    if (valsend != 1) {
#ifdef _WIN32
        throw std::runtime_error("[Client] ERROR! Failed to send server Quit "
                                 "message: WSAGetLastError()=" +
                                 std::to_string(WSAGetLastError()));
#else
        throw std::runtime_error(
            "[Client] ERROR! Failed to send server Quit message: errno(" +
            std::to_string(errno) + ")=\"" + std::strerror(errno) + "\"");
#endif
    }

    // closing the conection
    ::shutdown(sock, SHUT_RDWR);
    TRACE("[Client] \"Q\" sent (Quit server).")
}

} // namespace upnplib
