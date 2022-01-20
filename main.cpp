#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>

void detect(cv::Mat frame, cv::CascadeClassifier& face_cascade, cv::CascadeClassifier& eyes_cascade);

int main(int argc, const char** argv)
{
    cv::CommandLineParser parser(argc, argv,
            "{help h||}"
            "{face_cascade|/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml|Path to face cascade.}"
            "{eyes_cascade|/usr/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml|Path to eyes cascade.}"
            "{camera|0|Camera device number.}");

    parser.about(
            "\nThis program demonstrates using the cv::CascadeClassifier class to detect objects (Face + eyes) in a video stream.\n"
            "You can use Haar or LBP features.\n\n");
    parser.printMessage();

    cv::String face_cascade_name = cv::samples::findFile(parser.get<cv::String>("face_cascade"));
    cv::String eyes_cascade_name = cv::samples::findFile(parser.get<cv::String>("eyes_cascade"));

    //-- 1. Load the cascades
    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load(face_cascade_name)) {
        std::cout << "--(!)Error loading face cascade\n";
        return -1;
    };

    cv::CascadeClassifier eyes_cascade;
    if (!eyes_cascade.load(eyes_cascade_name)) {
        std::cout << "--(!)Error loading eyes cascade\n";
        return -1;
    };

    int camera_device = parser.get<int>("camera");
    cv::VideoCapture capture;
    //-- 2. Read the video stream
    capture.open(camera_device);
    if (!capture.isOpened()) {
        std::cout << "--(!)Error opening video capture\n";
        return -1;
    }

    cv::Mat frame;
    while (capture.read(frame)) {
        if (frame.empty()) {
            std::cout << "--(!) No captured frame -- Break!\n";
            break;
        }

        detect(frame, face_cascade, eyes_cascade);

        imshow("Capture - Face detection", frame);

        if (cv::pollKey()==27) {
            break;
        }
    }
    return 0;
}

void detect(cv::Mat frame, cv::CascadeClassifier& face_cascade, cv::CascadeClassifier& eyes_cascade)
{
    cv::Mat frame_gray;
    cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);

    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale(frame_gray, faces);

    for (size_t i = 0; i<faces.size(); i++) {
        cv::Point center(faces[i].x+faces[i].width/2, faces[i].y+faces[i].height/2);
        ellipse(frame, center, cv::Size(faces[i].width/2, faces[i].height/2), 0, 0, 360, cv::Scalar(255, 0, 255), 4);

        cv::Mat faceROI = frame_gray(faces[i]);

        std::vector<cv::Rect> eyes;
        eyes_cascade.detectMultiScale(faceROI, eyes);

        for (size_t j = 0; j<eyes.size(); j++) {
            cv::Point eye_center(faces[i].x+eyes[j].x+eyes[j].width/2, faces[i].y+eyes[j].y+eyes[j].height/2);
            int radius = cvRound((eyes[j].width+eyes[j].height)*0.25);
            circle(frame, eye_center, radius, cv::Scalar(255, 0, 0), 4);
        }
    }

}
