#ifndef SIMPLE_TLS_SERVER_HPP
#define SIMPLE_TLS_SERVER_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-13

#include "port.hpp"
#include "port_sock.hpp"

namespace compa {

// Simple TLS Server
// =================
// Inspired by https://wiki.openssl.org/index.php/Simple_TLS_Server and
// https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=examples-server-application-ssl-code

class CSimpleTLSServer {
  public:
    CSimpleTLSServer();
    virtual ~CSimpleTLSServer();
    virtual void run();
    bool ready(int delay) const;

  private:
    bool m_ready{false};
    SOCKET m_sock{INVALID_SOCKET};
    addrinfo* m_ai;
};

} // namespace compa

#endif // SIMPLE_TLS_SERVER_HPP
