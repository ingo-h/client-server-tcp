// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-15

#include "client-server-tcp.hpp"
#include <stdexcept>
#include <thread>
#include <cstring>
#include <string>

namespace compa {

// Simple TLS Server
// =================

CSimpleTLSServer::CSimpleTLSServer() {
    TRACE("Construct compa::CSimpleTLSServer\n");
#ifdef _WIN32
    // Initialize Windows sochets
    // --------------------------
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != NO_ERROR) {
        // This exception here calls std::terminate, means the destructor isn't
        // executed. We cannot use the server if don't have sockets available.
        throw std::runtime_error(
            "FATAL! Failed to initialize Windows sockets (WSAStartup).");
    }
#endif

    // Get a socket.
    // -------------
    // Should be finished with CLOSE_SOCKET_P()
    m_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (m_sock == INVALID_SOCKET) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        WSACleanup();
        throw std::runtime_error(
            "FATAL! Failed to create socket: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        throw std::runtime_error("FATAL! Failed to create socket: errno(" +
                                 std::to_string(errno) + ")=\"" +
                                 std::strerror(errno) + "\"");
#endif
    }

    // Get socket option IPV6_V6ONLY for information/developement.
    // -----------------------------------------------------------
    int ipv6_only{-1};
    socklen_t optlen{sizeof(ipv6_only)};
    if (getsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6_only,
                   &optlen) != 0) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        CLOSE_SOCKET_P(m_sock);
        WSACleanup();
        throw std::runtime_error("FATAL! Failed to get socket option "
                                 "IPV6_V6ONLY: WSAGetLastError()=" +
                                 std::to_string(err_no));
#else
        int err_no = errno;
        CLOSE_SOCKET_P(m_sock);
        throw std::runtime_error(
            "FATAL! Failed to get socket option IPV6_V6ONLY: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\"");
#endif
    }
    std::cout << "INFO: IPV6_V6ONLY = " << ipv6_only << ".\n";

    // Set socket option IPV6_V6ONLY to false, means allowing IPv4 and IPv6.
    // ---------------------------------------------------------------------
    // Set option only if needed.
    if (ipv6_only == 1) {
        ipv6_only = 0;
        // Type cast (char*)&ipv6_only is needed for Microsoft Windows.
        if (setsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6_only,
                       optlen) != 0) {
            // Cleanup already allocated resources because the destructor isn't
            // called with throwing an exception here.
#ifdef _WIN32
            int err_no = WSAGetLastError();
            CLOSE_SOCKET_P(m_sock);
            WSACleanup();
            throw std::runtime_error("FATAL! Failed to set socket option "
                                     "IPV6_V6ONLY: WSAGetLastError()=" +
                                     std::to_string(err_no));
#else
            int err_no = errno;
            CLOSE_SOCKET_P(m_sock);
            throw std::runtime_error(
                "FATAL! Failed to set socket option IPV6_V6ONLY: errno(" +
                std::to_string(errno) + ")=\"" + std::strerror(errno) + "\"");
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
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
        CLOSE_SOCKET_P(m_sock);
#ifdef _WIN32
        // Cleanup Windows sochets
        WSACleanup();
#endif
        throw std::runtime_error(
            "FATAL! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + gai_strerror(ret) + "\"");
    }

    // Bind socket to a local address.
    // -------------------------------
    if (bind(m_sock, m_ai->ai_addr, m_ai->ai_addrlen) == SOCKET_ERROR) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(m_ai);
        CLOSE_SOCKET_P(m_sock);
        WSACleanup();
        throw std::runtime_error(
            "FATAL! Failed to bind a socket to an address: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(m_ai);
        CLOSE_SOCKET_P(m_sock);
        throw std::runtime_error(
            "FATAL! Failed to bind a socket to an address: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\".");
#endif
    }

    // Listen on socket for incomming messages.
    // ----------------------------------------
    // Should be finished with shutdown()
    if (listen(m_sock, 1) != 0) {
        // Cleanup already allocated resources because the destructor isn't
        // called with throwing an exception here.
#ifdef _WIN32
        int err_no = WSAGetLastError();
        freeaddrinfo(m_ai);
        CLOSE_SOCKET_P(m_sock);
        WSACleanup();
        throw std::runtime_error(
            "FATAL! Failed to bind a socket to an address: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        int err_no = errno;
        freeaddrinfo(m_ai);
        CLOSE_SOCKET_P(m_sock);
        throw std::runtime_error(
            "FATAL! Failed to bind a socket to an address: errno(" +
            std::to_string(err_no) + ")=\"" + std::strerror(err_no) + "\".");
#endif
    }
} // end constructor


CSimpleTLSServer::~CSimpleTLSServer() {
    TRACE("Destruct compa::CSimpleTLSServer\n");
    shutdown(m_sock, SHUT_RDWR);
    freeaddrinfo(m_ai);
    CLOSE_SOCKET_P(m_sock);
#ifdef _WIN32
    // Cleanup Windows sochets
    WSACleanup();
#endif
}


void CSimpleTLSServer::run() {
    TRACE("executing compa::CSimpleTLSServer::run()\n");
    m_ready = true;
}

bool CSimpleTLSServer::ready(int a_delay) const {
    if (!m_ready)
        // This is only to aviod busy polling from the calling thread.
        std::this_thread::sleep_for(std::chrono::microseconds(a_delay));
    return m_ready;
}

} // namespace compa
