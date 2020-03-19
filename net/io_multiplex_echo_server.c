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
#include <sys/poll.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static void process(const int *pfd);

static int poll_read(int fd, char *buffer, int size);

static int poll_write(int fd, char *buffer, int size);

static int select_read(int fd, char *buffer, int size);

static int select_write(int fd, char *buffer, int size);

void start_io_multi_echo_server(int port) {

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

    socklen_t address_len = sizeof(struct sockaddr);
    struct sockaddr client_address;
    int channel;

    struct pollfd poll_fd;
    poll_fd.fd = socket_fd;
    poll_fd.events = POLLIN;

    while (true) {

        /* poll */

        printf("Poll.\n");

        int flag = poll(&poll_fd, 1, 5000);

        if (-1 == flag) {
            printf("Poll error (%s).\n", strerror(errno));
            continue;
        }

        if (0 == flag) {
            printf("Poll time out.\n");
            continue;
        }

        if (!poll_fd.revents & POLLIN) {
            printf("Invalid revent(%d).\n", poll_fd.revents);
            continue;
        }

        /* accept */

        channel = accept(socket_fd, &client_address, &address_len);

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

        /* poll read */
        read_len = select_read(fd, buffer, BUFFER_SIZE);

        if (read_len <= 0) {
            break;
        }

        /* poll write */
        select_write(fd, buffer, read_len);

        bzero(buffer, BUFFER_SIZE);
    }

    close(fd);
    printf("Close chanel (%d).\n", fd);

}

static int poll_read(int fd, char *buffer, int size) {

    struct pollfd poll_fd;
    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    int flag;

    while (true) {
        /* poll */
        flag = poll(&poll_fd, 1, 5000);

        if (-1 == flag) {
            printf("Poll read error (%s).\n", strerror(errno));
            return -1;
        }

        if (0 == flag) {
            printf("Poll read time out.\n");
            continue;
        }

        if (!poll_fd.revents & POLLIN) {
            printf("Invalid revent(%d).\n", poll_fd.revents);
            continue;
        }

        int read_len = read(fd, buffer, size);
        printf("Chanel (%d) Read  <----- %d bytes.\n", fd, read_len);
        return read_len;
    }

}

static int poll_write(int fd, char *buffer, int size) {
    struct pollfd poll_fd;
    poll_fd.fd = fd;
    poll_fd.events = POLLOUT;
    int flag;

    while (true) {
        /* poll */
        flag = poll(&poll_fd, 1, 5000);

        if (-1 == flag) {
            printf("Poll write error (%s).\n", strerror(errno));
            return -1;
        }

        if (0 == flag) {
            printf("Poll write time out.\n");
            continue;
        }

        if (!poll_fd.revents & POLLOUT) {
            printf("Invalid revent(%d).\n", poll_fd.revents);
            continue;
        }

        int write_len = write(fd, buffer, size);
        printf("Chanel (%d) Write -----> %d bytes.\n", fd, write_len);
        return write_len;

    }
}

static int select_read(int fd, char *buffer, int size) {
    struct timespec time_spec;
    time_spec.tv_sec = 5; // 5sec
    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(fd, &fdSet);

    while (true) {

        int retval = pselect(fd + 1, &fdSet, NULL, NULL, &time_spec, NULL);

        if (-1 == retval) {
            printf("Select read error (%s).\n", strerror(errno));
            return -1;
        }

        if (0 == retval) {
            printf("Select read time out.\n");
            continue;
        }

        int read_len = read(fd, buffer, size);
        printf("Chanel (%d) Read  <----- %d bytes.\n", fd, read_len);
        return read_len;
    }

}

static int select_write(int fd, char *buffer, int size) {
    struct timespec time_spec;
    time_spec.tv_sec = 5; // 5sec
    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(fd, &fdSet);

    while (true) {

        int retval = pselect(fd + 1, NULL, &fdSet, NULL, &time_spec, NULL);

        if (-1 == retval) {
            printf("Select write error (%s).\n", strerror(errno));
            return -1;
        }

        if (0 == retval) {
            printf("Select write time out.\n");
            continue;
        }

        int write_len = write(fd, buffer, size);
        printf("Chanel (%d) Write -----> %d bytes.\n", fd, write_len);
        return write_len;
    }
}