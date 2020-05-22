cmake_minimum_required (VERSION 3.17)
project (IFF VERSION 0.3.0)
include_directories("include")
configure_file(include/config.h.in include/config.h)

set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB)

file(GLOB COMMON "src/*.cpp")
file(GLOB ARCHIVE "src/archive/*.cpp")
file(GLOB CREATE "src/create/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

add_executable(create ${COMMON} ${CREATE})
target_link_libraries(create ${ZLIB_LIBRARIES})
target_include_directories(create PUBLIC "${PROJECT_BINARY_DIR}")
