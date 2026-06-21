cmake_minimum_required(VERSION 3.16)
project(QtNetworkUtils 
    VERSION 1.0.0 
    DESCRIPTION "Reusable Qt network utilities"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Опции сборки
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Добавляем библиотеки
add_subdirectory(libs/file_logger)
add_subdirectory(libs/ftp_client)

# Примеры (опционально)
if(BUILD_EXAMPLES)
    add_subdirectory(examples/logger_demo)
    add_subdirectory(examples/ftp_upload)
endif()
