// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-01

#include "client-tcp.hpp"
#include "server-tcp.hpp"
#include "addrinfo.hpp"
#include "port.hpp"
#include "gmock/gmock.h"
#include <thread>

using testing::EndsWith;
using testing::HasSubstr;
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
    CSocket sock(AF_INET6, SOCK_STREAM);

    // Check if socket is valid
    EXPECT_NE((SOCKET)sock, INVALID_SOCKET);
    int errbuf{-0xAA};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, move_socket_successful) {
    WINSOCK_INIT_P

    // Provide a socket object
    CSocket sock1(AF_INET6, SOCK_STREAM);
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
    int errbuf{-0xAA55};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock2, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, assign_socket_successful) {
    WINSOCK_INIT_P

    // Provide two socket objects
    CSocket sock1(AF_INET6, SOCK_STREAM);
    SOCKET old_fd_sock1 = sock1;

    CSocket sock2;

    // Test Unit
    sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the destination object
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Check if new socket is valid
    int errbuf{-0xA5A5};
    socklen_t errbuflen{sizeof(errbuf)};
    EXPECT_EQ(
        getsockopt(sock2, SOL_SOCKET, SO_ERROR, (char*)&errbuf, &errbuflen), 0);
    EXPECT_EQ(errbuf, 0);
}

TEST(SocketTestSuite, set_wrong_address_family) {
    // Test Unit
    EXPECT_THAT([]() { CSocket sock(-1, SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    StartsWith("ERROR! Failed to create socket: ")));
}

TEST(AddrinfoTestSuite, get_successful) {
    // If node is not empty  AI_PASSIVE is ignored.
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
    // Returns what getaddrinfo() returns.
    EXPECT_EQ(ai1.addr_str(), "::1");
    EXPECT_EQ(ai1.port(), 50001);
}

TEST(AddrinfoTestSuite, get_passive_addressinfo) {
    // To get a passive address info, node must be empty otherwise flag
    // AI_PASSIVE is ignored.
    WINSOCK_INIT_P

    // Test Unit
    CAddrinfo ai1("", "50006", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    // wildcard address ipv4 = 0.0.0.0, ipv6 = ::/128
    EXPECT_EQ(ai1.addr_str(), "::");
    EXPECT_EQ(ai1.port(), 50006);
}

TEST(AddrinfoTestSuite, get_info_loopback_interface) {
    // To get info of the loopback interface node must be empty without
    // AI_PASSIVE flag set.
    WINSOCK_INIT_P

    // Test Unit
    CAddrinfo ai1("", "50007", AF_UNSPEC, SOCK_STREAM,
                  AI_NUMERICHOST | AI_NUMERICSERV);

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "::1");
    EXPECT_EQ(ai1.port(), 50007);
}

TEST(AddrinfoTestSuite, uninitilized_port_nummer) {
    // With a node but an empty service the returned port number in the address
    // structure remains uninitialized. It appears to be initialized to zero
    // nonetheless, but should not be relied upon.
    WINSOCK_INIT_P

    // Test Unit
    CAddrinfo ai1("::1", "", AF_INET6, SOCK_STREAM,
                  AI_NUMERICHOST | AI_NUMERICSERV);

    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "::1");
    EXPECT_EQ(ai1.port(), 0);
}

TEST(AddrinfoTestSuite, get_fails) {
    // Test Unit. Address family does not match the numeric host address.
    EXPECT_THAT(
        []() {
            CAddrinfo ai1("127.0.0.1", "50003", AF_INET6, SOCK_STREAM,
                          AI_NUMERICHOST | AI_NUMERICSERV);
        },
        // errid(-9)="Address family for hostname not supported"
        ThrowsMessage<std::runtime_error>(
            HasSubstr("ERROR! Failed to get address information: errid(")));
}

TEST(AddrinfoTestSuite, copy_successful) {
    // This tests the copy constructor.
    // Get valid address information.
    WINSOCK_INIT_P

    CAddrinfo ai1("127.0.0.1", "50002", AF_INET, SOCK_DGRAM,
                  AI_NUMERICHOST | AI_NUMERICSERV);

    // Test Unit
    { // scope for ai2
        CAddrinfo ai2 = ai1;

        EXPECT_EQ(ai2->ai_family, AF_INET);
        EXPECT_EQ(ai2->ai_socktype, SOCK_DGRAM);
        EXPECT_EQ(ai2->ai_protocol, 0);
        EXPECT_EQ(ai2->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
        EXPECT_EQ(ai2.addr_str(), "127.0.0.1");
        EXPECT_EQ(ai2.port(), 50002);
    } // End scope, ai2 will be destructed

    // Check if ai1 is still available.
    EXPECT_EQ(ai1->ai_family, AF_INET);
    EXPECT_EQ(ai1->ai_socktype, SOCK_DGRAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "127.0.0.1");
    EXPECT_EQ(ai1.port(), 50002);
}
/*
 * TEST(AddrinfoTestSuite, copy_fails) {
 *      This isn't possible because the test would be:
 *      CAddrinfo ai2 = ai1;
 *      and I cannot find a way to provide an invalid object ai1 that the
 *      compiler accepts.
 * }
 */
TEST(AddrinfoTestSuite, assign_other_object_successful) {
    // This tests the copy assignment operator.
    // Get two valid address informations.
    WINSOCK_INIT_P

    // With node != nullptr AI_PASSIVE is ignored.
    CAddrinfo ai1("::1", "50004", AF_INET6, SOCK_STREAM,
                  AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    CAddrinfo ai2("localhost", "50005", AF_INET, SOCK_DGRAM, AI_NUMERICSERV);

    // Test Unit
    ai2 = ai1;

    // Check ai2.
    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai2->ai_protocol, 0);
    EXPECT_EQ(ai2->ai_flags, AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai2.addr_str(), "::1");
    EXPECT_EQ(ai2.port(), 50004);

    // Check if ai1 is still available.
    EXPECT_EQ(ai1->ai_family, AF_INET6);
    EXPECT_EQ(ai1->ai_socktype, SOCK_STREAM);
    EXPECT_EQ(ai1->ai_protocol, 0);
    EXPECT_EQ(ai1->ai_flags, AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    EXPECT_EQ(ai1.addr_str(), "::1");
    EXPECT_EQ(ai1.port(), 50004);
}

} // namespace upnplib


// TODO: there is a problem with a reused address on the server.
int main(int argc, char** argv) {
    CServerTCP* tcp_svr;
    std::thread* t1;

    // Instantiate TCP server object and catch errors on resource initialization
    // from its constructor.
    try {
        static CServerTCP tcp_svrObj;
        // To have the object available outside this block we need a pointer.
        // That's also why we declare it static.
        tcp_svr = &tcp_svrObj;
        // Run simple tcp server with a thread.
        static std::thread t1Obj(&CServerTCP::run, tcp_svr);
        t1 = &t1Obj;
    } catch (const std::exception& e) {
        std::clog << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }

    // Wait until the tcp server is ready.
    constexpr int delay{60}; // polling delay in microseconds
    constexpr int limit{10}; // limit of polling retries
    int i;
    for (i = 0; i < limit; i++) {
        if (tcp_svr->ready(delay))
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


// int main(int argc, char** argv) {
//     testing::InitGoogleMock(&argc, argv);
//     return RUN_ALL_TESTS();
//     // #include "upnplib/gtest_main.inc"
// }
