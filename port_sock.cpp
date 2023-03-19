// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-20

#include "port_sock.hpp"
#include <string>
#include <cstring>
#include <stdexcept>

#ifdef UPNPLIB_WITH_TRACE
#include <iostream>
#define TRACE(s) std::clog << "TRACE(" << __LINE__ << "): " << (s)
#else
#define TRACE(s)
#endif

namespace upnplib {

#ifdef _MSC_VER
CWSAStartup::CWSAStartup() {
    TRACE("Construct upnplib::CWSAStartup\n");
    WSADATA wsaData;
    int rc = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        throw std::runtime_error("ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() returns " +
                                 std::to_string(rc));
    }
}

CWSAStartup::~CWSAStartup() {
    TRACE("Destruct upnplib::CWSAStartup\n");
    ::WSACleanup();
}
#endif // MSC_VER


// Wrap socket() system call
// -------------------------
CSocket::CSocket(int domain, int type, int protocol) {
    TRACE("Construct upnplib::CSocket\n");
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
    TRACE("Destruct upnplib::CSocket\n");
    CLOSE_SOCKET_P(this->fd);
}

CSocket::operator SOCKET() const { return this->fd; }


// Wrap getaddrinfo() system call
// ------------------------------
// Default constructor to provide an empty object.
CGetaddrinfo::CGetaddrinfo() {
    TRACE("Construct default upnplib::CGetaddrinfo()\n");
}

// Constructor with getting an address information.
CGetaddrinfo::CGetaddrinfo(const char* node, const char* service,
                           const struct addrinfo* hints) {
    TRACE("Construct upnplib::CGetaddrinfo(..) with arguments\n");
    int ret = ::getaddrinfo(node, service, hints, &m_res);
    if (ret != 0) {
        throw std::runtime_error(
            "ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"");
    }
}

// Copy constructor
CGetaddrinfo::CGetaddrinfo(const CGetaddrinfo& other) {
    TRACE("Executing upnplib::CGetaddrinfo() copy constructor\n");
    m_res = this->copy_addrinfo(other);
}

// Copy assignment operator
CGetaddrinfo& CGetaddrinfo::operator=(const CGetaddrinfo& other) {
    TRACE("Executing upnplib::CGetaddrinfo::operator=\n");
    if (this != &other) { // protect against invalid self-assignment
        // 1: allocate new memory and copy the elements
        addrinfo* new_res = this->copy_addrinfo(other);
        // 2: deallocate old memory
        ::freeaddrinfo(m_res);
        // 3: assign the new memory to the object
        m_res = new_res;
    }
    // by convention, always return *this
    return *this;
}

CGetaddrinfo::~CGetaddrinfo() {
    TRACE("Destruct upnplib::CGetaddrinfo()\n");
    ::freeaddrinfo(m_res);
    m_res = nullptr;
}

addrinfo* CGetaddrinfo::copy_addrinfo(const CGetaddrinfo& other) {
    TRACE("Executing upnplib::CGetaddrinfo::copy_addrinfo()\n");

    // Get name information from the other address.
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    int ret = ::getnameinfo(other.m_res->ai_addr, other.m_res->ai_addrlen, hbuf,
                            sizeof(hbuf), sbuf, sizeof(sbuf),
                            NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        throw std::runtime_error(
            "ERROR! Failed to get name information from address: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"");
    }

    // Get new address information with values from the other address.
    addrinfo hints{};
    hints.ai_family = other.m_res->ai_family;
    hints.ai_socktype = other.m_res->ai_socktype;
    hints.ai_protocol = other.m_res->ai_protocol;
    hints.ai_flags = other.m_res->ai_flags;

    addrinfo* new_res{nullptr};

    ret = ::getaddrinfo(hbuf, sbuf, &hints, &new_res);
    if (ret != 0) {
        throw std::runtime_error(
            "ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"");
    }

    return new_res;
}

addrinfo* CGetaddrinfo::operator->() const {
    if (m_res == nullptr) {
        throw std::logic_error("ERROR! No address information available, must "
                               "be requested beforehand.");
    }
    return m_res;
}

} // namespace upnplib
