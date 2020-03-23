#include "BoardClient.h"
#include "BoardMessage.h"
#include "ImageCoder.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/Timespan.h"
#include "Poco/FileStream.h"
#include "Poco/Net/DNS.h"
#include <iostream>

BoardClient::BoardClient()
{
}

BoardClient::~BoardClient(){
	if(is_connected()){
		disconnect();
	}
}

int BoardClient::connect(const std::string &uri){
	std::string id("user");
	try{
		connection.socket = Poco::Net::StreamSocket(Poco::Net::SocketAddress(uri));
		BoardMessage msg(BoardMessage::HANDSHAKE_CLIENT, 1);
		msg.addstring(id);
		connection.send(msg);
		server_uri = uri;
	}catch(Poco::Net::ConnectionRefusedException cre){
		return -1;
	}
	return 0;
}

bool BoardClient::is_connected() const{
	return connection.socket.impl()->initialized();
}
void BoardClient::get_server(std::string &server_id){
	server_id = server_uri;
}

int BoardClient::disconnect(){
	BoardMessage msg(BoardMessage::CLIENT_DISCONNECT, 0);
	connection.send(msg);
	connection.close();
	return 0;
}

void BoardClient::get_boards(std::vector<std::string> &boards){
	BoardMessage msg(BoardMessage::ENUMERATE_BOARDS, 0);
	connection.send(msg);
	BoardMessage resp;
	if(poll(BoardMessage::BOARD_ENUMERATION, resp)){
		unsigned off = 0;
		unsigned n = resp.id();
		boards.clear();
		boards.reserve(n);
		for(unsigned i = 0; i < n; ++i){
			std::string str = resp.getstring(off);
			off += str.size()+1;
			boards.push_back(str);
		}
	}
}

void BoardClient::new_board(const std::string &title, unsigned width, unsigned height){
	BoardMessage msg(BoardMessage::BOARD_CREATE, 0);
	msg.addstring(title);
	connection.send(msg);
}
int BoardClient::delete_board(BoardClient::board_index iboard){
	return 0;
}

void BoardClient::get_size(BoardClient::board_index iboard, unsigned &width, unsigned &height){
	BoardMessage msg(BoardMessage::BOARD_GET_SIZE, iboard);
	connection.send(msg);
	BoardMessage resp;
	if(poll(BoardMessage::BOARD_SIZE, resp)){
		if(msg.id() == iboard && resp.size() >= 4){
			width = resp.gets(0);
			height = resp.gets(2);
		}
	}
}
void BoardClient::get_contents(BoardClient::board_index iboard, unsigned char *img){
	BoardMessage msg(BoardMessage::BOARD_GET_CONTENTS, iboard);
	connection.send(msg);
	BoardMessage resp;
	if(poll(BoardMessage::BOARD_UPDATED, resp)){
		process_message(resp); // punt
	}
}
void BoardClient::send_update(BoardClient::board_index iboard, unsigned char *img, unsigned stride, unsigned x, unsigned y, unsigned w, unsigned h){
	int method = 1;
	BoardMessage msg(BoardMessage::BOARD_UPDATE, iboard);
	msg.adds(w);
	msg.adds(h);
	msg.adds(x);
	msg.adds(y);
	msg.adds(method);
	ImageCoder::encode(method,
		&img[3*(x+y*stride)], stride, w, h,
		msg.payload
	);
	connection.send(msg);
}

int BoardClient::poll(){
	if(connection.can_recv()){
		BoardMessage msg;
		connection.recv(msg);
		process_message(msg);
		/*
		int len = connection.socket.available();
		std::vector<char> buffer(len);
		connection.socket.receiveBytes(&buffer[0], len, 0);
		std::string str(buffer.begin(), buffer.end());
		std::cout << "Received " << str << std::endl;
		*/
	}
	return 1;
}
int BoardClient::poll(BoardMessage::Type type, BoardMessage &msg){
	printf("Looking for type %02x\n", type);
	while(1)
	while(connection.can_recv()){
		if(connection.recv(msg)){
			if(msg.type() == type){ return 1; }
			process_message(msg);
		}
	}
	return 0;
}

void BoardClient::process_message(const BoardMessage &msg){
	if(msg.type() == BoardMessage::BOARD_UPDATED && msg.size() >= 10){
		unsigned w = msg.gets(0);
		unsigned h = msg.gets(2);
		unsigned x = msg.gets(4);
		unsigned y = msg.gets(6);
		unsigned enc = msg.gets(8);
		on_update(msg.id(), enc, &msg.payload[10], msg.payload.size()-10, x, y, w, h);
	}else if(msg.type() == BoardMessage::HANDSHAKE_SERVER){
	}
}
