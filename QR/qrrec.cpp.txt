#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <fstream>

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
    while (true) {
        cap >> frame; // 프레임 캡처
        if (frame.empty()) {
            std::cerr << "Error: Empty frame" << std::endl;
            break;
        }

        std::string data = qrDecoder.detectAndDecode(frame); // QR 코드 인식 및 디코딩
        if (!data.empty()) {
            std::cout << "QR Code detected: " << data << std::endl;
            
            // 데이터를 파일에 쓰기
            std::ofstream outFile("qrcode_data.txt");
            if (outFile.is_open()) {
                outFile << data;
                outFile.close();
            } else {
                std::cerr << "Error: Could not open file to write data" << std::endl;
            }

            cv::putText(frame, data, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
        }

        cv::imshow("QR Code Scanner", frame); // 프레임 출력
        if (cv::waitKey(1) == 27) { // ESC 키를 누르면 종료
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
