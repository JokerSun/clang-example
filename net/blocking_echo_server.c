//
// Created by lql on 2020/3/17.
//

#include "constants.h"
#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <errno.h>
#include "blocking_echo_server.h"

void start_server(int port) {

    // define fd, address

    int socket_fd;
    struct sockaddr_in socket_address;

    bzero(&socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(port);

    // open socket

    if (fail(socket_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        printf("Open socket error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Open socket.\n");
    }

    // bind

    if (fail(bind(socket_fd, (struct sockaddr *) &socket_address, sizeof(socket_address)))) {
        printf("Bind error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Bind socket on '%d' port.\n", SERVER_PORT);
    }

    // listen

    if (fail(listen(socket_fd, BACKLOG))) {
        printf("Listen error (%s)\n", strerror(errno));
        return;
    } else {
        printf("Start listening.\n");
    }

    // accept

    while (true) {
        socklen_t address_len = sizeof(struct sockaddr);
        struct sockaddr client_address;
        int channel = accept(socket_fd, &client_address, &address_len);

        if (fail(channel)) {
            continue;
        }

        // run in other thread

        pthread_t t;
        pthread_create(&t, NULL, (void *) process, &channel);

    }

}

void process(const int *pfd) {

    int fd = *pfd;
    int read_len = 0;
    int write_len = 0;
    char buffer[BUFFER_SIZE];

    printf("Accept channel '%d'.\n", fd);

    while (true) {

        read_len = read(fd, buffer, BUFFER_SIZE);

        if (read_len < 0) {
            printf("Chanel (%d) Read error (%s).\n", fd, strerror(errno));
            break;
        }

        printf("Chanel (%d) Read  <----- %d bytes.\n", fd, read_len);

        write_len = write(fd, buffer, read_len);
        printf("Chanel (%d) Write -----> %d bytes.\n", fd, write_len);

    }

    close(fd);
    printf("Close channel '%d'.\n", fd);

}