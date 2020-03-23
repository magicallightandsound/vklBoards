#ifndef BOARD_SERVER_H_INCLUDED
#define BOARD_SERVER_H_INCLUDED

#define POCO_WIN32_UTF8
#include "Poco/Net/ServerSocket.h"
#include <string>
#include <vector>
#include "BoardMessage.h"

class BoardServer{
	friend class BoardClient;
	Poco::Net::ServerSocket socket;
	
	struct Connection{
		Poco::Net::StreamSocket socket;
		std::string id;
		std::vector<unsigned char> recvbuf;
		Connection(){}
		Connection(const Poco::Net::StreamSocket &sock):socket(sock){}
		int send(const BoardMessage &msg);
		int recv(BoardMessage &msg);
		bool can_recv();
		void close();
	};
	std::vector<Connection> connections;
	
	struct Board{
		std::vector<unsigned char> img;
		unsigned width, height;
		std::string title;
	};
	std::vector<Board*> boards;
	
	void process_message(size_t iconn, const BoardMessage &msg);
	void broadcast(const BoardMessage &msg, int iconn_exclude);
public:
	BoardServer(int port);
	BoardServer(const char *addr, int port);
	~BoardServer();
	
	void init();
	int add_board(unsigned width, unsigned height, const std::string &title);
	int poll(); // returns zero if no further polling should occur
	
	void get_uri(std::string &uri) const;
	
	static int encode(int method,
		const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
		std::vector<unsigned char> &buffer
	);
	static int decode(int method,
		const unsigned char *buffer, size_t buflen,
		unsigned char *rgb, unsigned stride, unsigned w, unsigned h
	);
};

#endif // BOARD_SERVER_H_INCLUDED
