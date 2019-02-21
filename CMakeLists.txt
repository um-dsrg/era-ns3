cmake_minimum_required(VERSION 3.5)
project(ns3)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories("$ENV{HOME}/Repositories/Ns3/build")

file(GLOB_RECURSE ns3_source_list "src/*.cc" "src/*.h")
file(GLOB_RECURSE ns3_examples_list "examples/*.cc" "examples/*.h")
file(GLOB_RECURSE ns3_common_code_list "common-code/*.cc" "common-code/*.h")

add_executable(ns3 ${ns3_source_list} ${ns3_examples_list} ${ns3_common_code_list})

target_link_libraries(ns3 tinyxml2 ${Boost_LIBRARIES})

add_custom_target(ns3-waf ALL $ENV{HOME}/Repositories/Ns3/waf build WORKING_DIRECTORY $ENV{HOME}/Repositories/Ns3/)
