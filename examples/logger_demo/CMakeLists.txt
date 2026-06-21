cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(logger_demo main.cpp)

target_link_libraries(logger_demo
    PRIVATE
        QtNetworkUtils::FileLogger
        Qt6::Core
)
