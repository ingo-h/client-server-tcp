#ifndef SERVER_TCP_HPP
#define SERVER_TCP_HPP
// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-10

#include "socket.hpp"
#include <string>

namespace upnplib {

// Simple TCP Server
// =================
// Inspired by https://www.geeksforgeeks.org/socket-programming-cc
// Inspired by https://wiki.openssl.org/index.php/Simple_TLS_Server and
// https://www.ibm.com/docs/en/ztpf/1.1.0.15?topic=examples-server-application-ssl-code

class CServerTCP {
  public:
    CServerTCP(const std::string& a_port, const bool a_reuse_addr = false);
    virtual ~CServerTCP();

    // Run the server to accept messages. This method can be run in its own
    // thread.
    virtual void run();

    // Return if the server is ready to run.
    bool ready(int delay) const;

    // Getter for the v6only flag:
    // true  = server supports only IPv6 protocol stack
    // false = server supports IPv4 and IPv6 dual protocol stack
    bool is_v6only() const;

    // Getter for the reuse address flag:
    // true  = server will immediately reuse a freed local network address
    // false = server will wait until TIME_WAIT has expired before reuse
    bool is_reuse_addr() const;

    // Getter for the listening port the server is bound
    // If you get a port number > 0 then ::bind() has been called.
    uint16_t get_port() const;

    // Getter if the server is set to listening (will accept connections):
    // true  = server is set to listen
    // false = server is not set to listen
    bool is_listen() const;

  private:
    WINSOCK_INIT_P
    bool m_ready{false};
    CSocket m_listen_sfd;
};

} // namespace upnplib

#endif // SERVER_TCP_HPP
