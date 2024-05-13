#include "server.hh"
#include "database.hh"

int main() {
    
    Server server;
    // Connect to the server
    server.setup_server();


    // signal(SIGPIPE, signal_handler);
    // signal(SIGINT, signal_handler);

    server.start_server();

    return 0;
}
