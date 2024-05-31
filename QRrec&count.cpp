#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace cv;
using namespace std;
using namespace zbar;

struct QRData {
    int row;
    int col;
    string data;
};

// QR 코드 인식 및 정보 추출 함수
QRData decodeQRCode(const Mat &image) {
    QRData qrData;
    qrData.row = -1;
    qrData.col = -1;

    // QR 코드 스캐너 초기화
    ImageScanner scanner;
    scanner.set_config(ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    int width = gray.cols;
    int height = gray.rows;
    uchar *raw = (uchar *)gray.data;

    Image zbarImage(width, height, "Y800", raw, width * height);
    scanner.scan(zbarImage);

    // QR 코드 데이터 추출
    for (Image::SymbolIterator symbol = zbarImage.symbol_begin(); symbol != zbarImage.symbol_end(); ++symbol) {
        qrData.data = symbol->get_data();
        // QR 코드 데이터에서 행과 열 추출 (예시로 행과 열을 데이터로부터 파싱)
        // qrData.row = ...;
        // qrData.col = ...;
        break;
    }

    return qrData;
}

// QR 코드 카운트 업데이트 함수
int updateQRCount(const string &data) {
    unordered_map<string, int> qrCount;
    ifstream infile("qr_count.txt");

    // 기존 카운트 파일 읽기
    if (infile.is_open()) {
        string key;
        int value;
        while (infile >> key >> value) {
            qrCount[key] = value;
        }
        infile.close();
    }

    // QR 코드 데이터 카운트 증가
    qrCount[data]++;

    // 업데이트된 카운트 파일 쓰기
    ofstream outfile("qr_count.txt");
    if (outfile.is_open()) {
        for (const auto &pair : qrCount) {
            outfile << pair.first << " " << pair.second << endl;
        }
        outfile.close();
    }

    return qrCount[data];
}

int main() {
    Mat image = imread("qrcode.png");
    if (image.empty()) {
        cerr << "Image not found!" << endl;
        return -1;
    }

    try {
        QRData qrData = decodeQRCode(image);
        if (qrData.data.empty()) {
            cerr << "No QR code detected!" << endl;
            return -1;
        }

        int count = updateQRCount(qrData.data);
        cout << "QR Code: " << qrData.data << " Count: " << count << endl;

        // C 프로그램에 전달할 정보 생성
        ofstream outfile("qr_info.txt");
        if (outfile.is_open()) {
            outfile << qrData.row << " " << qrData.col << " " << count << endl;
            outfile.close();
        }

    } catch (const exception &e) {
        cerr << e.what() << endl;
    }

    return 0;
}
