
cmake_minimum_required(VERSION 3.10.2)
project(algo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-std=c++20 -fcoroutines)
### add_definitions(-g)
### add_definitions(-O2)

message("-----------------------------------------------")
message("CMAKE_BUILD_TYPE        = ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS           = ${CMAKE_C_FLAGS}")
message("CMAKE_C_FLAGS_DEBUG     = ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE   = ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS         = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_DEBUG   = ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
message("-----------------------------------------------")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS   "-g -O0")
    set(CMAKE_CXX_FLAGS "-g -O0")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS   "-g -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "-g -O3 -DNDEBUG")
endif()

message("-----------------------------------------------")
message("CMAKE_BUILD_TYPE        = ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS           = ${CMAKE_C_FLAGS}")
message("CMAKE_C_FLAGS_DEBUG     = ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE   = ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS         = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_DEBUG   = ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")
message("-----------------------------------------------")

### header ###
include_directories(include)
include_directories("~/Downloads/boost_1_74_0") 
# include_directories("~/dev/yaml-cpp/include") 
# include_directories("~/dev/reckless/reckless/include") 
# If the same class exists in more than one include path, pick the former one.

### source ###
file(GLOB SOURCES "src/*.cpp" "src_test/*.cpp")
add_executable(Test ${SOURCES})

### link lib ###
target_link_libraries(Test -lpthread)
target_link_libraries(Test -lrt)
target_link_libraries(Test -lgtest)
# target_link_libraries(Test "/home/dick/dev/yaml-cpp/build/debug/libyaml-cppd.a")
# target_link_libraries(Test "/home/dick/dev/reckless/build/debug/libreckless.a")

# The following does not work. Why?
# link_directories("~/dev/yaml-cpp/build/debug") 
# target_link_libraries(Test libyaml-cppd.a) 

