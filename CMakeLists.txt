cmake_minimum_required(VERSION 3.10)
project(image_morphing)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(image_morphing main.cpp)

include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(image_morphing ${OpenCV_LIBS})