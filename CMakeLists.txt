cmake_minimum_required(VERSION 3.21)
project(posture_reminder)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED core objdetect videoio highgui)
find_package(Threads REQUIRED)

find_library(PULSE pulse)
find_library(PULSE_SIMPLE pulse-simple)

add_executable(posture_reminder main.cpp)
target_link_libraries(posture_reminder PRIVATE ${OpenCV_LIBS} Threads::Threads)

if (PULSE AND PULSE_SIMPLE)
    target_link_libraries(posture_reminder PRIVATE ${PULSE_SIMPLE} ${PULSE})
    target_compile_definitions(posture_reminder PRIVATE HAS_PULSE)
endif()