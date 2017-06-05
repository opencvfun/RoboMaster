cmake_minimum_required(VERSION 3.6)
project(Client)

set(CMAKE_CXX_STANDARD 11)
find_package(OpenCV REQUIRED)

set(SOURCE_FILES main.cpp Structure.cpp Structure.h)
add_executable(Client ${SOURCE_FILES})
target_link_libraries(Client ${OpenCV_LIBS})