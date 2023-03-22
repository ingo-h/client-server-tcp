// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-21

#include "port_sock.hpp"
#include "port.hpp"
#include <string>
#include <cstring>
#include <stdexcept>

namespace upnplib {

#ifdef _MSC_VER
CWSAStartup::CWSAStartup() {
    TRACE2(this, " Construct upnplib::CWSAStartup\n");
    WSADATA wsaData;
    int rc = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        throw std::runtime_error("ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() returns " +
                                 std::to_string(rc));
    }
}

CWSAStartup::~CWSAStartup() {
    TRACE2(this, " Destruct upnplib::CWSAStartup\n");
    ::WSACleanup();
}
#endif // MSC_VER


// Wrap socket() system call
// -------------------------
CSocket::CSocket(int domain, int type, int protocol) {
    TRACE2(this, " Construct upnplib::CSocket\n");
    this->fd = ::socket(domain, type, protocol);
    if (this->fd == INVALID_SOCKET) {
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
}

CSocket::~CSocket() {
    TRACE2(this, " Destruct upnplib::CSocket\n");
    CLOSE_SOCKET_P(this->fd);
}

CSocket::operator SOCKET() const { return this->fd; }

} // namespace upnplib
