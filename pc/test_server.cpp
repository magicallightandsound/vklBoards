#include "BoardServer.h"
#include "BoardClient.h"

#include <iostream>

int main(int argc, char *argv[]){
	BoardServer server(9000);
	server.add_board(2048, 1024, "default");
	while(server.poll()){
	}
	return 0;
}
