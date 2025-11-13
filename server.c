// Grabbing in the libraries needed to create the server
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

// defining the variables 
#define BACKLOG 10
#define BUF_SIZE 1024
#define NAME_LEN 32

// making the struct of our person and 
typedef struct person{
    int sock;
    char name[NAME_LEN];
    struct person *next;
}person_t;

// 
static person_t *person = NULL;
static pthread_mutex_t person_mtx = PTHREAD_MUTEX_INITIALIZER;
static int server_sock = -1;

// creating the timestamps for the logging feature 
void print_log(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    vprintf(fmt, ap);
    printf("\n");
    fflush(stdout);
    va_end(ap);
}

// adding in a new person  to the linked list
void add_person(person_t *p) {
    pthread_mutex_lock(&person_mtx);
    p->next = person;
    person = p;
    pthread_mutex_unlock(&person_mtx);
}

// Removing a person
void remove_person(int sock) {
    pthread_mutex_lock(&person_mtx);
    person_t **curr = &person;

    while (*curr) {
        if ((*curr)->sock == sock) {
            person_t *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            break;
        }
        curr = &(*curr)->next;
    }

    pthread_mutex_unlock(&person_mtx);
}

// Broadcast a message to all clients except sender
void broadcast_message(const char *msg, int sender_sock) {
    pthread_mutex_lock(&person_mtx);
    person_t *curr = person;

    while (curr) {
        if (curr->sock != sender_sock) {
            send(curr->sock, msg, strlen(msg), 0);
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&person_mtx);
}

// Handle each client in a separate thread
void *handle_client(void *arg) {
    person_t *p = (person_t *)arg;
    char buf[BUF_SIZE];

    while (1) {
        int bytes_received = recv(p->sock, buf, sizeof(buf) - 1, 0);
        if (bytes_received <= 0) break; // Client disconnected

        buf[bytes_received] = '\0';
        print_log("%s: %s", p->name, buf);
        broadcast_message(buf, p->sock);
    }

    print_log("%s disconnected", p->name);
    remove_person(p->sock);
    close(p->sock);
    return NULL;
}

// Graceful shutdown on Ctrl+C
void handle_sigint(int sig) {
    print_log("Shutting down server...");
    close(server_sock);

    pthread_mutex_lock(&person_mtx);
    person_t *curr = person;
    while (curr) {
        close(curr->sock);
        person_t *tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    pthread_mutex_unlock(&person_mtx);

    exit(0);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_sigint); // handle Ctrl+C

    int port = atoi(argv[1]);

    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, BACKLOG) < 0) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    print_log("Server listening on port %d", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        // Allocate client struct
        person_t *p = malloc(sizeof(person_t));
        p->sock = client_sock;
        snprintf(p->name, NAME_LEN, "User%d", client_sock);
        add_person(p);

        print_log("New client connected: %s", p->name);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, p);
        pthread_detach(tid);
    }

    return 0;
}
