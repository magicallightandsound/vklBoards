#include "BoardServer.h"
#include "BoardClient.h"

#include <iostream>

int main(int argc, char *argv[]){
	int port = 9000;
	BoardServer *server = NULL;
	if(argc > 1){
		if(argc > 2){
			port = atoi(argv[2]);
			server = new BoardServer(argv[1], port);
		}else{
			port = atoi(argv[1]);
			server = new BoardServer(port);
		}
	}else{
		server = new BoardServer(9000);
	}
	{
		std::string uri;
		server->get_uri(uri);
		printf("Server URI: %s:%d\n", uri.c_str(), port);
		fflush(stdout);
	}
	server->add_board(2048, 1024, "default");
	while(server->poll()){
	}
	return 0;
}
