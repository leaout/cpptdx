cmake_minimum_required(VERSION 3.10)
project(cpptdx)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER "g++")
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/deps/)
include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(/usr/local/include/brpc/include)


find_package(Boost REQUIRED COMPONENTS system filesystem thread)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)
include_directories(${ASIO_INCLUDE_DIRS})

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

link_directories(${CMAKE_BINARY_DIR})
link_directories(/usr/local/lib/brpc/lib/)

aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/tdx SRC_TDX_LIST)
add_library(tdx_api SHARED ${SRC_TDX_LIST})
target_link_libraries(tdx_api -lpthread  Boost::system Boost::thread ${ZLIB_LIBRARIES})

add_executable(tdxhq_server ${SRC_LIST})
add_dependencies(tdxhq_server tdx_api)
target_link_libraries(tdxhq_server -lpthread  Boost::system Boost::thread ${ZLIB_LIBRARIES} -lyaml-cpp -ltdx_api)