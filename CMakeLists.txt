cmake_minimum_required(VERSION 3.22)
project(SnorlaxNet C)
include(ExternalProject)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DSNORLAX_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DSNORLAX_DEBUG")

ExternalProject_Add(snorlax-std
    PREFIX          "${PROJECT_SOURCE_DIR}/lib/snorlax/std"
    TMP_DIR         "${PROJECT_SOURCE_DIR}/lib/snorlax/std/tmp"
    STAMP_DIR       "${PROJECT_SOURCE_DIR}/lib/snorlax/std/stamp"
    DOWNLOAD_DIR    "${PROJECT_SOURCE_DIR}/lib/snorlax/std/src"
    SOURCE_DIR      "${PROJECT_SOURCE_DIR}/lib/snorlax/std/src"
    BINARY_DIR      "${PROJECT_SOURCE_DIR}/lib/snorlax/std/build"
    INSTALL_DIR     "${PROJECT_SOURCE_DIR}/lib/snorlax/std/install"
    LOG_DIR         "${PROJECT_SOURCE_DIR}/lib/snorlax/std/log"
    GIT_REPOSITORY  git@github.com:SnorlaxBio/std.git
    GIT_TAG         main
    GIT_REMOTE_NAME origin
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug
                                       -S "${PROJECT_SOURCE_DIR}/lib/snorlax/std/src"
                                       -B "${PROJECT_SOURCE_DIR}/lib/snorlax/std/build"
                                       --install-prefix ${PROJECT_SOURCE_DIR}
    BUILD_COMMAND     ${CMAKE_COMMAND} --build "${PROJECT_SOURCE_DIR}/lib/snorlax/std/build"
                                       --target all)

ExternalProject_Add(snorlax-eva
    PREFIX          "${PROJECT_SOURCE_DIR}/lib/snorlax/eva"
    TMP_DIR         "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/tmp"
    STAMP_DIR       "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/stamp"
    DOWNLOAD_DIR    "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/src"
    SOURCE_DIR      "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/src"
    BINARY_DIR      "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/build"
    INSTALL_DIR     "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/install"
    LOG_DIR         "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/log"
    GIT_REPOSITORY  git@github.com:SnorlaxBio/eva.git
    GIT_TAG         main
    GIT_REMOTE_NAME origin
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug
                                        -S "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/src"
                                        -B "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/build"
                                        --install-prefix ${PROJECT_SOURCE_DIR}
    BUILD_COMMAND     ${CMAKE_COMMAND} --build "${PROJECT_SOURCE_DIR}/lib/snorlax/eva/build"
                                        --target all)

include_directories(src include)
link_directories(${PROJECT_SOURCE_DIR}/lib ${PROJECT_SOURCE_DIR}/build)

set(SNORLAX_NET_SOURCE      src/snorlax/network.c
                            src/snorlax/network/if.c
                            src/snorlax/network/ip/addr.c
                            src/snorlax/network/tun.c
                            src/snorlax/network/netlink.c
                            src/snorlax/network/netlink/message.c
                            src/snorlax/network/netlink/request.c)
set(SNORLAX_NET_HEADER      src/snorlax/network.h
                            src/snorlax/network/if.h
                            src/snorlax/network/ip/addr.h
                            src/snorlax/network/tun.h
                            src/snorlax/network/netlink.h
                            src/snorlax/network/netlink/message.h
                            src/snorlax/network/netlink/request.h)

add_library(snorlax-net             ${SNORLAX_NET_SOURCE}
                                    ${SNORLAX_NET_HEADER})
target_compile_options(snorlax-net  PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")

add_executable(snorlax-network-tun  ${SNORLAX_NET_SOURCE}
                                    ${SNORLAX_NET_HEADER}
                                    src/snorlax/network/tun/main.c)
target_compile_options(snorlax-network-tun PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
target_link_libraries(snorlax-network-tun libsnorlax-eva.a libsnorlax-std.a)

install(TARGETS snorlax-net ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/ DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
                  FILES_MATCHING PATTERN "*.h")