cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(qftp_client
    src/qftp_client.cpp
    include/qftp_client.h
)

target_include_directories(qftp_client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(qftp_client
    PUBLIC
        Qt6::Core
        Qt6::Network
)

# Алиас для использования в подпроектах
add_library(QtNetworkUtils::FtpClient ALIAS qftp_client)
