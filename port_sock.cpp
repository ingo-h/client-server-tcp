// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-30

#include "port_sock.hpp"
#include "port.hpp"
#include <string>
#include <cstring>
#include <stdexcept>

namespace upnplib {

#ifdef _MSC_VER
CWSAStartup::CWSAStartup() {
    TRACE2(this, " Construct upnplib::CWSAStartup");
    WSADATA wsaData;
    int rc = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        throw std::runtime_error("ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() returns " +
                                 std::to_string(rc));
    }
}

CWSAStartup::~CWSAStartup() {
    TRACE2(this, " Destruct upnplib::CWSAStartup");
    ::WSACleanup();
}
#endif // MSC_VER


// Wrap socket() system call
// -------------------------
// Constructor
CSocket::CSocket() { TRACE2(this, " Construct upnplib::CSocket()"); }

// Move constructor
CSocket::CSocket(CSocket&& that) {
    TRACE2(this, " Construct move upnplib::CSocket()");
    fd = that.fd;
    that.fd = INVALID_SOCKET;
}

// Assignment operator (parameter as value)
CSocket& CSocket::operator=(CSocket that) {
    TRACE2(this, " Assignment operator upnplib::CSocket()");
    std::swap(fd, that.fd);
    return *this;
}

// Destructor
CSocket::~CSocket() {
    TRACE2(this, " Destruct upnplib::CSocket()");
    CLOSE_SOCKET_P(this->fd);
}

// Setter for the socket
void CSocket::set(int domain, int type, int protocol) {
    TRACE2(this, " Executing upnplib::CSocket::set()");

    // We get first a new socket before closing the old one to be sure that we
    // do not get the same one again.
    SOCKET sfd = ::socket(domain, type, protocol);
    if (sfd == INVALID_SOCKET) {
#ifdef _MSC_VER
        int err_no = ::WSAGetLastError();
        throw std::runtime_error(
            "ERROR! Failed to create socket: WSAGetLastError()=" +
            std::to_string(err_no));
#else
        throw std::runtime_error("ERROR! Failed to create socket: errno(" +
                                 std::to_string(errno) + ")=\"" +
                                 std::strerror(errno) + "\"");
#endif
    }
    CLOSE_SOCKET_P(this->fd);
    this->fd = sfd;
}

// Get the socket
CSocket::operator SOCKET&() { return this->fd; }

} // namespace upnplib
