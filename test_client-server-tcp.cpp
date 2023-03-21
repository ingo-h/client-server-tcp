// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-21

#include "client-tcp.hpp"
#include "server-tcp.hpp"
#include "addrinfo.hpp"
#include "port.hpp"
#include <gtest/gtest.h>
#include <thread>

using upnplib::CServerTCP;

namespace upnplib {

TEST(GetaddrinfoTestSuite, class_getaddrinfo) {
    addrinfo hints{};
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    // Get no address information
    CAddrinfo ai1;
    // "ERROR! No address information available, must be requested beforehand.";
    EXPECT_THROW({ [[maybe_unused]] int dummy = ai1->ai_family; },
                 std::logic_error);

    // Get valid address information
    CAddrinfo ai2("localhost", "54321", &hints);

    EXPECT_EQ(ai2->ai_family, AF_INET6);
    EXPECT_EQ(ai2->ai_socktype, SOCK_STREAM);
    // Different on platforms: Ubuntu & MacOS return 6, win32 returns 0
    // EXPECT_EQ(ai2->ai_protocol, 6);
    // Different on platforms: Ubuntu returns 1025, MacOS & win32 return 0
    // EXPECT_EQ(ai2->ai_flags, 1025);
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
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    CAddrinfo ai4("localhost", "51234", &hints);

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

} // namespace upnplib


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
    constexpr int delay{50}; // polling delay in microseconds
    constexpr int limit{10}; // limit of polling retries
    int i;
    for (i = 0; i < limit; i++) {
        if (tls_svr->ready(delay))
            break;
    }
    if (i >= limit) {
        std::clog << "ERROR! ready loop for 'simpleTLSserver' thread called "
                  << i << " times. Check for deadlock.\n";
        std::exit(EXIT_FAILURE);
    }

    // Here we run the gtests as usual.
    ::testing::InitGoogleTest(&argc, argv);
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
