// Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2023-03-17

#include "client-tcp.hpp"
#include "server-tcp.hpp"
#include "port.hpp"
#include <gtest/gtest.h>
#include <thread>

using upnplib::CServerTCP;

namespace upnplib {

TEST(EmptyTestSuite, empty_gtest) {
    //
}

} // namespace upnplib

int main(int argc, char** argv) {
    CServerTCP* tls_svr;
    std::thread* t1;

    // Instantiate simple TLS server object and catch errors on resource
    // initialization from its constructor.
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

    // Quit server
    upnplib::quit_server();

    t1->join();
    return gtest_rc;
}
