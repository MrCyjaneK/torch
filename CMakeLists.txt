cmake_minimum_required(VERSION 3.10)
project(torch)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(TOR_BUILD_DIR "" CACHE PATH "Path to tor build directory")

if(NOT TOR_BUILD_DIR)
    message(FATAL_ERROR "You must set TOR_BUILD_DIR to tor build directory")
endif()

add_library(torch SHARED torch.cpp)

target_include_directories(torch
    PRIVATE
        ${TOR_BUILD_DIR}/include
)

target_link_libraries(torch
    PRIVATE
        ${TOR_BUILD_DIR}/lib/libtor.a
        ${TOR_BUILD_DIR}/lib/libevent.a
        ${TOR_BUILD_DIR}/lib/libz.a
        ${TOR_BUILD_DIR}/lib/libcrypto.a
        ${TOR_BUILD_DIR}/lib/libssl.a
        Threads::Threads
)

install(TARGETS torch
        LIBRARY DESTINATION lib)

install(FILES torch.h
        DESTINATION include)
