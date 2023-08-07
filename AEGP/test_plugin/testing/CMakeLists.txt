cmake_minimum_required(VERSION 3.17)
project(Drafting)

set(CMAKE_CXX_STANDARD 14)

#list(APPEND CMAKE_PREFIX_PATH "/Users/emerson/opt/miniconda3/include/python3.8")
link_directories(/usr/local/lib)
include_directories(/usr/local/include)

add_executable(Drafting main.cpp Server.cpp Server.h)

# target_link_libraries(Drafting zmq)
target_link_libraries(Drafting sioclient)
# find_package(RapidJSON)
