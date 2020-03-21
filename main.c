#include "include/constants.h"
#include "include/splice_zero_copy_echo_server.h"

int main() {
    start_splice_pipe_server(SERVER_PORT);
    return 0;
}
