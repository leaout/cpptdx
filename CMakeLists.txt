cmake_minimum_required(VERSION 3.10)
project(cpptdx)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_C_COMPILER "g++")
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/deps/)
include_directories(${CMAKE_SOURCE_DIR}/src/)
include_directories(${CMAKE_SOURCE_DIR}/deps/httplib)
include_directories(${CMAKE_SOURCE_DIR}/deps/json)

include_directories(/usr/include)
include_directories(/usr/include/boost)
include_directories(/usr/include/asio)

include_directories(${CMAKE_SOURCE_DIR}/deps/yaml-cpp)

find_package(Threads REQUIRED)

find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
  include_directories(${YAML_CPP_INCLUDE_DIR})
  set(YAML_CPP_LIBS -lyaml-cpp)
else()
  set(YAML_CPP_LIBS "")
endif()

aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/tdx SRC_TDX_LIST)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/msg SRC_MSG_LIST)
add_library(tdx_api SHARED ${SRC_TDX_LIST} ${SRC_MSG_LIST})
target_link_libraries(tdx_api -lpthread ${ZLIB_LIBRARIES})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,'$ORIGIN'")

add_executable(tdxhq_server ${SRC_LIST})
add_dependencies(tdxhq_server tdx_api)
target_link_libraries(tdxhq_server Threads::Threads ${ZLIB_LIBRARIES} ${YAML_CPP_LIBS} -L${CMAKE_BINARY_DIR} -ltdx_api)

enable_testing()
add_subdirectory(tests)
