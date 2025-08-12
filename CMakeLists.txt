cmake_minimum_required(VERSION 3.10)
project(torch)

set(CMAKE_CXX_STANDARD 17)

set(TOR_BUILD_DIR "" CACHE PATH "Path to tor build directory")

if(NOT TOR_BUILD_DIR)
    message(FATAL_ERROR "You must set TOR_BUILD_DIR to tor build directory")
endif()

if(ANDROID)
    target_compile_options(torch PRIVATE -stdlib=libc++)
    target_link_options(torch PRIVATE -stdlib=libc++)
    set(extra_libs
        ${TOR_BUILD_DIR}/lib/libc++_static.a
        ${TOR_BUILD_DIR}/lib/libc++abi.a
    )
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
        ${extra_libs}
)

install(TARGETS torch
        LIBRARY DESTINATION lib)

install(FILES torch.h
        DESTINATION include)
