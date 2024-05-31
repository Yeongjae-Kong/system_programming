#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME "qrcode_data.txt"
#define BUFSIZE 1024

int main() {
    char lastData[BUFSIZE] = {0};

    while (1) {
        FILE *file = fopen(FILENAME, "r");
        if (file == NULL) {
            perror("Error opening file");
            return 1;
        }

        char data[BUFSIZE] = {0};
        if (fgets(data, BUFSIZE, file) != NULL) {
            // Remove newline character from the end of the string if present
            data[strcspn(data, "\n")] = '\0';

            if (strcmp(data, lastData) != 0) {
                printf("New QR Code data: %s\n", data);
                strncpy(lastData, data, BUFSIZE);
            }
        } else {
            printf("No data in file\n");
        }

        fclose(file);
        sleep(1); // 1초 대기
    }

    return 0;
}
