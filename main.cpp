#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <optional>

std::optional<cv::Rect> detect(cv::Mat frame, cv::CascadeClassifier& face_cascade, cv::CascadeClassifier& eyes_cascade);

#define BEEP_FILE "data/beep.wav"

void beep()
{
    bool fallback = true;

#ifdef __linux__
    if(system("paplay " BEEP_FILE "&") == 0) {
        fallback = false;
    } else {

    }
#endif

    if (fallback) {
        std::cout << "\a" << std::flush;
    }
}

int main(int argc, const char** argv)
{
    cv::CommandLineParser parser(argc, argv,
            "{help h||}"
            "{face_cascade|/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml|Path to face cascade.}"
            "{eyes_cascade|/usr/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml|Path to eyes cascade.}"
            "{camera|0|Camera device number.}");

    parser.about("This program uses face recognition to remind you to sit up straight.\n");

    if (!parser.check()) {
        parser.printErrors();
        return -1;
    }
    if (!parser.get<cv::String>("help").empty()) {
        parser.printMessage();
        return 0;
    }

    //-- 1. Load the cascades
    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load(cv::samples::findFile(parser.get<cv::String>("face_cascade")))) {
        std::cout << "--(!)Error loading face cascade\n";
        return -1;
    };

    cv::CascadeClassifier eyes_cascade;
    if (!eyes_cascade.load(cv::samples::findFile(parser.get<cv::String>("eyes_cascade")))) {
        std::cout << "--(!)Error loading eyes cascade\n";
        return -1;
    };

    int camera_device = parser.get<int>("camera");
    cv::VideoCapture capture;
    capture.open(camera_device, cv::CAP_V4L2);
    if (!capture.isOpened()) {
        std::cout << "--(!)Error opening video capture\n";
        return -1;
    }

    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    capture.set(cv::CAP_PROP_FPS, 30);

    std::optional<cv::Rect> desired;

    bool calibrate = true;

    cv::Mat frame;
    while (capture.read(frame)) {
        if (frame.empty()) {
            std::cout << "--(!) No captured frame -- Break!\n";
            break;
        }

        std::optional<cv::Rect> postion = detect(frame, face_cascade, eyes_cascade);

        if (desired.has_value()) {
            cv::rectangle(frame, desired->tl(), desired->br(), cv::Scalar(0, 0, 255));
        }

        imshow("Capture - Face detection", frame);

        int key = cv::pollKey();
        if (key==27) {
            break;
        } else if (key==' ') {
            calibrate = true;
        }

        // TOOD use multiple positions to calibrate
        if (calibrate && postion.has_value()) {
            calibrate = false;
            desired = postion;
            std::cout << "Set desired rectangle to " << *desired << "\n";
        }

        if (desired.has_value() && postion.has_value()) {
            auto d = *desired;
            auto p = *postion;

            assert(d.width>=0);
            assert(d.height>=0);
            assert(p.width>=0);
            assert(p.height>=0);

            auto deltax = (d.x+d.width/2)-(p.x+p.width/2);
            auto deltay = (d.y+d.height/2)-(p.y+p.height/2);
            auto delta_size = sqrt(d.width*d.height)/sqrt(p.width*p.height);

            if (abs(deltay)>150 || abs(1-delta_size)>0.2) {
                beep();
            }
            std::cout << "deltax=" << deltax << " deltay=" << deltay << " delta_size=" << delta_size << "\n";
        } else if (desired.has_value()) {
            std::cout << "Could not find a face\n";
        }

    }
    return 0;
}

std::optional<cv::Rect> detect(cv::Mat frame, cv::CascadeClassifier& face_cascade, cv::CascadeClassifier& eyes_cascade)
{
    cv::Mat frame_gray;
    cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);

    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale(frame_gray, faces);

    for (auto const& face: faces) {
//        cv::Point center(face.x+face.width/2, face.y+face.height/2);
        cv::rectangle(frame, face.tl(), face.br(), cv::Scalar(255, 0, 255));

        cv::Mat faceROI = frame_gray(face);

        std::vector<cv::Rect> eyes;
        eyes_cascade.detectMultiScale(faceROI, eyes);

        for (auto const& eye: eyes) {
            cv::Point eye_center(face.x+eye.x+eye.width/2, face.y+eye.y+eye.height/2);
            int radius = cvRound((eye.width+eye.height)*0.25);
            circle(frame, eye_center, radius, cv::Scalar(255, 0, 0), 4);
        }
    }

    if (faces.size()==1) {
        return faces[0];
    } else {
        return {};
    }
}
