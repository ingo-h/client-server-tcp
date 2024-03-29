// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2024-01-12

#include "server-tcp.hpp"
#include "port.hpp"
#include "addrinfo.hpp"
#include <thread>
#include <cstring>
#include <stdexcept>

namespace upnplib {

static inline void throw_error(std::string errmsg) {
    // error number given by WSAGetLastError(), resp. contained in errno is
    // used to specify details of the error.
#ifdef _WIN32
    throw std::runtime_error(
        errmsg + " WSAGetLastError()=" + std::to_string(WSAGetLastError()));
#else
    throw std::runtime_error(errmsg + " errno(" + std::to_string(errno) +
                             ")=\"" + std::strerror(errno) + "\"");
#endif
}

// Simple TCP Server
// =================

CServerTCP::CServerTCP(const std::string& a_port,
                       [[maybe_unused]] const bool a_reuse_addr)
    : m_listen_sfd(AF_INET6, SOCK_STREAM) {
    TRACE2(this, " Construct upnplib::CServerTCP")

    // Get local address information that can be bound to the socket.
    // --------------------------------------------------------------
    // AF_INET6 serves both IPv4 and IPv6 if IPV6_V6ONLY flag is set to false.
    // Host and port are only numeric to avoid expensive name resolution.
    // If AI_PASSIVE is set then node must be empty to get a passive usable
    // address (passive usage will be set with listen).
    CAddrinfo ai("", a_port.c_str(), AF_INET6, SOCK_STREAM,
                 AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    // Bind socket to a local address.
    // -------------------------------
    m_listen_sfd.bind(ai);

    // Listen specifies passive usage of the socket for incomming connections.
    // -----------------------------------------------------------------------
    m_listen_sfd.listen();

} // end constructor


CServerTCP::~CServerTCP() { TRACE2(this, " Destruct upnplib::CServerTCP") }


void CServerTCP::run() {
    // TODO: Improve protocol handling
    // REF: [close vs shutdown socket?]
    // (https://stackoverflow.com/q/4160347/5014688)
    //
    // This method can run in a thread and should be thread safe.
    // Method will quit if we have received a single "Q" string (['Q', '\0']).
    TRACE2(this, " executing upnplib::CServerTCP::run()")

    // Accept an incomming request. This call is blocking.
    // ---------------------------------------------------
    // Should be finished with shutdown()
    SOCKET accept_sfd{INVALID_SOCKET};
    char buffer[1024]{};
    ssize_t valread{};

    // Now we are ready to accept requests and flag this. To be thread safe we
    // should do it normaly after calling accept() but we cannot do it because
    // it is blocking. In this case it should not matter because incomming
    // characters are cached by the operating system?
    m_ready = true;

    while (buffer[0] != 'Q' || valread != 1) {
        accept_sfd = ::accept(m_listen_sfd, nullptr, nullptr);
        if (accept_sfd == INVALID_SOCKET)
            throw_error("[Server] ERROR! MSG1022: Failed to accept an "
                        "incomming request:");

        // Read accepted connection.
        // -------------------------
        valread = ::recv(accept_sfd, buffer, sizeof(buffer) - 1, 0);

        ::shutdown(accept_sfd, SHUT_RDWR);
        CLOSE_SOCKET_P(accept_sfd);

        switch (valread) {
        case 0:
            throw_error(
                "[Server] ERROR! MSG1023: Read an incomming request with "
                "message length = 0:");
            break; // Needed to suppress compiler warning
        case SOCKET_ERROR:
            throw_error("[Server] ERROR! MSG1024: Failed to read an incomming "
                        "request:");
        }
        buffer[valread] = '\0';
    } // while

    TRACE2(this, " [Server] Quit.")
}

bool CServerTCP::ready(int a_delay) const {
    if (!m_ready)
        // This is only to aviod busy polling from the calling thread.
        std::this_thread::sleep_for(std::chrono::microseconds(a_delay));
    return m_ready;
}

bool CServerTCP::is_v6only() const {
    TRACE2(this, " Executing upnplib::CServerTCP::get_v6only()")
    return m_listen_sfd.is_v6only();
}

bool CServerTCP::is_reuse_addr() const {
    TRACE2(this, " Executing upnplib::CServerTCP::get_reuse_addr()")
    return m_listen_sfd.is_reuse_addr();
}

uint16_t CServerTCP::get_port() const {
    TRACE2(this, " Executing upnplib::CServerTCP::get_port()")
    return m_listen_sfd.get_port();
}

bool CServerTCP::is_listen() const {
    TRACE2(this, " Executing upnplib::CServerTCP::get_listen()")
    return m_listen_sfd.is_listen();
}

} // namespace upnplib
