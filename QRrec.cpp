#include <iostream>
#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <cstring>
#include <thread>

extern "C" {
    void send_action_to_server(const char *server_addr, int server_port, int row, int col, int trap);
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

void qr_code_scanner_thread(const char *server_addr, int server_port) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return;
    }

    int previous_row = -1, previous_col = -1;
    int count = 0;

    while (true) {
        cv::Mat frame;
        cap >> frame;

        int row, col;
        if (decode_qr_code(frame, &row, &col)) {
            std::cout << "QR Code detected: row=" << row << ", col=" << col << std::endl;

            if (row != previous_row || col != previous_col) {
                count++;
                previous_row = row;
                previous_col = col;
            }

            int trap = (count >= 8 && count <= 11) ? 1 : 0;
            send_action_to_server(server_addr, server_port, row, col, trap);
        }

        cv::imshow("QR Code Scanner", frame);
        if (cv::waitKey(30) >= 0) break;
    }
}

int main() {
    const char *server_addr = "127.0.0.1";
    int server_port = 8080;

    std::thread qr_thread(qr_code_scanner_thread, server_addr, server_port);
    qr_thread.join();

    return 0;
}
