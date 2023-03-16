#ifndef SERVER_TCP_HPP
#define SERVER_TCP_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-17

#include "port_sock.hpp"

namespace upnplib {

// Simple TCP Server
// =================
// Inspired by https://www.geeksforgeeks.org/socket-programming-cc
// Inspired by https://wiki.openssl.org/index.php/Simple_TLS_Server and
// https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=examples-server-application-ssl-code

class CServerTCP {
  public:
    CServerTCP();
    virtual ~CServerTCP();
    virtual void run();
    bool ready(int delay) const;

  private:
    bool m_ready{false};
    SOCKET m_listen_sfd{INVALID_SOCKET};
    addrinfo* m_ai;
};

} // namespace upnplib

#endif // SERVER_TCP_HPP
