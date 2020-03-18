#include "net/constants.h"
#include "net/io_multiplex_echo_server.h"

int main() {
    start_io_multi_echo_server(SERVER_PORT);
    return 0;
}
