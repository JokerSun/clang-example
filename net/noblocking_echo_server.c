//
// Created by lql on 2020/3/18.
//

#include "constants.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <errno.h>
#include "utils.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static void process(const int *pfd);

static int nio_read(int fd, char *buffer, int size);

static int nio_write(int fd, char *buffer, int size);

void start_nio_server(int port) {

    printf("No blocking io echo server starting.\n");

    /* define fd, address */

    int socket_fd;
    struct sockaddr_in socket_address;

    bzero(&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(port);

    /* open socket and set type SOCK_STREAM and SOCK_NONBLOCK */

    if (fail(socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))) {
        printf("Open socket error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Open socket.\n");
    }

    /* bind */

    if (fail(bind(socket_fd, (struct sockaddr *) &socket_address, sizeof(socket_address)))) {
        printf("Bind error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Bind socket on '%d' port.\n", SERVER_PORT);
    }

    /* listen */

    if (fail(listen(socket_fd, BACKLOG))) {
        printf("Listen error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Start listening.\n");
    }

    /* accept */

    while (true) {
        socklen_t address_len = sizeof(struct sockaddr);
        struct sockaddr client_address;

        int channel;
        for (int i = 0; i < 100; i++) {
            if (9 == i) {
                /* if run 100 times then sleep 20ms */
                printf("No connection.\n");
                usleep(1000000);
                i = 0;
            } else {
                channel = accept(socket_fd, &client_address, &address_len);
                if (-1 != channel) {
                    break;
                }
            }
        }

        /* run in other thread */

        pthread_mutex_lock(&lock);

        pthread_t t;
        pthread_create(&t, NULL, (void *) process, &channel);

        pthread_cond_wait((pthread_cond_t *) &condition, &lock);
        pthread_mutex_unlock(&lock);
    }

}

static void process(const int *pfd) {
    pthread_mutex_lock(&lock);
    int fd = *pfd;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&lock);

    int read_len = 0;
    int write_len = 0;
    char buffer[BUFFER_SIZE];

    printf("Accept channel '%d'.\n", fd);

    while (true) {

        /* no blocking read */

        read_len = nio_read(fd, buffer, BUFFER_SIZE);

        if (read_len <= 0) {
            break;
        }

        /* no blocking write */
        nio_write(fd, buffer, read_len);

        bzero(buffer, BUFFER_SIZE);
    }

    close(fd);
    printf("Close chanel (%d).\n", fd);

}

static int nio_write(int fd, char *buffer, int size) {
    int flag;
    for (int i = 0; true; i++) {
        flag = send(fd, buffer, size, MSG_DONTWAIT);
        switch (flag) {
            case 0:
                printf("Chanel (%d) Connection closed.\n", fd);
                return 0;
            case -1 :
                if (EAGAIN != errno) {
                    /* read error */
                    printf("Chanel (%d) Write error (%s).\n", fd, strerror(errno));
                    return -1;
                }
                /* message temporarily unavailable */
                if (0 == i % 1000) {
                    printf("Chanel (%d) Connection temporarily unwritable.\n", fd);
                    usleep(200000);
                }
            default:
                printf("Chanel (%d) Write ------> %d bytes.\n", fd, flag);
                return flag;
        }
    }
}

static int nio_read(int fd, char *buffer, int size) {
    int flag;
    for (int i = 0; true; i++) {
        flag = recv(fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);
        if (0 == flag) {
            /* no message to read */
            printf("Chanel (%d) Connection closed.\n", fd);
            return 0;
        } else if (-1 == flag) {
            if (EAGAIN != errno) {
                /* read error */
                printf("Chanel (%d) Read error (%s).\n", fd, strerror(errno));
                break;
            }
            /* message temporarily unavailable */
            if (0 == i % 1000) {
                printf("Chanel (%d) Message temporarily unavailable.\n", fd);
                usleep(200000);
            }
        } else {
            printf("Chanel (%d) Read  <----- %d bytes.\n", fd, flag);
            return flag;
        }
    }

    return -1;
}