//
// Created by lql on 2020/3/20.
//

#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <fcntl.h>
#include "../include/constants.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/sendfile.h>
#include "../include/utils.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static void process(const int *pfd);

void start_splice_pipe_server(int port) {

    printf("No blocking io echo server starting.\n");

    /* define fd, address */

    int socket_fd;
    struct sockaddr_in socket_address;

    bzero(&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(port);

    /* open socket and set type SOCK_STREAM and SOCK_NONBLOCK */

    if (fail(socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP))) {
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

    printf("Accept channel '%d'.\n", fd);

//    /* set socket busy poll */
//    int val = 10 * 1000; // 10ms
//    int return_value = setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &val, sizeof(val));
//    if (-1 == return_value) {
//        printf("Set socket busy poll error (%s).\n", strerror(errno));
//        goto final;
//    }

    /* create pipe */

#define SPLICE_MAX (512*1024)

    int n;

    int pipe_fd[2];
    pipe(pipe_fd);

    fcntl(pipe_fd[0], F_SETPIPE_SZ, SPLICE_MAX);

    while (true) {

        n = splice(fd, NULL, pipe_fd[1], NULL, SPLICE_MAX, SPLICE_F_MOVE);

        if (n < 0) {
            printf("Splice read error (%d:%s).\n", errno, strerror(errno));
            break;
        }

        if (n == 0) {
            printf("No message to transfer.\n");
            break;
        }

        printf("Chanel (%d) Read  <----- %d bytes.\n", fd, n);

        n = splice(pipe_fd[0], NULL, fd, NULL, n, SPLICE_F_MOVE);

        if (n < 0) {
            printf("Splice write error (%s).\n", strerror(errno));
            break;
        }

        printf("Chanel (%d) Write -----> %d bytes.\n", fd, n);

    }

    final:
    {
        close(fd);
        printf("Close chanel (%d).\n", fd);
    }

}