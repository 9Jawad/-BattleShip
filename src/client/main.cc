#include "client.hh"

int main(){

    shared_ptr<BoardLite> board = make_shared<BoardLite>();
    GridDisplay grid_display{cout, board};
    Client client{&grid_display};
    client.start_client();

    printf("Disconnected from the server \n");

    return 0;
}
