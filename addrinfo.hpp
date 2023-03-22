#ifndef UPNPLIB_INCLUDE_ADDRINFO_HPP
#define UPNPLIB_INCLUDE_ADDRINFO_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-23

#include "port_sock.hpp"
#include <string>

namespace upnplib {

// Provide C style addrinfo as class and wrap its system calls
// -----------------------------------------------------------
class CAddrinfo {
  public:
    // Default constructor to provide an empty object.
    CAddrinfo();

    // Constructor for getting an address information.
    CAddrinfo(const std::string& a_node, const std::string& a_service,
              const int a_family = AF_UNSPEC, const int a_socktype = 0,
              const int a_flags = 0, const int protocol = 0);

    // Rule of three: we need a copy constructor and a copy assignment operator.
    // Reference:
    // https://en.wikipedia.org/wiki/Rule_of_three_(C%2B%2B_programming)
    //
    // copy constructor:
    // Example: CAddrinfo ai2 = ai1; // ai1 is an instantiated valid object,
    // or       CAddrinfo ai2{ai1};
    CAddrinfo(const CAddrinfo& other);
    //
    // copy assignment operator:
    // Example: ai2 = ai1; // ai1 and ai2 are instantiated valid objects.
    CAddrinfo& operator=(const CAddrinfo& other);

    virtual ~CAddrinfo();

    // Reference: https://stackoverflow.com/a/8782794/5014688
    addrinfo* operator->() const;

  private:
    // This pointer is the reason why we need a copy constructor and a copy
    // assignment operator.
    addrinfo* m_res{nullptr};

    // Cache the hints that are given by the user, so we can always get
    // identical address information from the operating system.
    std::string m_node;
    std::string m_service;
    addrinfo m_hints{};

    // This is a helper method that gets a new address information. Because we
    // always use the same cached hints we also get the same information with
    // new allocated memory. We cannot just copy the structure that m_res
    // pointed to (*m_res = *other.m_res;). Copy works but MS Windows failed to
    // destruct it with freeaddrinfo(m_res);. It throws an exception "A
    // non-recoverable error occurred during a database lookup.". So we have to
    // go the hard way with getaddrinfo() and free it with freeaddrinfo(),
    addrinfo* get_new_addrinfo();
};

} // namespace upnplib

#endif // UPNPLIB_INCLUDE_ADDRINFO_HPP
