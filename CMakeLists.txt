cmake_minimum_required(VERSION 3.0.0)
project(lock-free-search-tree VERSION 0.1.0)

include(CTest)
enable_testing()

set(SOURCE
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/treap.cpp
)

add_executable(lock-free-search-tree ${SOURCE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
