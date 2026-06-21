cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(ftp_upload main.cpp)

target_link_libraries(ftp_upload
    PRIVATE
        QtNetworkUtils::FtpClient
        QtNetworkUtils::FileLogger
        Qt6::Core
        Qt6::Network
)
