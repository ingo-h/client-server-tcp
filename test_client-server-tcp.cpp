// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-04-11

#include "client-tcp.hpp"
#include "server-tcp.hpp"
#include "addrinfo.hpp"
#include "gmock/gmock.h"
#include <thread>
#include <cstring>

using testing::HasSubstr;
using testing::StartsWith;
using testing::ThrowsMessage;


namespace upnplib {

TEST(SocketTestSuite, get_socket_successful) {
    WINSOCK_INIT_P

    // Test Unit
    CSocket sock(AF_INET6, SOCK_STREAM);

    EXPECT_THAT([&sock]() { sock.get_port(); },
                ThrowsMessage<std::runtime_error>(
                    "ERROR! Failed to get socket port number: \"not bound to "
                    "an address\""));
    EXPECT_EQ(sock.get_sockerr(), 0);
    EXPECT_FALSE(sock.is_reuse_addr());
    EXPECT_FALSE(sock.is_v6only());
    EXPECT_FALSE(sock.is_bind());
    EXPECT_FALSE(sock.is_listen());
}

TEST(SocketTestSuite, move_socket_successful) {
    WINSOCK_INIT_P

    // Provide a socket object
    CSocket sock1(AF_INET, SOCK_STREAM);
    SOCKET old_fd_sock1 = sock1;

    // Get local interface address. Socket type (SOCK_STREAM) must be the same
    // as that from the socket.
    CAddrinfo ai("", "50009", AF_INET, SOCK_STREAM,
                 AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    // Bind address to the socket.
    ASSERT_NO_THROW(sock1.bind(ai));

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Test Unit, move sock1 to a new sock2
    // CSocket sock2 = sock1; // This does not compile because it needs a copy
    //                           constructor that isn't available. We restrict
    //                           to move only.
    // This moves the socket file descriptor.
    CSocket sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the new object.
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see later test.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.get_port(), 50009);
    EXPECT_EQ(sock2.get_sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    EXPECT_FALSE(sock2.is_v6only());
    EXPECT_TRUE(sock2.is_bind());
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, assign_socket_successful) {
    WINSOCK_INIT_P

    // Get local interface address. Socket type (SOCK_STREAM) must be the same
    // as that from the socket.
    CAddrinfo ai("", "50010", AF_INET6, SOCK_STREAM,
                 AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    // Provide first of two socket objects.
    CSocket sock1(AF_INET6, SOCK_STREAM);
    SOCKET old_fd_sock1 = sock1;

    // Bind local interface address to the socket.
    ASSERT_NO_THROW(sock1.bind(ai));

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock1.listen());

    // Provide second empty socket.
    CSocket sock2;

    // Test Unit. We can only move. Copy a socket resource is not useful.
    sock2 = std::move(sock1);

    // The socket file descriptor has been moved to the destination object.
    EXPECT_EQ((SOCKET)sock2, old_fd_sock1);
    // The old object has now an invalid socket file descriptor.
    EXPECT_EQ((SOCKET)sock1, INVALID_SOCKET);

    // Tests of socket object (sock1) with INVALID_SOCKET see later test.
    // Check if new socket is valid.
    EXPECT_EQ(sock2.get_port(), 50010);
    EXPECT_EQ(sock2.get_sockerr(), 0);
    EXPECT_FALSE(sock2.is_reuse_addr());
    EXPECT_FALSE(sock2.is_v6only());
    EXPECT_TRUE(sock2.is_bind());
    EXPECT_TRUE(sock2.is_listen());
}

TEST(SocketTestSuite, object_with_invalid_socket_fd) {
    WINSOCK_INIT_P

    // Test Unit
    CSocket sock;
    EXPECT_EQ((SOCKET)sock, INVALID_SOCKET);

    // All getter from an INVALID_SOCKET throw an exception.
    EXPECT_THAT(
        [&sock]() { sock.get_port(); },
        ThrowsMessage<std::runtime_error>("ERROR! Failed to get socket port "
                                          "number: \"Bad file descriptor\""));
    EXPECT_THAT([&sock]() { sock.get_sockerr(); },
                ThrowsMessage<std::runtime_error>(StartsWith(
                    "ERROR! Failed to get socket option SO_ERROR: ")));
    EXPECT_THAT([&sock]() { sock.is_reuse_addr(); },
                ThrowsMessage<std::runtime_error>(StartsWith(
                    "ERROR! Failed to get socket option SO_REUSEADDR: ")));
    EXPECT_THAT([&sock]() { sock.is_v6only(); },
                ThrowsMessage<std::runtime_error>(
                    "ERROR! Failed to get socket option 'is_v6only': \"Bad "
                    "file descriptor\""));
    EXPECT_THAT([&sock]() { sock.is_bind(); },
                ThrowsMessage<std::runtime_error>(
                    "ERROR! Failed to get socket option 'is_bind': \"Bad file "
                    "descriptor\""));
    EXPECT_THAT([&sock]() { sock.is_listen(); },
                ThrowsMessage<std::runtime_error>(
                    "ERROR! Failed to get socket option "
                    "'is_Listen': \"Bad file descriptor\""));
}

TEST(SocketTestSuite, set_bind) {
    WINSOCK_INIT_P

    // Get local interface address.
    const CAddrinfo ai("", "50012", AF_INET6, SOCK_STREAM,
                       AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    // Test Unit.
    // This binds the local address. You will get an error "address already in
    // use" if you try to bind to it again in this test.
    CSocket sock(AF_INET6, SOCK_STREAM);
    ASSERT_NO_THROW(sock.bind(ai));
    EXPECT_EQ(sock.get_port(), 50012); // This tests the binding

    // Test Unit. Binding an empty socket object will fail.
    EXPECT_THAT(
        [&ai]() {
            CSocket sock;
            sock.bind(ai);
        },
        ThrowsMessage<std::runtime_error>(
            StartsWith("ERROR! Failed to bind socket to an address:")));

    // Test Unit. Binding with a different AF_INET will fail.
    EXPECT_THAT(
        [&ai]() {
            CSocket sock(AF_INET, SOCK_STREAM);
            sock.bind(ai);
        },
        ThrowsMessage<std::runtime_error>(
            StartsWith("ERROR! Failed to bind socket to an address:")));
}

TEST(SocketTestSuite, set_bind_with_different_socket_type) {
    WINSOCK_INIT_P

    // Get local interface address.
    const CAddrinfo ai("", "50011", AF_INET, SOCK_DGRAM,
                       AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);

    // Test Unit. Binding with different SOCK_STREAM (not SOCK_DGRAM) will fail.
    EXPECT_THAT(
        [&ai]() {
            CSocket sock(AF_INET, SOCK_STREAM);
            sock.bind(ai);
        },
        ThrowsMessage<std::runtime_error>(StartsWith(
            "ERROR! Failed to bind socket to an address: \"socket type of "
            "address (")));
}

TEST(SocketTestSuite, set_wrong_arguments) {
    // Test Unit. Set wrong address family.
    EXPECT_THAT([]() { CSocket sock(-1, SOCK_STREAM); },
                ThrowsMessage<std::runtime_error>(
                    StartsWith("ERROR! Failed to create socket: ")));

    // Test Unit. Set wrong socket type.
    EXPECT_THAT([]() { CSocket sock(AF_INET6, -1); },
                ThrowsMessage<std::runtime_error>(
                    StartsWith("ERROR! Failed to create socket: ")));
}

#if 0
// This has already be done as part of other tests.
TEST(SocketTestSuite, listen_successful) {
    WINSOCK_INIT_P

    // Get socket
    CSocket sock(AF_INET6, SOCK_STREAM);

    // Get local interface address info and bind it to the socket.
    CAddrinfo ai("", "50008", AF_INET6, SOCK_STREAM,
                 AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV);
    ASSERT_NO_THROW(sock.bind(ai));

    // Configure socket to listen so it will accept connections.
    ASSERT_NO_THROW(sock.listen());

    // Test Unit
    EXPECT_TRUE(sock.is_listen());
}
#endif

TEST(SocketTestSuite, check_af_inet6_v6only) {
    WINSOCK_INIT_P

    // Get socket
    CSocket sock(AF_INET6, SOCK_DGRAM);

    // Set option IPV6_V6ONLY. Of course this can only be done with AF_INET6.
    constexpr int so_option = 1; // Set IPV6_V6ONLY
    constexpr socklen_t optlen{sizeof(so_option)};
    ASSERT_EQ(::setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&so_option,
                           optlen),
              0);
    // Test Unit
    EXPECT_TRUE(sock.is_v6only());
}

TEST(SocketTestSuite, check_af_inet_v6only) {
    // This must always return false because on AF_INET we cannot have v6only.
    WINSOCK_INIT_P

    // Get socket
    CSocket sock(AF_INET, SOCK_STREAM);

    // Test Unit
    EXPECT_FALSE(sock.is_v6only());
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

TEST(ServerTcpTestSuite, listen_successful) {
    // Test Unit
    CServerTCP svrObj("4434", true);

    EXPECT_FALSE(svrObj.is_v6only());
    EXPECT_FALSE(svrObj.is_reuse_addr());
    EXPECT_EQ(svrObj.get_port(), 4434);
    EXPECT_TRUE(svrObj.is_listen());

    // TODO: Continue here with creating tests for CServerTCP::run(). I need to
    // mock socket functions.
}

} // namespace upnplib

int main(int argc, char** argv) {
    upnplib::CServerTCP* tcp_svr;
    std::thread* t1;

    // Instantiate TCP server object and catch errors on resource initialization
    // from its constructor.
    try {
        static upnplib::CServerTCP tcp_svrObj("4433", true);
        // To have the object available outside this block we need a pointer.
        // That's also why we declare it static.
        tcp_svr = &tcp_svrObj;
        // Run simple tcp server with a thread.
        static std::thread t1Obj(&upnplib::CServerTCP::run, tcp_svr);
        t1 = &t1Obj;
    } catch (const std::exception& e) {
        std::clog << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }

    // Wait until the tcp server is ready.
    constexpr int delay{90}; // polling delay in microseconds
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
