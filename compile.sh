g++ -o posture_reminder -DHAS_PULSE -I/usr/include/opencv4 main.cpp -lopencv_core -lopencv_objdetect -lopencv_videoio -lopencv_highgui -pthread -lpulse-simple -lpulse -lopencv_imgproc
