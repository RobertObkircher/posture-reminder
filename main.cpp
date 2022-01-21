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

void open_capture(cv::VideoCapture& capture, int camera_device)
{
    capture.open(camera_device, cv::CAP_V4L2);
    if (!capture.isOpened()) {
        std::cerr << "Error opening video capture\n";
        return;
    }

    // without mjpg my camera can only do 5 fps at 1080p
    capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    capture.set(cv::CAP_PROP_FPS, 30);

    std::cout << "capturing with "
              << capture.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << "@"
              << capture.get(cv::CAP_PROP_FPS) << "\n";
}

class History {
    int m_size;
    std::deque<cv::Rect> m_positions;
    std::optional<cv::Rect> m_average_position;

public:
    explicit History(int size)
            :m_size(size) { };

    [[nodiscard]] std::optional<cv::Rect> average_position() const { return m_average_position; }

    [[nodiscard]] bool full() const { return m_positions.size()==m_size; }

    void add(cv::Rect position)
    {
        if (m_positions.size()>=m_size) {
            m_positions.pop_front();
        }
        m_positions.push_back(position);

        cv::Rect average;
        for (auto const& pos: m_positions) {
            average.x += pos.x;
            average.y += pos.y;
            average.width += pos.width;
            average.height += pos.height;
        }
        int size = m_positions.size();
        average.x /= size;
        average.y /= size;
        average.width /= size;
        average.height /= size;
        m_average_position = average;
    }

    void clear()
    {
        m_positions.clear();
        m_average_position = {};
    }
};

enum class State {
    Quit,
    Calibrating,
    Reset,
    Checking, // requires "until"
    Paused,
    Sleeping, // requires "until"
};

State handle_key(int key, State otherwise, State if_space, State if_p)
{
    if (key==27 || key=='q') {
        return State::Quit;
    } else if (key==' ') {
        return if_space;
    } else if (key=='p') {
        return if_p;
    } else {
        return otherwise;
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
    cv::Mat frame;
    History history{10};
    std::optional<cv::Rect> desired;
    std::chrono::time_point until = std::chrono::steady_clock::now();

    State state = State::Reset;

    while (state!=State::Quit) {
        // process camera input
        if (state==State::Calibrating || state==State::Checking) {
            if (!capture.isOpened()) {
                open_capture(capture, camera_device);
            }
            if (capture.isOpened()) {
                capture.read(frame);
                if (frame.empty()) {
                    std::cerr << "No captured frame!\n";
                } else {
                    std::optional<cv::Rect> position = detect_single_face(frame, face_cascade, eyes_cascade);
                    if (position.has_value()) {
                        history.add(*position);
                    }

                    if (desired.has_value()) {
                        cv::rectangle(frame, desired->tl(), desired->br(), cv::Scalar(0, 255, 0), 6);
                    }
                    if (history.average_position().has_value()) {
                        cv::rectangle(frame, history.average_position()->tl(), history.average_position()->br(),
                                cv::Scalar(255, 0, 0), 3);
                    }
                    imshow("posture_reminder", frame);
                }
            }
        } else {
            capture.release();
        }

        switch (state) {
        case State::Quit: {
            break;
        }
        case State::Reset: {
            desired = {};
            history.clear();
            state = State::Calibrating;
            break;
        }
        case State::Calibrating: {
            if (history.full() && history.average_position().has_value()) {
                desired = history.average_position();
                std::cout << "Set desired rectangle to " << *desired << "\n";

                until = std::chrono::steady_clock::now()+std::chrono::seconds(10);
                state = State::Checking;
            } else {
                state = handle_key(cv::pollKey(), state, State::Calibrating, State::Paused);
            }
            break;
        }
        case State::Checking: {
            if (desired.has_value() && history.average_position().has_value()) {
                auto d = *desired;
                auto p = *history.average_position();

                auto deltax = (d.x+d.width/2)-(p.x+p.width/2);
                auto deltay = (d.y+d.height/2)-(p.y+p.height/2);
                auto delta_size = sqrt(d.width*d.height)/sqrt(p.width*p.height);

                if (abs(deltay)>100 || abs(1-delta_size)>0.1) {
                    beep();
                }
                std::cout << "deltax: " << deltax << ", deltay: " << deltay << " delta_size: " << delta_size << "\n";
            }

            auto now = std::chrono::steady_clock::now();
            if (until<now) {
                until = now+std::chrono::minutes(10);
                state = State::Sleeping;
            } else {
                state = handle_key(cv::pollKey(), state, State::Calibrating, State::Paused);
            }
            break;
        }
        case State::Paused: {
            state = handle_key(cv::waitKey(1000), state, State::Reset, State::Reset);
            break;
        }
        case State::Sleeping: {
            auto now = std::chrono::steady_clock::now();
            if (until<now) {
                history.clear();
                until = now+std::chrono::seconds(10);
                state = State::Checking;
            } else {
                state = handle_key(cv::waitKey(1000), state, State::Reset, State::Paused);
            }
            break;
        }
        }
    }

    return 0;
}
