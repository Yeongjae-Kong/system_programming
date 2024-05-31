#include <iostream>
#include <cmath>
#include <vector>
#include <queue>
#include <utility>
#include <limits>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// 서버에서 정의한 상수와 구조체
#define MAX_CLIENTS 2
#define _MAP_ROW 4
#define _MAP_COL 4
#define MAP_ROW _MAP_ROW + 1
#define MAP_COL _MAP_COL + 1
#define MAP_SIZE MAP_COL * MAP_ROW

const int MAX_SCORE = 4;
const int SETTING_PERIOD = 20;
const int INITIAL_ITEM = 10;
const int INITIAL_BOMB = 4;
const int SCORE_DEDUCTION = 2;

typedef struct {
    int socket;
    struct sockaddr_in address;
    int row;
    int col;
    int score;
    int bomb;
} client_info;

enum Status { nothing, item, trap };

typedef struct {
    enum Status status;
    int score;
} Item;

typedef struct {
    int row;
    int col;
    Item item;
} Node;

typedef struct {
    client_info players[MAX_CLIENTS];
    Node map[MAP_ROW][MAP_COL];
} DGIST;

enum Action { moveAction, setBomb };

typedef struct {
    int row;
    int col;
    enum Action action;
} ClientAction;

typedef pair<int, int> Coord;

int manhattanDistance(const Coord& a, const Coord& b) {
    return abs(a.first - b.first) + abs(a.second - b.second);
}

vector<Coord> findNearestItems(const ClientAction& myPos, const DGIST& mapInfo) {
    vector<Coord> nearestItems;
    int minDist = numeric_limits<int>::max();

    for (int row = 0; row < MAP_ROW; ++row) {
        for (int col = 0; col < MAP_COL; ++col) {
            if (mapInfo.map[row][col].item.status == item) {
                int dist = manhattanDistance({ myPos.row, myPos.col }, { row, col });
                if (dist < minDist) {
                    minDist = dist;
                    nearestItems.clear();
                    nearestItems.push_back({ row, col });
                }
                else if (dist == minDist) {
                    nearestItems.push_back({ row, col });
                }
            }
        }
    }

    return nearestItems;
}

vector<int> findPath(const Coord& start, const vector<Coord>& ends, const DGIST& mapInfo) {
    vector<int> shortestPath;
    int minPathLength = numeric_limits<int>::max();

    for (const Coord& end : ends) {
        vector<vector<bool>> visited(MAP_ROW, vector<bool>(MAP_COL, false));
        queue<pair<Coord, vector<int>>> q;
        q.push({ {start.first, start.second}, {} });
        visited[start.first][start.second] = true;

        vector<int> path;
        while (!q.empty()) {
            auto [curr, currPath] = q.front();
            q.pop();

            if (curr == end) {
                path = currPath;
                break;
            }

            static const int dr[] = { -1, 0, 1, 0 };
            static const int dc[] = { 0, 1, 0, -1 };
            static const int directions[] = { 2, 0, 1, 3 };
            for (int dir = 0; dir < 4; ++dir) {
                int newRow = curr.first + dr[dir];
                int newCol = curr.second + dc[dir];

                if (newRow >= 0 && newRow < MAP_ROW && newCol >= 0 && newCol < MAP_COL &&
                    !visited[newRow][newCol] && mapInfo.map[newRow][newCol].item.status != trap) {
                    visited[newRow][newCol] = true;
                    vector<int> newPath = currPath;
                    newPath.push_back(directions[dir]);
                    q.push({ {newRow, newCol}, newPath });
                }
            }
        }

        if (!path.empty() && static_cast<int>(path.size()) < minPathLength) {
            minPathLength = path.size();
            shortestPath = path;
        }
    }

    return shortestPath;
}

void printDGIST(const DGIST& mapInfo) {
    cout << "====== Map Information ======" << endl;
    for (int row = 0; row < MAP_ROW; ++row) {
        for (int col = 0; col < MAP_COL; ++col) {
            cout << "(" << row << ", " << col << ") ";
            switch (mapInfo.map[row][col].item.status) {
            case nothing:
                cout << "Nothing";
                break;
            case item:
                cout << "Item(" << mapInfo.map[row][col].item.score << ")";
                break;
            case trap:
                cout << "Trap";
                break;
            }
            cout << " ";
        }
        cout << endl;
    }
    cout << "=============================" << endl;
}

int main() {
    // 소켓 생성
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout << "Socket creation failed" << endl;
        return 1;
    }

    // 서버 주소 설정
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // 서버 포트 번호
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // 서버 IP 주소

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Connection failed" << endl;
        return 1;
    }

    // 서버로부터 맵 정보 수신
    DGIST mapInfo;
    int bytesReceived = recv(sockfd, &mapInfo, sizeof(DGIST), 0);
    if (bytesReceived <= 0) {
        cout << "Failed to receive map information" << endl;
        return 1;
    }

    // 수신한 맵 정보 출력
    printDGIST(mapInfo);

    // 클라이언트 위치 설정 (예시: (0, 0))
    ClientAction myPos = { 0, 0, moveAction };

    // 가장 가까운 아이템 좌표 계산
    vector<Coord> nearestItems = findNearestItems(myPos, mapInfo);
    if (nearestItems.empty()) {
        cout << "No item found" << endl;
    }
    else {
        cout << "Nearest item coordinates:" << endl;
        for (const Coord& item : nearestItems) {
            cout << "(" << item.first << ", " << item.second << ")" << endl;
        }
    }

    // 최단 경로 계산
    vector<int> path = findPath({ myPos.row, myPos.col }, nearestItems, mapInfo);

    // 경로 출력
    cout << "Shortest path from (" << myPos.row << ", " << myPos.col << ") to the nearest item:" << endl;
    for (int dir : path) {
        cout << dir << " ";
    }
    cout << endl;

    // 최단 경로 정보를 서버로 전송
    int bytesSent = send(sockfd, &path[0], path.size() * sizeof(int), 0);
    if (bytesSent <= 0) {
        cout << "Failed to send path information" << endl;
        return 1;
    }

    // 소켓 종료
    close(sockfd);

    return 0;
}