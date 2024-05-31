#include <iostream>
#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread> // 스레드 관련 라이브러리 추가

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"

struct ClientAction {
    int row;
    int col;
    int action;
};

void send_action_to_server(int sockfd, ClientAction *action) {
    if (send(sockfd, action, sizeof(ClientAction), 0) == -1) {
        perror("send");
        exit(1);
    }
}

int decode_qr_code(cv::Mat &frame, int *row, int *col) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);

    zbar::Image image(gray.cols, gray.rows, "Y800", gray.data, gray.cols * gray.rows);

    int n = scanner.scan(image);

    for (zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
        std::string data = symbol->get_data();
        if (data.length() == 2) {
            *row = data[0] - '0';
            *col = data[1] - '0';
            return 1;
        }
    }
    return 0;
}

void qr_code_scanner_thread(int sockfd) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        close(sockfd);
        return;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame;

        int row, col;
        if (decode_qr_code(frame, &row, &col)) {
            std::cout << "QR Code detected: row=" << row << ", col=" << col << std::endl;

            ClientAction action;
            action.row = row;
            action.col = col;
            action.action = 0;

            send_action_to_server(sockfd, &action);
        }

        cv::imshow("QR Code Scanner", frame);
        if (cv::waitKey(30) >= 0) break;
    }
}

int main() {
    int sockfd;
    sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    if (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    // QR 코드 스캐너를 별도의 스레드로 실행
    std::thread qr_thread(qr_code_scanner_thread, sockfd);
    
    // 메인 스레드에서 다른 작업이 있다면 여기에 작성
    // 예시로 메인 스레드가 종료되지 않도록 대기
    qr_thread.join();

    close(sockfd);
    return 0;
}