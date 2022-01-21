#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <optional>
#include <deque>

#define BEEP_FILE "data/beep.wav"

void beep()
{
    bool fallback = true;

#ifdef __linux__
    if (system("paplay " BEEP_FILE "&")==0) {
        fallback = false;
    } else {

    }
#endif

    if (fallback) {
        std::cout << "\a" << std::flush;
    }
}

std::optional<cv::Rect>
detect_single_face(cv::Mat frame, cv::CascadeClassifier& face_cascade, cv::CascadeClassifier& eyes_cascade)
{
    cv::Mat frame_gray;
    cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);

    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale(frame_gray, faces);

    for (auto const& face: faces) {
        cv::rectangle(frame, face.tl(), face.br(), cv::Scalar(255, 0, 255));

        cv::Mat faceROI = frame_gray(face);

        std::vector<cv::Rect> eyes;
        eyes_cascade.detectMultiScale(faceROI, eyes);

        for (auto const& eye: eyes) {
            cv::Point eye_center(face.x+eye.x+eye.width/2, face.y+eye.y+eye.height/2);
            int radius = cvRound((eye.width+eye.height)*0.25);
            circle(frame, eye_center, radius, cv::Scalar(255, 0, 255), 2);
        }
    }

    if (faces.size()==1) {
        return faces[0];
    } else {
        return {};
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

    const int HISTORY_SIZE = 10;
    std::deque<cv::Rect> previous_positions;
    std::optional<cv::Rect> average_position;

    cv::Mat frame;
    while (capture.read(frame)) {
        if (frame.empty()) {
            std::cout << "--(!) No captured frame -- Break!\n";
            break;
        }

        std::optional<cv::Rect> position = detect_single_face(frame, face_cascade, eyes_cascade);

        if (position.has_value()) {
            if (previous_positions.size()>=HISTORY_SIZE) {
                previous_positions.pop_front();
            }
            previous_positions.push_back(*position);

            cv::Rect average;
            for (auto const& pos: previous_positions) {
                average.x += pos.x;
                average.y += pos.y;
                average.width += pos.width;
                average.height += pos.height;
            }
            int size = previous_positions.size();
            average.x /= size;
            average.y /= size;
            average.width /= size;
            average.height /= size;
            average_position = average;
        }

        for (const auto &pos : previous_positions) {
//            cv::rectangle(frame, pos.tl(), pos.br(), cv::Scalar(255, 0,255), 1);
        }
        if (desired.has_value()) {
            cv::rectangle(frame, desired->tl(), desired->br(), cv::Scalar(0, 255, 0), 6);
        }
        if (average_position.has_value()) {
            cv::rectangle(frame, average_position->tl(), average_position->br(), cv::Scalar(255, 0,0), 3);
        }


        imshow("Capture - Face detection", frame);

        int key = cv::pollKey();
        if (key==27) {
            break;
        } else if (key==' ') {
            calibrate = true;
        }

        // TOOD use multiple previous_positions to calibrate
        if (calibrate && previous_positions.size()==HISTORY_SIZE && average_position.has_value()) {
            calibrate = false;
            desired = average_position;
            std::cout << "Set desired rectangle to " << *desired << "\n";
        }

        if (desired.has_value() && average_position.has_value()) {
            auto d = *desired;
            auto p = *average_position;

            assert(d.width>=0);
            assert(d.height>=0);
            assert(p.width>=0);
            assert(p.height>=0);

            auto deltax = (d.x+d.width/2)-(p.x+p.width/2);
            auto deltay = (d.y+d.height/2)-(p.y+p.height/2);
            auto delta_size = sqrt(d.width*d.height)/sqrt(p.width*p.height);

            if (abs(deltay)>100 || abs(1-delta_size)>0.1) {
                beep();
            }
            std::cout << "deltax=" << deltax << " deltay=" << deltay << " delta_size=" << delta_size << "\n";
        } else if (desired.has_value()) {
            std::cout << "Could not find a face\n";
        }

    }
    return 0;
}
