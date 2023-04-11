// Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-10

#include "socket.hpp"
#include "port.hpp"

#include <string>
#include <cstring>
#include <stdexcept>

namespace upnplib {

#ifdef _MSC_VER
CWSAStartup::CWSAStartup() {
    TRACE2(this, " Construct upnplib::CWSAStartup")
    WSADATA wsaData;
    int rc = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        throw std::runtime_error("ERROR! Failed to initialize Windows "
                                 "sockets: WSAStartup() returns " +
                                 std::to_string(rc));
    }
}

CWSAStartup::~CWSAStartup() {
    TRACE2(this, " Destruct upnplib::CWSAStartup")
    ::WSACleanup();
}
#endif // MSC_VER


static inline void throw_error(std::string errmsg) {
    // error number given by WSAGetLastError(), resp. contained in errno is
    // used to specify details of the error. It is important that these error
    // numbers hasn't been modified by executing other statements.
#ifdef _MSC_VER
    throw std::runtime_error(
        errmsg + " WSAGetLastError()=" + std::to_string(WSAGetLastError()));
#else
    throw std::runtime_error(errmsg + " errno(" + std::to_string(errno) +
                             ")=\"" + std::strerror(errno) + "\"");
#endif
}

// Wrap socket() system call
// -------------------------
// Constructor
CSocket::CSocket(int a_domain, int a_type, int a_protocol) {
    TRACE2(this, " Construct upnplib::CSocket()")

    // Check if we want an empty socket object.
    if (a_domain == 0 && a_type == 0 && a_protocol == 0)
        return;

    // Get socket file descriptor.
    SOCKET sfd = ::socket(a_domain, a_type, a_protocol);
    if (sfd == INVALID_SOCKET)
        throw_error("ERROR! Failed to create socket:");

    int so_option{0};
    constexpr socklen_t optlen{sizeof(so_option)};

    // Reset SO_REUSEADDR on all platforms if it should be set by default. This
    // is unclear on WIN32. See note below. If needed this option can be set
    // later with a setter.
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (::setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&so_option,
                     optlen) != 0) {
        CLOSE_SOCKET_P(sfd);
        throw_error("ERROR! Failed to set socket option SO_REUSEADDR:");
    }

#ifdef _MSC_VER
    // Set socket option SO_EXCLUSIVEADDRUSE on Microsoft Windows.
    // THIS IS AN IMPORTANT SECURITY ISSUE! Lock at
    // REF: [Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE]
    // (https://learn.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse#application-strategies)
    so_option = 1; // Set SO_EXCLUSIVEADDRUSE
    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (::setsockopt(sfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&so_option,
                     optlen) != 0) {
        CLOSE_SOCKET_P(sfd);
        throw_error("ERROR! Failed to set socket option SO_EXCLUSIVEADDRUSE:");
    }
#endif

    // Set socket option IPV6_V6ONLY to false, means allowing IPv4 and IPv6.
    if (a_domain == AF_INET6) {
        so_option = 0;
        // Type cast (char*)&so_option is needed for Microsoft Windows.
        if (::setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&so_option,
                         optlen) != 0) {
            CLOSE_SOCKET_P(sfd);
            throw_error("ERROR! Failed to set socket option IPV6_V6ONLY:");
        }
    }

    // Store socket file descriptor and settings
    m_sfd = sfd;
    m_af = a_domain;
}

// Move constructor
CSocket::CSocket(CSocket&& that) {
    TRACE2(this, " Construct move upnplib::CSocket()")
    m_listen = that.m_listen;
    that.m_listen = false;
    m_bound = that.m_bound;
    that.m_bound = false;
    m_af = that.m_af;
    that.m_af = -1;
    m_sfd = that.m_sfd;
    that.m_sfd = INVALID_SOCKET;
}

// Assignment operator (parameter as value)
CSocket& CSocket::operator=(CSocket that) {
    TRACE2(this, " Executing upnplib::CSocket::operator=()")
    std::swap(m_sfd, that.m_sfd);
    std::swap(m_listen, that.m_listen);
    std::swap(m_bound, that.m_bound);
    std::swap(m_af, that.m_af);
    return *this;
}

// Destructor
CSocket::~CSocket() {
    TRACE2(this, " Destruct upnplib::CSocket()")
    CLOSE_SOCKET_P(m_sfd);
}

// Get the raw socket file descriptor
CSocket::operator SOCKET&() const {
    // There is no problem with cast here. We cast to const so we can only read.
    return const_cast<SOCKET&>(m_sfd);
}

#if 0
void CSocket::set_reuse_addr(bool a_reuse) {
    // Set socket option SO_REUSEADDR on other platforms.
    // --------------------------------------------------
    // REF: [How do SO_REUSEADDR and SO_REUSEPORT differ?]
    // (https://stackoverflow.com/a/14388707/5014688)
    so_option = a_reuse_addr ? 1 : 0;
    // Type cast (char*)&so_reuseaddr is needed for Microsoft Windows.
    if (::setsockopt(m_listen_sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&so_option,
                     optlen) != 0)
        throw_error(
            "[Server] ERROR! Failed to set socket option SO_REUSEADDR:");
}
#endif

// Setter: bind socket to local address
// REF: [Bind: Address Already in Use]
// (https://hea-www.harvard.edu/~fine/Tech/addrinuse.html)
void CSocket::bind(const CAddrinfo& ai) {
    TRACE2(this, " Executing upnplib::CSocket::bind()")
    // std::lock_guard<std::mutex> guard(m_listen_mutex);

    int so_option{-1};
    socklen_t optlen{sizeof(so_option)}; // May be modified
    if (::getsockopt(m_sfd, SOL_SOCKET, SO_TYPE, (char*)&so_option, &optlen) ==
        SOCKET_ERROR)
        throw_error("ERROR! Failed to bind socket to an address:");

    if (ai->ai_socktype != so_option)
        throw std::runtime_error(
            "ERROR! Failed to bind socket to an address: "
            "\"socket type of address does not match socket type\"");

    if (::bind(m_sfd, ai->ai_addr, ai->ai_addrlen) == SOCKET_ERROR)
        throw_error("ERROR! Failed to bind socket to an address:");

    m_bound = true;
}

// Setter: set socket to listen
void CSocket::listen() {
    TRACE2(this, " Executing upnplib::CSocket::listen()")
    // std::lock_guard<std::mutex> guard(m_listen_mutex);

    // Second argument backlog (maximum length of the queue for pending
    // connections) is hard coded set to 1 for now.
    if (::listen(m_sfd, 1) != 0)
        throw_error("ERROR! Failed to set socket to listen:");

    m_listen = true;
}

// Getter
uint16_t CSocket::get_port() const {
    TRACE2(this, " Executing upnplib::CSocket::get_port()")
    if (m_sfd == INVALID_SOCKET)
        throw std::runtime_error("ERROR! Failed to get socket port number: "
                                 "\"Bad file descriptor\"");
    if (!this->is_bind())
        throw std::runtime_error("ERROR! Failed to get socket port number: "
                                 "\"not bound to an address\"");
    // Get port number
    sockaddr_storage ss{};
    socklen_t len = sizeof(ss); // May be modified
    if (::getsockname(m_sfd, (sockaddr*)&ss, &len) != 0)
        throw_error("ERROR! Failed to get socket port number:");

    return ntohs(((sockaddr_in6*)&ss)->sin6_port);
}

int CSocket::get_sockerr() const {
    TRACE2(this, " Executing upnplib::CSocket::get_sockerr()")
    return this->getsockopt_int(SOL_SOCKET, SO_ERROR, "SO_ERROR");
}

bool CSocket::is_reuse_addr() const {
    TRACE2(this, " Executing upnplib::CSocket::is_reuse_addr()")
    return this->getsockopt_int(SOL_SOCKET, SO_REUSEADDR, "SO_REUSEADDR");
}

bool CSocket::is_v6only() const {
    TRACE2(this, " Executing upnplib::CSocket::is_v6only()")
    if (m_sfd == INVALID_SOCKET)
        throw std::runtime_error("ERROR! Failed to get socket option "
                                 "'is_v6only': \"Bad file descriptor\"");
    // We can have v6only with AF_INET6. Otherwise always false is returned.
    return (m_af == AF_INET6)
               ? this->getsockopt_int(IPPROTO_IPV6, IPV6_V6ONLY, "IPV6_V6ONLY")
               : false;
}

bool CSocket::is_bind() const {
    TRACE2(this, " Executing upnplib::CSocket::is_bind()")
    if (m_sfd == INVALID_SOCKET)
        throw std::runtime_error("ERROR! Failed to get socket option "
                                 "'is_bind': \"Bad file descriptor\"");
    return m_bound;
}

bool CSocket::is_listen() const {
    TRACE2(this, " Executing upnplib::CSocket::is_listen()")
    if (m_sfd == INVALID_SOCKET)
        throw std::runtime_error("ERROR! Failed to get socket option "
                                 "'is_Listen': \"Bad file descriptor\"");
    return m_listen;
}

int CSocket::getsockopt_int(int a_level, int a_optname,
                            const std::string& a_optname_str) const {
    TRACE2(this,
           " Executing upnplib::CSocket::getsockopt_int(), " + a_optname_str)
    int so_option{-1};
    socklen_t optlen{sizeof(so_option)}; // May be modified

    // Type cast (char*)&so_option is needed for Microsoft Windows.
    if (::getsockopt(m_sfd, a_level, a_optname, (char*)&so_option, &optlen) !=
        0)
        throw_error("ERROR! Failed to get socket option " + a_optname_str +
                    ":");

    return so_option;
}

} // namespace upnplib
