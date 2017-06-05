cmake_minimum_required(VERSION 2.8)
project(VideoServer)

set(CMAKE_CXX_STANDARD 11)
find_package(OpenCV REQUIRED)

set(SOURCE_FILES main.cpp Structure.cpp Structure.h v4l2uvc.c v4l2uvc.h uvcvideo.h)
add_executable(VideoServer ${SOURCE_FILES})
target_link_libraries(VideoServer -lv4l2 -lpthread ${OpenCV_LIBS})
