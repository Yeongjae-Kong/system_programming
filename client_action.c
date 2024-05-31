#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

struct ClientAction {
    int row;
    int col;
    int trap; // 1: 설치, 0: 미설치
};

void sendActionToServer(struct ClientAction action) {
    // 소켓 통신을 통해 서버로 정보 전송
    int sock;
    struct sockaddr_in server_addr;
    char message[50];

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket() error");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() error");
        close(sock);
        exit(1);
    }

    sprintf(message, "Row: %d, Col: %d, Trap: %d", action.row, action.col, action.trap);
    write(sock, message, strlen(message));
    close(sock);

    printf("Action sent to server: row=%d, col=%d, trap=%d\n", action.row, action.col, action.trap);
}

int main() {
    struct ClientAction action;
    int count;

    // C++ 프로그램에서 생성한 QR 코드 정보 파일 읽기
    FILE *file = fopen("qr_info.txt", "r");
    if (file == NULL) {
        perror("fopen() error");
        return 1;
    }

    fscanf(file, "%d %d %d", &action.row, &action.col, &count);
    fclose(file);

    // QR 코드 카운트가 8, 9, 10, 11일 때만 함정 설치
    if (count >= 8 && count <= 11) {
        action.trap = 1;
    } else {
        action.trap = 0;
    }

    sendActionToServer(action);

    return 0;
}
