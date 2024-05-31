#include <iostream>
#include <opencv2/opencv.hpp>
#include <zbar.h>
#include <curl/curl.h>

using namespace cv;
using namespace std;
using namespace zbar;

// Function to send QR data to server
void sendToServer(const string& data) {
    CURL* curl;
    CURLcode res;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://your-server-url.com");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        
        string postData = "qrdata=" + data;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

// Function to recognize QR code
void recognizeQR(const Mat& frame) {
    ImageScanner scanner;
    scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
    
    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    
    int width = gray.cols;
    int height = gray.rows;
    uchar *raw = (uchar *)gray.data;
    Image image(width, height, "Y800", raw, width * height);
    
    scanner.scan(image);
    
    for(Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
        cout << "Decoded QR Code: " << symbol->get_data() << endl;
        sendToServer(symbol->get_data());
    }
}

int main(int argc, char* argv[]) {
    VideoCapture cap(0);
    if(!cap.isOpened()) {
        cerr << "Error opening video stream" << endl;
        return -1;
    }
    
    while(1) {
        Mat frame;
        cap >> frame;
        
        if(frame.empty())
            break;
        
        recognizeQR(frame);
        
        imshow("QR Code Scanner", frame);
        
        if(waitKey(10) == 27)
            break;
    }
    
    cap.release();
    destroyAllWindows();
    
    return 0;
}
