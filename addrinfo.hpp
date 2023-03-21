#ifndef UPNPLIB_INCLUDE_ADDRINFO_HPP
#define UPNPLIB_INCLUDE_ADDRINFO_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-21

#include "port_sock.hpp"

namespace upnplib {

// Provide C style addrinfo as class and wrap its system calls
// -----------------------------------------------------------
class CAddrinfo {
  public:
    // Default constructor to provide an empty object.
    CAddrinfo();

    // Constructor with getting an address information.
    CAddrinfo(const char* node, const char* service,
              const struct addrinfo* hints);

    // We need a copy constructor and a copy assignment operator.
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

    // This is a helper method that copies the addrinfo structure. We cannot
    // just copy the structure that m_res pointed to (*m_res = *other.m_res;).
    // Copy works but MS Windows failed to destruct it with
    // freeaddrinfo(m_res);. It throws an exception "A non-recoverable error
    // occurred during a database lookup.". So we have to go the hard way: get
    // name information from the other address and create a new one with
    // getaddrinfo() with that values.
    addrinfo* copy_addrinfo(const CAddrinfo& other);
};

} // namespace upnplib

#endif // UPNPLIB_INCLUDE_ADDRINFO_HPP
