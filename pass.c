#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define FILENAME "qrcode_data.txt"
#define BUFSIZE 1024

struct ClientAction {
    int row;
    int col;
    int action;
};

void send_action_to_server(int sockfd, struct ClientAction *action) {
    if (send(sockfd, action, sizeof(struct ClientAction), 0) == -1) {
        perror("send");
        exit(1);
    }
}

void read_qr_code_data(int sockfd) {
    char lastData[BUFSIZE] = {0};

    while (1) {
        FILE *file = fopen(FILENAME, "r");
        if (file == NULL) {
            perror("Error opening file");
            exit(1);
        }

        char data[BUFSIZE] = {0};
        if (fgets(data, BUFSIZE, file) != NULL) {
            // Remove newline character from the end of the string if present
            data[strcspn(data, "\n")] = '\0';

            if (strcmp(data, lastData) != 0) {
                printf("New QR Code data: %s\n", data);

                struct ClientAction action;
                // Ensure data is in the format "row col" (two integers separated by space)
                if (sscanf(data, "%1d%1d", &action.row, &action.col) == 2) {
                    action.action = 0; // 기본 동작은 move로 설정
                    send_action_to_server(sockfd, &action);
                    strncpy(lastData, data, BUFSIZE);
                } else {
                    fprintf(stderr, "Error: Invalid QR Code data format\n");
                }
            }
        } else {
            printf("No data in file\n");
        }

        fclose(file);
        sleep(1); // 1초 대기
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    read_qr_code_data(sockfd);

    close(sockfd);
    return 0;
}
