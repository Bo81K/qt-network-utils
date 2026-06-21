cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(qfile_logger
    src/qfile_logger.cpp
    include/qfile_logger.h
)

target_include_directories(qfile_logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(qfile_logger
    PUBLIC
        Qt6::Core
)

# Алиас для использования в подпроектах
add_library(QtNetworkUtils::FileLogger ALIAS qfile_logger)
