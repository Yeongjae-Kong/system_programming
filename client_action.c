#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

struct ClientAction {
    int row;
    int col;
    int trap;
};

void send_action_to_server(const char *server_addr, int server_port, int row, int col, int trap) {
    int sockfd;
    struct sockaddr_in server_addr_struct;

    struct ClientAction action;
    action.row = row;
    action.col = col;
    action.trap = trap;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    server_addr_struct.sin_family = AF_INET;
    server_addr_struct.sin_port = htons(server_port);
    server_addr_struct.sin_addr.s_addr = inet_addr(server_addr);
    memset(server_addr_struct.sin_zero, '\0', sizeof server_addr_struct.sin_zero);

    if (connect(sockfd, (struct sockaddr *)&server_addr_struct, sizeof(server_addr_struct)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    if (send(sockfd, &action, sizeof(struct ClientAction), 0) == -1) {
        perror("send");
        close(sockfd);
        exit(1);
    }

    close(sockfd);
}
