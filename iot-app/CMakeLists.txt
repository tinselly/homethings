
cmake_minimum_required(VERSION 3.20.0)

list(APPEND CONF_FILE
    ${CMAKE_SOURCE_DIR}/conf/debug.conf
    ${CMAKE_SOURCE_DIR}/conf/network.conf
    ${CMAKE_SOURCE_DIR}/conf/project.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    homethings
    VERSION 0.0.0
    LANGUAGES C CXX
)

add_subdirectory(src)
