#include <stdio.h>
#include <unistd.h>
#include <string.h>

void convertQRValue(const char* qrValue) {
    int value = atoi(qrValue);
    if (value == 32) {
        printf("(3, 2)\n");
    } else {
        printf("QR Code data received: %s\n", qrValue);
    }
}

int main() {
    int pipeFd[2];
    if (pipe(pipeFd) == -1) {
        fprintf(stderr, "Error: Could not create pipe\n");
        return -1;
    }

    char buffer[1024];
    while (1) {
        ssize_t bytesRead = read(pipeFd[0], buffer, sizeof(buffer) - 1);
        if (bytesRead == -1) {
            fprintf(stderr, "Error: Could not read from pipe\n");
            break;
        }

        buffer[bytesRead] = '\0';
        convertQRValue(buffer);
    }

    close(pipeFd[0]);
    close(pipeFd[1]);
    return 0;
}