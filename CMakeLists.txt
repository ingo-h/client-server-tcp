# Copyright (C) 2023+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-01-12
# ~~~
# Configure and build with:
# cmake -S . -B build -D GOOGLETEST=ON [-D CMAKE_BUILD_TYPE=Debug]
# cmake --build build --config Debug|Release
# ~~~
cmake_minimum_required(VERSION 3.18)

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

#################################
# Options                       #
#################################
set (BUILD_SHARED_LIBS ON CACHE INTERNAL
    "This option belongs only to GOOGLETEST and build its libraries shared.")  # Implies FORCE

# Set the default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None, Debug, Release, RelWithDebInfo, or Release." FORCE)
    message(STATUS "No build type specified, defaulting to Release.")
endif()


#################################
# General build settings        #
#################################
# We have visibility support as described at https://gcc.gnu.org/wiki/Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_compile_definitions(
        # General define DEBUG if build type is "Debug". Manage setting NDEBUG
        # is done by cmake by default.
        $<$<CONFIG:Debug>:DEBUG>
        # There are traditional min/max macros on MS Windows which conflict
        # with the C++ std::min|max functions. Following switch disable the
        # macros.
        $<$<CXX_COMPILER_ID:MSVC>:NOMINMAX>
)
add_compile_options(
        # disable warning C4996: 'strdup'(or others): The POSIX name for this
        # item is deprecated. Instead, use the ISO C and C++ conformant name:
        # _strdup. See online help for details. (This is Microsoft specific, no
        # need to fix).
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996>
        # Enable lots of warnings and all warnings as errors
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wuninitialized>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wdeprecated>
        $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override> # Only valid for C++/ObjC++ but not for C
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fstack-protector-all>
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fstack-protector-strong>
        # Disable
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-dangling-else>
        #$<$<CXX_COMPILER_ID:GNU>:-Wno-format-truncation>
)
add_link_options(
        $<$<CXX_COMPILER_ID:MSVC>:/WX>   # all linker warnings as errors
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

    link_libraries(gtest gmock gtest_main gmock_main
    )

    message(CHECK_PASS "done")
endif()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")


#################################
# Build the Unit Tests          #
#################################
add_executable(test_client-server-tcp
    client-tcp.cpp
    server-tcp.cpp
    socket.cpp
    addrinfo.cpp
    test_client-server-tcp.cpp
)
#target_include_directories(test_client-server-tcp
#    PRIVATE
#)
#target_compile_definitions(test_client-server-tcp
#    PRIVATE
#        UPNPLIB_WITH_TRACE
#)
target_link_libraries(test_client-server-tcp
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:ws2_32> # winsock to support sockets
)
add_test(NAME ctest_client-server-tcp
         COMMAND test_client-server-tcp --gtest_shuffle
         WORKING_DIRECTORY ${PROJECT_OUTPUT_DIRECTORY}
)
