#ifndef UPNPLIB_INCLUDE_ADDRINFO_HPP
#define UPNPLIB_INCLUDE_ADDRINFO_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-30

#include "port_sock.hpp"
#include <string>

namespace upnplib {

// Provide C style addrinfo as class and wrap its system calls
// -----------------------------------------------------------
class CAddrinfo {
  public:
    // Constructor for getting an address information.
    CAddrinfo(const std::string& a_node, const std::string& a_service,
              const int a_family = AF_UNSPEC, const int a_socktype = 0,
              const int a_flags = 0, const int protocol = 0);

    // Rule of three: we need a copy constructor and a copy assignment operator.
    // Reference:
    // [What is The Rule of Three?](https://stackoverflow.com/q/4172722/5014688)
    //
    // copy constructor:
    // Example: CAddrinfo ai2 = ai1; // ai1 is an instantiated valid object,
    // or       CAddrinfo ai2{ai1};
    CAddrinfo(const CAddrinfo& that);
    //
    // copy assignment operator:
    // Provides strong exception guarantee.
    // Example: ai2 = ai1; // ai1 and ai2 are instantiated valid objects.
    CAddrinfo& operator=(CAddrinfo that);

    virtual ~CAddrinfo();

    // This is to have read access to members of the addrinfo structure,
    // Example: CAddrinfo ai(..); if(ai->family == AF_INET6) {..};
    // Reference: https://stackoverflow.com/a/8782794/5014688
    addrinfo* operator->() const;

    // Getter for address string
    std::string addr_str() const;

    // Getter for port number
    uint16_t port() const;

  private:
    // This pointer is the reason why we need a copy constructor and a copy
    // assignment operator.
    addrinfo* m_res{nullptr};

    // Cache the hints that are given by the user, so we can always get
    // identical address information from the operating system.
    std::string m_node;
    std::string m_service;
    addrinfo m_hints{};

    // This is a helper method that gets a new address information from the
    // operating system. Because we always use the same cached hints we also get
    // the same information with new allocated memory. We cannot just copy the
    // structure that m_res pointed to (*m_res = *that.m_res;). Copy works but
    // MS Windows failed to destruct it with freeaddrinfo(m_res);. It throws an
    // exception "A non-recoverable error occurred during a database lookup.".
    // Seems there are also pointer within the addrinfo structure that are not
    // deeply copied. So we have to go the hard way with getaddrinfo() and free
    // it with freeaddrinfo(),
    // Provides strong exception guarantee.
    addrinfo* get_new_addrinfo();
};

} // namespace upnplib

#endif // UPNPLIB_INCLUDE_ADDRINFO_HPP
