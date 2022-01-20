cmake_minimum_required(VERSION 3.21)
project(posture_reminder)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCV REQUIRED core objdetect videoio highgui)

add_executable(posture_reminder main.cpp)
target_link_libraries(posture_reminder ${OpenCV_LIBS})

get_target_property(LIBS posture_reminder LINK_LIBRARIES)
list(APPEND LIBS "opencv_imgproc")
list(JOIN LIBS " -l" LIBS)

file(WRITE "${CMAKE_SOURCE_DIR}/compile.sh" "g++ -o posture_reminder -I${OpenCV_INCLUDE_DIRS} main.cpp -l${LIBS}")

message(STATUS ${OUT})