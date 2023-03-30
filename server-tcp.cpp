// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-30

#include "server-tcp.hpp"
#include "port.hpp"
#include <thread>
#include <cstring>
#include <string>
#include <stdexcept>
#include <iostream>

namespace upnplib {

// Simple TCP Server
// =================

CServerTCP::CServerTCP() {
    TRACE2(this, " Construct upnplib::CServerTCP");
#ifdef _WIN32
    // Initialize Windows sochets
    // --------------------------
    // Should be finished with WSACleanup().
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != NO_ERROR) {
        // This exception here calls std::terminate, means the destructor isn't
        // executed. We cannot use the server if don't have sockets available.
        throw std::runtime_error("[Server] ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() return code = " +
                                 std::to_string(rc));
    }
#endif

    // Get a socket.
    // -------------
    {
        CSocket sock(AF_INET6, SOCK_STREAM);
        m_listen_sfd = std::move(sock);
    }
    if (m_listen_sfd == INVALID_SOCKET) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        WSACleanup();
        throw std::runtime_error(
            "[Server] ERROR! Failed to create socket: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        throw std::runtime_error(
            "[Server] ERROR! Failed to create socket: errno(" +
            std::to_string(errno) + ")=\"" + std::strerror(errno) + "\"");
#endif
    }

    // Get socket option IPV6_V6ONLY for information/developement.
    // -----------------------------------------------------------
    int ipv6_only{-1};
    socklen_t optlen{sizeof(ipv6_only)};
    if (getsockopt(m_listen_sfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6_only,
                   &optlen) != 0) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        WSACleanup();
        throw std::runtime_error("[Server] ERROR! Failed to get socket option "
                                 "IPV6_V6ONLY: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        throw std::runtime_error(
            "[Server] ERROR! Failed to get socket option IPV6_V6ONLY: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\"");
#endif
    }
    std::cout << "[Server] INFO: IPV6_V6ONLY = " << ipv6_only << ".\n";

    // Set socket option IPV6_V6ONLY to false, means allowing IPv4 and IPv6.
    // ---------------------------------------------------------------------
    // Set option only if needed.
    if (ipv6_only == 1) {
        ipv6_only = 0;
        // Type cast (char*)&ipv6_only is needed for Microsoft Windows.
        if (setsockopt(m_listen_sfd, IPPROTO_IPV6, IPV6_V6ONLY,
                       (char*)&ipv6_only, optlen) != 0) {
            // Cleanup already allocated resources because the destructor isn't
            // called with throwing an exception here.
#ifdef _WIN32
            int err_no = WSAGetLastError();
            WSACleanup();
            throw std::runtime_error(
                "[Server] ERROR! Failed to set socket option "
                "IPV6_V6ONLY: WSAGetLastError()=" +
                std::to_string(err_no));
#else
            int err_no = errno;
            throw std::runtime_error("[Server] ERROR! Failed to set socket "
                                     "option IPV6_V6ONLY: errno(" +
                                     std::to_string(err_no) + ")=\"" +
                                     std::strerror(err_no) + "\"");
#endif
        }
    }

    // Get local address information that can be bound to the socket.
    // --------------------------------------------------------------
    // Should be finished with freeaddrinfo()
    addrinfo hints{};
    hints.ai_family = AF_INET6;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    // For host and port only numeric to avoid expensive name resolution.
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;

    int ret = getaddrinfo(nullptr, "4433", &hints, &m_ai);
    if (ret != 0) {
#ifdef _WIN32
        // Cleanup Windows sochets
        WSACleanup();
#endif
        throw std::runtime_error(
            "[Server] ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + gai_strerror(ret) + "\"");
    }

    // Bind socket to a local address.
    // -------------------------------
    if (bind(m_listen_sfd, m_ai->ai_addr, m_ai->ai_addrlen) == SOCKET_ERROR) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(m_ai);
        WSACleanup();
        throw std::runtime_error("[Server] ERROR! Failed to bind a socket to "
                                 "an address: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(m_ai);
        throw std::runtime_error(
            "[Server] ERROR! Failed to bind a socket to an address: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\".");
#endif
    }

    // Listen on socket for incomming messages.
    // ----------------------------------------
    // Should be finished with shutdown()
    if (listen(m_listen_sfd, 1) != 0) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(m_ai);
        WSACleanup();
        throw std::runtime_error("[Server] ERROR! Failed to bind a socket to "
                                 "an address: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(m_ai);
        throw std::runtime_error(
            "[Server] ERROR! Failed to bind a socket to an address: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\".");
#endif
    }
} // end constructor


CServerTCP::~CServerTCP() {
    TRACE2(this, " Destruct upnplib::CServerTCP");
    shutdown(m_listen_sfd, SHUT_RDWR);
    freeaddrinfo(m_ai);
#ifdef _WIN32
    // Cleanup Windows sochets
    WSACleanup();
#endif
}


void CServerTCP::run() {
    // This method can run in a thread and should be thread safe.
    TRACE2(this, " executing upnplib::CServerTCP::run()");
    // This flag is only set here and shows that the server is ready to accept
    // requests.
    m_ready = true;

    SOCKET accept_sfd{INVALID_SOCKET};
    char buffer[1024]{};

    // Accept an incomming request. This call is blocking.
    // ---------------------------------------------------
    // Method will quit if we have received a single "Q" string (['Q', '\0']).
    ssize_t valread{};
    while (buffer[0] != 'Q' || valread != 1) {
        accept_sfd = accept(m_listen_sfd, nullptr, nullptr);
        if (accept_sfd == INVALID_SOCKET) {
#ifdef _WIN32
            int err_no = WSAGetLastError();
            throw std::runtime_error("[Server] ERROR! Failed to accept an "
                                     "incomming request: WSAGetLastError()=" +
                                     std::to_string(err_no));
#else
            int err_no = errno;
            throw std::runtime_error("[Server] ERROR! Failed to accept an "
                                     "incomming request: errno(" +
                                     std::to_string(err_no) + ")=\"" +
                                     std::strerror(err_no) + "\".");
#endif
        }

        // Read accepted connection.
        // -------------------------
        valread = recv(accept_sfd, buffer, sizeof(buffer) - 1, 0);
#ifdef _WIN32
        int err_no = WSAGetLastError();
#else
        int err_no = errno;
#endif
        CLOSE_SOCKET_P(accept_sfd);

        switch (valread) {
        case 0:
#ifdef _WIN32
            throw std::runtime_error(
                "[Server] ERROR! Read an incomming request with message length "
                "= 0: WSAGetLastError()=" +
                std::to_string(err_no));
#else
            throw std::runtime_error("[Server] ERROR! Read an incomming "
                                     "request with message length = 0: errno(" +
                                     std::to_string(err_no) + ")=\"" +
                                     std::strerror(err_no) + "\".");
#endif
        case SOCKET_ERROR:
#ifdef _WIN32
            throw std::runtime_error("[Server] ERROR! Failed to read an "
                                     "incomming request: WSAGetLastError()=" +
                                     std::to_string(err_no));
#else
            throw std::runtime_error("[Server] ERROR! Failed to read an "
                                     "incomming request: errno(" +
                                     std::to_string(err_no) + ")=\"" +
                                     std::strerror(err_no) + "\".");
#endif
        }
        buffer[valread] = '\0';
    }
    TRACE2(this, " [Server] Quit.");
}

bool CServerTCP::ready(int a_delay) const {
    if (!m_ready)
        // This is only to aviod busy polling from the calling thread.
        std::this_thread::sleep_for(std::chrono::microseconds(a_delay));
    return m_ready;
}

} // namespace upnplib
