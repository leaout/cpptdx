cmake_minimum_required(VERSION 3.10)
project(cpptdx)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER "g++")
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(Boost REQUIRED COMPONENTS system filesystem thread)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

include_directories(${ASIO_INCLUDE_DIRS})


add_executable(tdx_client ./src/tdx_client.cpp)
target_link_libraries(tdx_client -lpthread  Boost::system Boost::thread ${ZLIB_LIBRARIES})