#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <unistd.h>
#include <cstring>

int main() {
    // 카메라 장치 열기
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return -1;
    }

    // QR 코드 디텍터 초기화
    cv::QRCodeDetector qrDecoder;
    cv::Mat frame;

    // 파이프 열기
    int pipeFd[2];
    if (pipe(pipeFd) == -1) {
        std::cerr << "Error: Could not create pipe" << std::endl;
        return -1;
    }

    while (true) {
        cap >> frame; // 프레임 캡처
        if (frame.empty()) {
            std::cerr << "Error: Empty frame" << std::endl;
            break;
        }

        std::string data = qrDecoder.detectAndDecode(frame); // QR 코드 인식 및 디코딩
        if (!data.empty()) {
            std::cout << "QR Code detected: " << data << std::endl;
            cv::putText(frame, data, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);

            // QR 코드 데이터를 파이프에 쓰기
            const char* dataStr = data.c_str();
            write(pipeFd[1], dataStr, strlen(dataStr) + 1);
        }

        cv::imshow("QR Code Scanner", frame); // 프레임 출력
        if (cv::waitKey(1) == 27) { // ESC 키를 누르면 종료
            break;
        }
    }

    // 파이프 닫기
    close(pipeFd[0]);
    close(pipeFd[1]);

    cap.release();
    cv::destroyAllWindows();
    return 0;
}