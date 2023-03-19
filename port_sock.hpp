#ifndef UPNPLIB_INCLUDE_PORT_SOCK_HPP
#define UPNPLIB_INCLUDE_PORT_SOCK_HPP
// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-20

// clang-format off

// Make sockets portable
// ---------------------
#ifdef _MSC_VER
  #include <fcntl.h>
  #include <winsock2.h>
  #include <iphlpapi.h> // must be after <winsock2.h>
  #include <ws2tcpip.h> // for getaddrinfo, socklen_t etc.

  // _MSC_VER has SOCKET defined but unsigned and not a file descriptor.
  #define sa_family_t ADDRESS_FAMILY
  #define CLOSE_SOCKET_P closesocket

  // For shutdown() send/receive on a socket there are other constant names.
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH

#else

  #include <sys/socket.h>
  #include <sys/select.h>
  #include <arpa/inet.h>
  #include <unistd.h> // Also needed here to use 'close()' for a socket.
  #include <netdb.h>  // for getaddrinfo etc.

  // This typedef makes the code slightly more WIN32 tolerant. On WIN32 systems,
  // SOCKET is unsigned and is not a file descriptor.
  typedef int SOCKET;
  // Posix has sa_family_t defined.
  #define CLOSE_SOCKET_P close

  // socket() returns INVALID_SOCKET on win32 and is unsigned.
  #define INVALID_SOCKET (-1)
  // some function returns SOCKET_ERROR on win32.
  #define SOCKET_ERROR (-1)
#endif

// clang-format on

namespace upnplib {

// Initialize and cleanup Microsoft Windows Sockets
// ------------------------------------------------
#ifdef _MSC_VER
class CWSAStartup {
  public:
    CWSAStartup();
    virtual ~CWSAStartup();
};
#define WINSOCK_INIT_P CWSAStartup winsock_init;
#else
#define WINSOCK_INIT_P
#endif


// Wrap socket() system call
// -------------------------
class CSocket {
  public:
    CSocket(int domain, int type, int protocol);
    virtual ~CSocket();

    // Get the socket, e.g.: CSocket sock; sfd = (SOCKET)sock;
    operator SOCKET() const;

  private:
    SOCKET fd{INVALID_SOCKET};
};


// Wrap getaddrinfo() system call
// ------------------------------
class CGetaddrinfo {
  public:
    // Default constructor to provide an empty object.
    CGetaddrinfo();

    // Constructor with getting an address information.
    CGetaddrinfo(const char* node, const char* service,
                 const struct addrinfo* hints);

    // We need a copy constructor and a copy assignment operator.
    // Reference:
    // https://en.wikipedia.org/wiki/Rule_of_three_(C%2B%2B_programming)
    //
    // copy constructor:
    // Example: CGetaddrinfo ai2 = ai1; // ai1 is an instantiated valid object,
    // or       CGetaddrinfo ai2{ai1};
    CGetaddrinfo(const CGetaddrinfo& other);
    //
    // copy assignment operator:
    // Example: ai2 = ai1; // ai1 and ai2 are instantiated valid objects.
    CGetaddrinfo& operator=(const CGetaddrinfo& other);

    virtual ~CGetaddrinfo();

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
    addrinfo* copy_addrinfo(const CGetaddrinfo& other);
};

} // namespace upnplib

#endif // UPNPLIB_INCLUDE_PORT_SOCK_HPP
