// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-21

#include "addrinfo.hpp"
#include "port.hpp"
#include <string>
#include <stdexcept>

namespace upnplib {

// Provide C style addrinfo as class and wrap its system calls
// -----------------------------------------------------------
// Default constructor to provide an empty object.
CAddrinfo::CAddrinfo() { TRACE("Construct default upnplib::CAddrinfo()\n"); }

// Constructor with getting an address information.
CAddrinfo::CAddrinfo(const char* node, const char* service,
                     const struct addrinfo* hints) {
    TRACE("Construct upnplib::CAddrinfo(..) with arguments\n");
    int ret = ::getaddrinfo(node, service, hints, &m_res);
    if (ret != 0) {
        throw std::runtime_error(
            "ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"");
    }
}

// Copy constructor
CAddrinfo::CAddrinfo(const CAddrinfo& other) {
    TRACE("Executing upnplib::CAddrinfo() copy constructor\n");
    m_res = this->copy_addrinfo(other);
}

// Copy assignment operator
CAddrinfo& CAddrinfo::operator=(const CAddrinfo& other) {
    TRACE("Executing upnplib::CAddrinfo::operator=\n");
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

CAddrinfo::~CAddrinfo() {
    TRACE("Destruct upnplib::CAddrinfo()\n");
    ::freeaddrinfo(m_res);
    m_res = nullptr;
}

addrinfo* CAddrinfo::copy_addrinfo(const CAddrinfo& other) {
    TRACE("Executing upnplib::CAddrinfo::copy_addrinfo()\n");

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

addrinfo* CAddrinfo::operator->() const {
    if (m_res == nullptr) {
        throw std::logic_error("ERROR! No address information available, must "
                               "be requested beforehand.");
    }
    return m_res;
}

} // namespace upnplib
