cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-narrowing")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/deps)
include_directories(${CMAKE_SOURCE_DIR}/deps/httplib)
include_directories(${CMAKE_SOURCE_DIR}/deps/json)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/msg)
include_directories(${CMAKE_SOURCE_DIR}/src/tdx)
include_directories(/usr/include)
include_directories(/usr/include/boost)
include_directories(/usr/include/asio)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

set(TEST_SOURCES
    test_tdx_api.cpp
)

add_executable(run_tests ${TEST_SOURCES})
target_link_libraries(run_tests
    Threads::Threads
    ${ZLIB_LIBRARIES}
)
