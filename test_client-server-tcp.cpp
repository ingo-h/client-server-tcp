// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-29

#include "client-tcp.hpp"
#include "server-tcp.hpp"
#include "addrinfo.hpp"
#include "port.hpp"
#include "gmock/gmock.h"
#include <thread>

using testing::EndsWith;
using testing::StartsWith;
using testing::ThrowsMessage;
using upnplib::CServerTCP;


namespace upnplib {

TEST(SocketTestSuite, empty_socket_obj) {
    CSocket sock;
    EXPECT_EQ((SOCKET)sock, INVALID_SOCKET);
}

TEST(SocketTestSuite, get_successful) {
    WINSOCK_INIT_P

    // Test Unit
    CSocket sock;
    sock.set(AF_INET6, SOCK_STREAM, 0);

    // Check if socket is valid
    int errbuf{-1};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, change_socket_successful) {
    WINSOCK_INIT_P

    // Provide a socket object
    CSocket sock;
    sock.set(AF_INET, SOCK_DGRAM, 0);
    SOCKET old_sockfd = sock;

    // Test Unit
    sock.set(AF_INET6, SOCK_STREAM, 0);

    int errbuf{-2};
    socklen_t errbuflen{sizeof(errbuf)};

    // Check if old socket file descriptor is invalid
    EXPECT_NE(old_sockfd, (SOCKET)sock);
    EXPECT_EQ(getsockopt(old_sockfd, SOL_SOCKET, SO_ERROR, (char*)&errbuf,
                         &errbuflen),
              -1);
    EXPECT_EQ(errbuf, -2); // errbuf is unchanged

    // Check if new socket is valid
    EXPECT_EQ(
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, move_socket_successful) {
    WINSOCK_INIT_P

    // Provide a socket object
    CSocket sock1;
    sock1.set(AF_INET6, SOCK_STREAM, 0);
    SOCKET old_fd_sock1 = sock1;

    // Test Unit
    // CSocket sock2 = sock1; // This does not compile because it needs a copy
    //                           constructor that isn't available because we
    //                           restrict to move only.
    // This moves the socket file descriptor
    CSocket sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the new object
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Check if new socket is valid
    int errbuf{-2};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock2, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, assign_socket_successful) {
    WINSOCK_INIT_P

    // Provide two socket objects
    CSocket sock1;
    sock1.set(AF_INET6, SOCK_STREAM, 0);
    SOCKET old_fd_sock1 = sock1;

    CSocket sock2;
    sock2.set(AF_INET, SOCK_DGRAM, 0);

    // Test Unit
    sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the destination object
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Check if new socket is valid
    int errbuf{-2};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock2, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, wrong_address_family) {
    // Test Unit
    EXPECT_THAT(
        []() {
            CSocket sock;
            sock.set(-1, SOCK_STREAM, 0);
        },
        ThrowsMessage<std::runtime_error>(
            StartsWith("ERROR! Failed to create socket: ")));
}

#if 0
// TODO: Test next test without CWSAStartup()
TEST(AddrinfoTestSuite, addrinfo_get_successful) {
    WINSOCK_INIT_P

    // Test Unit
    CAddrinfo ai1("localhost", "50001", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICSERV);

    // Returns what getaddrinfo() returns.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    // Returns what getaddrinfo() returns.
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    // Different on platforms: Ubuntu & MacOS return 6, win32 returns 0.
    // We just return that what was requested by the user.
    EXPECT_EQ(ai1->ai_protocol, 0);
    // Different on platforms: Ubuntu returns 1025, MacOS & win32 return 0.
    // We just return that what was requested by the user.
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "::1");
    EXPECT_EQ(ai1.port(), 50001);
}

TEST(AddrinfoTestSuite, addrinfo_copy) {
    // This tests the copy constructor.
    // Get valid address information.
    CAddrinfo ai1("127.0.0.1", "50002", AF_INET, SOCK_DGRAM,
                  AI_NUMERICHOST | AI_NUMERICSERV);

    // Test Unit
    CAddrinfo ai2 = ai1;

    EXPECT_EQ(ai2->ai_family, AF_INET);
    EXPECT_EQ(ai2->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai2.addr_str(), "127.0.0.1");
    EXPECT_EQ(ai2.port(), 50002);

    // Check if ai1 is still available.
    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "127.0.0.1");
    EXPECT_EQ(ai1.port(), 50002);
}

TEST(AddrinfoTestSuite, addrinfo_successful) {
    CAddrinfo ai1;

    // Get no address information
    EXPECT_THAT(
        [&ai1] { [[maybe_unused]] int dummy = ai1->ai_family; },
        ThrowsMessage<std::logic_error>(EndsWith(
            "ERROR! No address information available, must be requested "
            "beforehand.")));

    // Get valid address information
    CAddrinfo ai2("localhost", "54321", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICSERV);

    // Returns what getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    // Returns what getaddrinfo() returns.
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    // Different on platforms: Ubuntu & MacOS return 6, win32 returns 0.
    // We return that what was given as argument.
    EXPECT_EQ(ai2->ai_protocol, 0);
    // Different on platforms: Ubuntu returns 1025, MacOS & win32 return 0.
    // We return that what was given as argument.
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICSERV);
    sockaddr_in6* sa6 = (sockaddr_in6*)ai2->ai_addr;
    char addrbuf[INET6_ADDRSTRLEN]{};
    inet_ntop(ai2->ai_family, sa6->sin6_addr.s6_addr, addrbuf, sizeof(addrbuf));
    EXPECT_STREQ(addrbuf, "::1");
    EXPECT_EQ(ntohs(sa6->sin6_port), 54321);

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    // Copy address information to a new object
    { // scope for the object
        CAddrinfo ai3 = ai2;

        EXPECT_EQ(getnameinfo(ai3->ai_addr, ai3->ai_addrlen, hbuf, sizeof(hbuf),
                              sbuf, sizeof(sbuf),
                              NI_NUMERICHOST | NI_NUMERICSERV),
                  0);
        EXPECT_STREQ(hbuf, "::1");
        EXPECT_STREQ(sbuf, "54321");
        EXPECT_EQ(ai3->ai_family, AF_INET6);
        EXPECT_EQ(ai3->ai_socktype, SOCK_STREAM);
    } // object will be destructed here

    // Copy address information to an existing object
    ai1 = ai2;

    EXPECT_EQ(getnameinfo(ai1->ai_addr, ai1->ai_addrlen, hbuf, sizeof(hbuf),
                          sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV),
              0);
    EXPECT_STREQ(hbuf, "::1");
    EXPECT_STREQ(sbuf, "54321");
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);

    // Copy another address
    CAddrinfo ai4("localhost", "51234", AF_INET, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICSERV);

    ai1 = ai4;

    EXPECT_EQ(getnameinfo(ai1->ai_addr, ai1->ai_addrlen, hbuf, sizeof(hbuf),
                          sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV),
              0);
    EXPECT_STREQ(hbuf, "127.0.0.1");
    EXPECT_STREQ(sbuf, "51234");
    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);

    // ai2 is still available
    EXPECT_EQ(getnameinfo(ai2->ai_addr, ai2->ai_addrlen, hbuf, sizeof(hbuf),
                          sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV),
              0);
    EXPECT_STREQ(hbuf, "::1");
    EXPECT_STREQ(sbuf, "54321");
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
}
#endif

} // namespace upnplib


int main(int argc, char** argv) {
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
    // #include "upnplib/gtest_main.inc"
}


#if 0
int main(int argc, char** argv) {
    CServerTCP* tls_svr;
    std::thread* t1;

    // Instantiate TCP server object and catch errors on resource initialization
    // from its constructor.
    try {
        static CServerTCP tls_svrObj;
        // To have the object available outside this block we need a pointer.
        // That's also why we declare it static.
        tls_svr = &tls_svrObj;
        // Run simple TLS server with a thread.
        static std::thread t1Obj(&CServerTCP::run, tls_svr);
        t1 = &t1Obj;
    } catch (const std::exception& e) {
        std::clog << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }

    // Wait until the TLS server is ready.
    constexpr int delay{60}; // polling delay in microseconds
    constexpr int limit{10}; // limit of polling retries
    int i;
    for (i = 0; i < limit; i++) {
        if (tls_svr->ready(delay))
            break;
    }
    if (i >= limit) {
        std::clog << "ERROR! ready loop for upnplib::CServerTCP thread called "
                  << i << " times. Check for deadlock.\n";
        std::exit(EXIT_FAILURE);
    }

    // Here we run the gtests as usual.
    ::testing::InitGoogleMock(&argc, argv);
    int gtest_rc = RUN_ALL_TESTS();

    try {
        // Quit server
        upnplib::quit_server();
    } catch (const std::exception& e) {
        std::clog << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }

    t1->join();
    return gtest_rc;
}
#endif
