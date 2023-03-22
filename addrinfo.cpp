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
CAddrinfo::CAddrinfo() { //
    TRACE2(this, " Construct default upnplib::CAddrinfo()\n");
}

// Constructor with getting an address information.
CAddrinfo::CAddrinfo(const std::string& a_node, const std::string& a_service,
                     const int a_family, const int a_socktype,
                     const int a_flags, const int a_protocol)
    : m_node(a_node),
      m_service(a_service), m_hints{a_flags, a_family, a_socktype, a_protocol,
                                    {},      nullptr,  nullptr,    nullptr} {

    TRACE2(this, " Construct upnplib::CAddrinfo(..) with arguments\n");

    // Get new address information from the operating system.
    m_res = get_new_addrinfo();
}

// Copy constructor
CAddrinfo::CAddrinfo(const CAddrinfo& other) {
    TRACE2(this, " Executing upnplib::CAddrinfo() copy constructor\n");
    m_node = other.m_node;
    m_service = other.m_service;
    m_hints = other.m_hints;
    m_res = this->get_new_addrinfo();
}

// Copy assignment operator
CAddrinfo& CAddrinfo::operator=(const CAddrinfo& other) {
    TRACE2(this, " Executing upnplib::CAddrinfo::operator=\n");
    if (this != &other) { // protect against invalid self-assignment
        // 1: allocate new memory and copy the elements
        m_node = other.m_node;
        m_service = other.m_service;
        m_hints = other.m_hints;
        // 2: deallocate memory of the target because it is an already
        // initialized object.
        ::freeaddrinfo(m_res);
        m_res = nullptr;
        // 3: assign the new memory to the object
        m_res = this->get_new_addrinfo();
    }
    // by convention, always return *this
    return *this;
}

CAddrinfo::~CAddrinfo() {
    TRACE2(this, " Destruct upnplib::CAddrinfo()\n");
    ::freeaddrinfo(m_res);
    m_res = nullptr;
}

addrinfo* CAddrinfo::get_new_addrinfo() {
    TRACE2(this, " Executing upnplib::CAddrinfo::get_new_addrinfo()\n");

    // Get new address information with cached hints. This should always return
    // the same address info.
    addrinfo* new_res{nullptr};

    int ret =
        ::getaddrinfo(m_node.c_str(), m_service.c_str(), &m_hints, &new_res);
    if (ret != 0) {
        throw std::runtime_error(
            "[" + std::to_string(__LINE__) +
            "] ERROR! Failed to get address information: errid(" +
            std::to_string(ret) + ")=\"" + ::gai_strerror(ret) + "\"");
    }

    // Different on platforms: Ubuntu & MacOS return protocol number, win32
    // returns 0. We just return what was requested by the user.
    new_res->ai_protocol = m_hints.ai_protocol;
    // Different on platforms: Ubuntu returns set flags, MacOS & win32 return 0.
    // We just return what was requested by the user.
    new_res->ai_flags = m_hints.ai_flags;

    return new_res;
}

addrinfo* CAddrinfo::operator->() const {
    if (m_res == nullptr) {
        throw std::logic_error(
            "[" + std::to_string(__LINE__) +
            "] ERROR! No address information available, must "
            "be requested beforehand.");
    }
    return m_res;
}

} // namespace upnplib
