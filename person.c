#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>

#define BUF_SIZE 1024

int sock;

// Thread to receive messages from server
void *recv_messages(void *arg) {
    char buf[BUF_SIZE];
    while (1) {
        int bytes = recv(sock, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0) break;
        buf[bytes] = '\0';
        printf("%s\n", buf);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    pthread_t tid;
    pthread_create(&tid, NULL, recv_messages, NULL);

    char buf[BUF_SIZE];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strcmp(buf, "quit\n") == 0) break;
        send(sock, buf, strlen(buf), 0);
    }

    close(sock);
    return 0;
}
