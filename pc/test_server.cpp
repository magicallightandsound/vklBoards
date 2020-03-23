#include "BoardServer.h"
#include "BoardClient.h"

#include <iostream>

int main(int argc, char *argv[]){
	int port = 9000;
	if(argc > 1){
		port = atoi(argv[1]);
	}
	BoardServer server(port);
	{
		std::string uri;
		server.get_uri(uri);
		printf("Server URI: %s:%d\n", uri.c_str(), port);
		fflush(stdout);
	}
	server.add_board(2048, 1024, "default");
	while(server.poll()){
	}
	return 0;
}
