# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2023-03-15

# Configure and build with:
# cmake -S . -B build [-D CMAKE_BUILD_TYPE=Debug -D GOOGLETEST=ON]
# cmake --build build --config Debug|Release

cmake_minimum_required(VERSION 3.18)

# This is the common header included by all (sub)projects
#--------------------------------------------------------
# Version 3.18 is the version from the Debian 11 (bullseye) repository.

# This is a guard that you do not build direct in the source tree.
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    message ("In-source builds are not supported.")
    message ("Use a separate folder for building, for example:\n")
    message ("    cmake -S . -B build/")
    message ("    cmake --build build/\n")
    message ("Before that, remove the files already created:\n")
    message ("    rm -rf CMakeCache.txt CMakeFiles\n")
    message (FATAL_ERROR "")
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the project name and version
project(CLIENT_SERVER_TCP VERSION 0001
        DESCRIPTION "Develop simple threaded test client and server."
        HOMEPAGE_URL "https://github.com/ingo-h")

enable_testing()

set (BUILD_SHARED_LIBS ON CACHE INTERNAL
    "This option belongs only to GOOGLETEST and build its libraries shared.")  # Implies FORCE

# Set the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None, Debug, Release, RelWithDebInfo, or Release." FORCE)
    message(STATUS "No build type specified, defaulting to Release.")
endif()

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
)

#################################
# Targets                       #
#################################
# Set target output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
if(WIN32)
    if(CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        set(PROJECT_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    else()
        set(PROJECT_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
    endif()
else()
    set(PROJECT_OUTPUT_DIRECTORY "")
endif()
# Output with cmake option --log-level=DEBUG
message(DEBUG "  DEBUG: PROJECT_OUTPUT_DIRECTORY is: ${PROJECT_OUTPUT_DIRECTORY}")


#################################
# Googletest                    #
#################################
# Download, configure and build
if(GOOGLETEST)
    include(FetchContent)

    message(CHECK_START "Download and configuring GoogleTest")

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY    https://github.com/google/googletest.git
        GIT_TAG           origin/main
        GIT_SHALLOW       ON
    )
    # This caches googletest_SOURCE_DIR, googletest_BINARY_DIR and googletest_POPULATED.
    FetchContent_MakeAvailable(googletest)

    link_libraries(gtest gmock
    )

    message(CHECK_PASS "done")
endif()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")


#################################
# Build the Unit Tests          #
#################################
add_executable(test_client-server-tcp
    ./client-server-tcp.cpp
    ./test_client-server-tcp.cpp
)
#target_include_directories(test_client-server-tcp
#    PRIVATE
#)
target_compile_definitions(test_client-server-tcp
    PRIVATE
        UPNPLIB_WITH_TRACE
)
target_link_libraries(test_client-server-tcp
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:ws2_32> # winsock to support sockets
)
add_test(NAME ctest_client-server-tcp COMMAND test_client-server-tcp --gtest_shuffle
        WORKING_DIRECTORY ${PROJECT_OUTPUT_DIRECTORY}
)
