#ifndef BOARD_CLIENT_H_INCLUDED
#define BOARD_CLIENT_H_INCLUDED

#define POCO_WIN32_UTF8
#include <string>
#include <vector>
#include "Poco/Net/StreamSocket.h"
#include "BoardServer.h"

class BoardClient{
	BoardServer::Connection connection;
	std::string server_uri;
public:
	typedef int board_index;
public:
	BoardClient();
	~BoardClient();
	
	int connect(const std::string &server_uri);
	int disconnect();
	int poll();
	int poll(BoardMessage::Type type, BoardMessage &msg);
	
	bool is_connected() const;
	void get_server(std::string &server_id);
	
	void get_boards(std::vector<std::string> &boards);
	void new_board(const std::string &title, unsigned width, unsigned height);
	int delete_board(board_index iboard);
	
	void get_size(board_index iboard, unsigned &width, unsigned &height);
	void get_contents(board_index iboard, unsigned char *img);
	void send_update(board_index iboard, unsigned char *img, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h);
	
	virtual void on_update(board_index iboard, const unsigned char *img, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h){}
private:
	void process_message(const BoardMessage &msg);
};

#endif // BOARD_CLIENT_H_INCLUDED
