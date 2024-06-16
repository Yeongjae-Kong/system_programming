#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#define MAP_SIZE 25
#define MAX_ITEMS 10
#define MAX_CLIENTS 2
#define MAP_ROW 5
#define MAP_COL 5

typedef enum {
    RIGHT, DOWN, LEFT, UP
} Direction;

typedef struct {
    int row;
    int col;
} Position;

typedef struct {
    enum Status {
        nothing, item, trap
    } status;
    int score;
} Item;

typedef struct {
    int row;
    int col;
    Item item;
} Node;

typedef struct {
    int socket;
    struct sockaddr_in address;
    int row;
    int col;
    int score;
    int bomb;
} client_info;

typedef struct {
    client_info players[MAX_CLIENTS];
    Node map[MAP_ROW][MAP_COL];
} DGIST;

enum Action {
    move, setBomb
};

typedef struct {
    int row;
    int col;
    enum Action action;
} ClientAction;

int map[MAP_ROW][MAP_COL];
Position items[MAX_ITEMS];
int numItems;

Position currentPos = { 5, 4 };
Direction currentDir = DOWN;
DGIST globalDgist; // 전역 변수로 선언

double distance(Position p1, Position p2) {
    return sqrt(pow(p1.row - p2.row, 2) + pow(p1.col - p2.col, 2));
}

void updateCarPosition(Direction direction);

int readQRData(Position* pos) {
    FILE* file = fopen("qrcode_data.txt", "r");
    if (file == NULL) {
        perror("fopen");
        return 0;
    }

    char buffer[3];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        Position newPos;
        newPos.row = buffer[0] - '0';
        newPos.col = buffer[1] - '0';

        if (newPos.row == pos->row && newPos.col == pos->col) {
            // 기존의 pos와 같은 값인 경우
            fclose(file);
            return 0;
        }
        else {
            printf("Change\n");
            *pos = newPos;
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}
void determineDirection() {

    FILE* logFile = fopen("direction_log.txt", "w");
    if (logFile == NULL) {
        perror("fopen");
        return;
    }

    // 1. targetItem의 좌표를 서버에서 받은 맵 정보를 바탕으로 업데이트
    Position targetItem = { 0, 1 };
    double minDistance = INFINITY;
    int maxScore = -1; // 점수는 음수가 아닐 것이므로 초기값을 -1로 설정

    for (int i = 0; i < MAP_ROW; i++) {
        for (int j = 0; j < MAP_COL; j++) {
            if (globalDgist.map[i][j].item.status == item && (i != currentPos.row || j != currentPos.col)) {
                double dist = distance(currentPos, (Position) { i, j });
                int currentScore = globalDgist.map[i][j].item.score;

                // 거리 조건이 더 작은 경우, 또는 거리가 같고 점수가 더 높은 경우 갱신
                if (dist < minDistance || (dist == minDistance && currentScore > maxScore)) {
                    minDistance = dist;
                    maxScore = currentScore;
                    targetItem.row = i;
                    targetItem.col = j;
                }
            }
        }
    }

    // 2. 현재 방향과 targetItem의 위치를 고려하여 이동 방향 결정
    int rowDiff = targetItem.row - currentPos.row;
    int colDiff = targetItem.col - currentPos.col;

    printf("Current Location: (%d, %d), Taret Item: (%d, %d)\n", currentPos.row, currentPos.col, targetItem.row, targetItem.col);

    // 직진하는 경우
    if (rowDiff == 0 && colDiff > 0 && currentDir == RIGHT && (globalDgist.map[currentPos.row][currentPos.col + 1].item.status != trap) && (0 <= currentPos.col + 1 && currentPos.col + 1 <= 4)) {
        printf("Command: STRAIGHT\n");
        fprintf(logFile, "STRAIGHT\n");
    }
    else if (rowDiff == 0 && colDiff < 0 && currentDir == LEFT && (globalDgist.map[currentPos.row][currentPos.col - 1].item.status != trap) && (0 <= currentPos.col - 1 && currentPos.col - 1 <= 4)) {
        printf("Command: STRAIGHT\n");
        fprintf(logFile, "STRAIGHT\n");
    }
    else if (rowDiff > 0 && colDiff == 0 && currentDir == UP && (globalDgist.map[currentPos.row + 1][currentPos.col].item.status != trap) && (0 <= currentPos.row - 1 && currentPos.row - 1 <= 4)) {
        printf("Command: STRAIGHT\n");
        fprintf(logFile, "STRAIGHT\n");
    }
    else if (rowDiff < 0 && colDiff == 0 && currentDir == DOWN && (globalDgist.map[currentPos.row - 1][currentPos.col].item.status != trap) && (0 <= currentPos.row + 1 && currentPos.row + 1 <= 4)) {
        printf("Command: STRAIGHT\n");
        fprintf(logFile, "STRAIGHT\n");
    }
    else {
        // 직진이 불가능한 경우, 좌회전 또는 우회전 결정
        Direction newDir;
        if (currentDir == RIGHT) {
            if (colDiff > 0 && (globalDgist.map[currentPos.row][currentPos.col + 1].item.status != trap) && (0 <= currentPos.col + 1 && currentPos.col + 1 <= 4)) {
                printf("Command: STRAIGHT\n");
                fprintf(logFile, "STRAIGHT\n");
                newDir = RIGHT;
            }
            else if (rowDiff >= 0 && (globalDgist.map[currentPos.row + 1][currentPos.col].item.status != trap) && (0 <= currentPos.row + 1 && currentPos.row + 1 <= 4)) {
                printf("Command: RIGHT\n");
                fprintf(logFile, "RIGHT\n");
                newDir = UP;
            }
            else if ((globalDgist.map[currentPos.row - 1][currentPos.col].item.status != trap) && (0 <= currentPos.row - 1 && currentPos.row - 1 <= 4)) {
                printf("Command: LEFT\n");
                fprintf(logFile, "LEFT\n");
                newDir = DOWN;
            }
            else {
                printf("Command: BACK\n");
                fprintf(logFile, "BACK\n");
                newDir = LEFT;
            }
        }
        else if (currentDir == DOWN) {
            if (rowDiff < 0 && (globalDgist.map[currentPos.row - 1][currentPos.col].item.status != trap) && (0 <= currentPos.row - 1 && currentPos.row - 1 <= 4)) {
                printf("Command: STRAIGHT\n");
                fprintf(logFile, "STRAIGHT\n");
                newDir = DOWN;
            }
            else if (colDiff <= 0 && (globalDgist.map[currentPos.row][currentPos.col - 1].item.status != trap) && (0 <= currentPos.col - 1 && currentPos.col - 1 <= 4)) {
                printf("Command: LEFT\n");
                fprintf(logFile, "LEFT\n");
                newDir = LEFT;
            }
            else if ((globalDgist.map[currentPos.row][currentPos.col + 1].item.status != trap) && (0 <= currentPos.col + 1 && currentPos.col + 1 <= 4)) {
                printf("Command: RIGHT\n");
                fprintf(logFile, "RIGHT\n");
                newDir = RIGHT;
            }
            else {
                printf("Command: BACK\n");
                fprintf(logFile, "BACK\n");
                newDir = UP;
            }
        }
        else if (currentDir == LEFT) {
            if (colDiff < 0 && (globalDgist.map[currentPos.row][currentPos.col - 1].item.status != trap) && (0 <= currentPos.col - 1 && currentPos.col - 1 <= 4)) {
                printf("Command: STRAIGHT\n");
                fprintf(logFile, "STRAIGHT\n");
                newDir = LEFT;
            }
            else if (rowDiff <= 0 && (globalDgist.map[currentPos.row - 1][currentPos.col].item.status != trap) && (0 <= currentPos.row - 1 && currentPos.row - 1 <= 4)) {
                printf("Command: RIGHT\n");
                fprintf(logFile, "RIGHT\n");
                newDir = DOWN;
            }
            else if ((globalDgist.map[currentPos.row + 1][currentPos.col].item.status != trap) && (0 <= currentPos.row + 1 && currentPos.row + 1 <= 4)) {
                printf("Command: LEFT\n");
                fprintf(logFile, "LEFT\n");
                newDir = UP;
            }
            else {
                printf("Command: BACK\n");
                fprintf(logFile, "BACK\n");
                newDir = RIGHT;
            }
        }
        else { // currentDir == UP
            if (rowDiff > 0 && (globalDgist.map[currentPos.row + 1][currentPos.col].item.status != trap) && (0 <= currentPos.row + 1 && currentPos.row + 1 <= 4)) {
                printf("Command: STRAIGHT\n");
                fprintf(logFile, "STRAIGHT\n");
                newDir = UP;
            }
            else if (colDiff >= 0 && (globalDgist.map[currentPos.row][currentPos.col + 1].item.status != trap) && (0 <= currentPos.col + 1 && currentPos.col + 1 <= 4)) {
                printf("Command: LEFT\n");
                fprintf(logFile, "LEFT\n");
                newDir = RIGHT;
            }
            else if ((globalDgist.map[currentPos.row][currentPos.col - 1].item.status != trap) && (0 <= currentPos.col - 1 && currentPos.col - 1 <= 4)) {
                printf("Command: RIGHT\n");
                fprintf(logFile, "RIGHT\n");
                newDir = LEFT;
            }
            else {
                printf("Command: BACK\n");
                fprintf(logFile, "BACK\n");
                newDir = DOWN;
            }
        }

        currentDir = newDir;
    }

    fclose(logFile);
}



// 맵 업데이트를 위한 스레드 함수
void* updateMapThread(void* arg) {
    int client_socket = *(int*)arg;
    while (1) {
        // 서버로부터 맵 정보 수신
        int bytesRead = read(client_socket, &globalDgist, sizeof(DGIST));
        if (bytesRead <= 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.5");  // 서버 IP 주소
    server_addr.sin_port = htons(8373);  // 서버 포트 번호

    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // 맵 업데이트를 위한 스레드 생성
    pthread_t mapThread;
    if (pthread_create(&mapThread, NULL, updateMapThread, (void*)&client_socket) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    /*
    FILE* logFile = fopen("direction_log.txt", "w");
    if (logFile == NULL) {
        perror("fopen");
        return;
    }
    fprint("Command Initialtion");
    fprintf(logFile, "STRAIGHT\n");
    fclose(logfile);
    */

    while (1) {

        // 하트비트 신호 전송
        //char heartbeat = 'H';
        //send(client_socket, &heartbeat, sizeof(char), 0);

        int QR = readQRData(&currentPos);

        if (QR == 1) {

            determineDirection();

            // 이동 후 좌표와 방향 출력
            printf("Current Location: (%d, %d), Direction: ", currentPos.row, currentPos.col);
            switch (currentDir) {
            case RIGHT:
                printf("RIGHT\n");
                break;
            case DOWN:
                printf("DOWN\n");
                break;
            case LEFT:
                printf("LEFT\n");
                break;
            case UP:
                printf("UP\n");
                break;
            }

            // 서버로 이동 정보 전송
            ClientAction action;
            action.row = currentPos.row;
            action.col = currentPos.col;
            action.action = move;

            // 내 시작점이 (0,0)
            /*if ((currentPos.row == 1 && currentPos.col == 3) ||
                (currentPos.row == 0 && currentPos.col == 3) ||
                (currentPos.row == 3 && currentPos.col == 1) ||
                (currentPos.row == 3 && currentPos.col == 0)) {
                action.action = setBomb;
            }
            else {
                action.action = move;
            }*/

            if ((currentPos.row == 1 && currentPos.col == 4) ||
                (currentPos.row == 1 && currentPos.col == 3) ||
                (currentPos.row == 3 && currentPos.col == 1) ||
                (currentPos.row == 4 && currentPos.col == 1)) {
                action.action = setBomb;
            }
            else {
                action.action = move;
            }

            send(client_socket, &action, sizeof(ClientAction), 0);


            // 일정 시간 대기
        }
        sleep(0.5);
    }

    return 0;
}