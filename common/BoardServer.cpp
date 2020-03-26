#include "BoardServer.h"
#include "ImageCoder.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/NetException.h"
#include "Poco/StreamCopier.h"
#include "Poco/Timespan.h"
#include "Poco/FileStream.h"
#include "Poco/Net/DNS.h"
#include <iostream>
#include <sstream>
#include <cstdarg>

#ifdef DEBUG_SERVER
static void dbgmsg(const char *fmt, ...){
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fflush(stdout);
}
static void msgdump(const BoardMessage &msg){
	dbgmsg(" type(%04x) id(%04x)", msg.type(), msg.id());
	size_t n = msg.size();
	if(n > 16){ n = 16; }
	for(size_t i = 0; i < n; ++i){
		dbgmsg(" %02x", msg.payload[i]);
	}
	if(msg.size() > 16){ printf(" ..."); }
	dbgmsg("\n");
}
#else
# define dbgmsg(FMT, ...) do{}while(0)
# define msgdump(MSG) do{}while(0)
#endif


int BoardServer::Connection::send(const BoardMessage &msg){
	dbgmsg("Sending:\n");
	msgdump(msg);
	
	int ret;
	uint16_t s;
	uint32_t l;
	s = htons(msg.type());
	ret = socket.sendBytes(&s, 2, 0);
	s = htons(msg.id());
	ret = socket.sendBytes(&s, 2, 0);
	l = htonl(8 + msg.payload.size());
	ret = socket.sendBytes(&l, 4, 0);
	return socket.sendBytes(&msg.payload[0], msg.payload.size(), 0);
}
int BoardServer::Connection::recv(BoardMessage &msg){
	int p = recvbuf.size();
	int len = socket.available();
	if(len > 0){
		recvbuf.resize(p + len);
		socket.receiveBytes(&recvbuf[p], len, 0);
	}
	if(recvbuf.size() >= 8){
		int expected_size = ntohl(*((uint32_t*)(&recvbuf[4])));
		if(recvbuf.size() < expected_size){
			recvbuf.reserve(expected_size);
			return 0;
		}
		// got a complete message
		msg.type_ = ntohs(*((uint16_t*)(&recvbuf[0])));
		msg.id_   = ntohs(*((uint16_t*)(&recvbuf[2])));
		msg.payload.resize(expected_size-8);
		memcpy(&msg.payload[0], &recvbuf[8], expected_size-8);
		recvbuf.erase(recvbuf.begin(), recvbuf.begin()+expected_size);
		dbgmsg("Received:\n");
		msgdump(msg);
		return 1;
	}
	return 0;
}
bool BoardServer::Connection::can_recv(){
	Poco::Timespan span(1000);
	return socket.poll(span, Poco::Net::Socket::SELECT_READ) || (recvbuf.size() >= 8);
}

void BoardServer::Connection::close(){
	socket.close();
}

BoardServer::BoardServer(int port):
	socket(port)
{
}
BoardServer::BoardServer(const char *addr, int port):
	socket(Poco::Net::SocketAddress(std::string(addr), port))
{
}

BoardServer::~BoardServer(){
}

int BoardServer::add_board(unsigned width, unsigned height, const std::string &title){
	int ret = boards.size();
	boards.push_back(new Board());
	boards.back()->width = width;
	boards.back()->height = height;
	boards.back()->img.resize(3*width*height);
	boards.back()->title = title;
	memset(&boards.back()->img[0], 0xff, 3*width*height);
	return ret;
}

int BoardServer::poll(){
	Poco::Timespan span(100);
	if(socket.poll(span, Poco::Net::Socket::SELECT_READ)){
		dbgmsg("Client connecting...");
		Poco::Net::StreamSocket strs = socket.acceptConnection();
		std::cout << "Client connected: " << strs.peerAddress().toString() << std::endl;
		connections.push_back(BoardServer::Connection(strs));
	}
	for(size_t iconn = 0; iconn < connections.size(); ++iconn){
		BoardServer::Connection &conn = connections[iconn];
		try{
			if(conn.can_recv()){
				BoardMessage msg;
				if(conn.recv(msg)){
					process_message(iconn, msg);
				}
			}
		}catch(Poco::Net::ConnectionResetException cre){
			// ignore for now
		}
	}
	return 1; // request for continued polling
}

void BoardServer::get_uri(std::string &uri) const{
	try{
		const Poco::Net::HostEntry& entry = Poco::Net::DNS::thisHost();
		const Poco::Net::HostEntry::AddressList& addrs = entry.addresses();
		Poco::Net::HostEntry::AddressList::const_iterator addr_it = addrs.begin();
		for (; addr_it != addrs.end(); ++addr_it){
			if(addr_it->isLinkLocal() || addr_it->isLinkLocalMC() || addr_it->isLoopback()){ continue; }
			uri = addr_it->toString();
		}
	}catch(Poco::Net::HostNotFoundException e){
	}catch(Poco::Net::NoAddressFoundException e){
	}catch(Poco::Net::DNSException e){
	}catch(Poco::IOException e){
	} 
}

void BoardServer::broadcast(const BoardMessage &msg, int iconn_exclude){
	for(int iconn = 0; iconn < connections.size(); ++iconn){
		if(iconn == iconn_exclude){ continue; }
		connections[iconn].send(msg);
	}
}

void BoardServer::process_message(size_t iconn, const BoardMessage &msg){
	BoardServer::Connection &conn = connections[iconn];
	const size_t msgsize = msg.size();
	switch(msg.type()){
	case BoardMessage::HANDSHAKE_CLIENT:
		{
			if(1 != msg.id()){
				dbgmsg("Client msg id expected 1, received: %u", (unsigned int)msg.id());
				return;
			}
			size_t len = msgsize;
			if(len > 256){
				len = 256;
			}
			{
				std::string str = msg.getstring(0);
				conn.id = str.c_str();
			}
			// Send handshake response
			BoardMessage resp(BoardMessage::HANDSHAKE_SERVER, 1);
			conn.send(resp);
			// Send connection announcement
			BoardMessage announce(BoardMessage::CLIENT_CONNECTED, 0);
			announce.addstring(conn.id);
			broadcast(announce, iconn);
			
			dbgmsg("Client connected: %s", conn.id.c_str());
		}
		break;
	case BoardMessage::CLIENT_DISCONNECT:
		{
			dbgmsg("Client disconnected: %s", conn.id.c_str());
			std::cout << "Client disconnected: " << conn.socket.peerAddress().toString() << std::endl;
			
			// Send disconnection announcement
			BoardMessage announce(BoardMessage::CLIENT_DISCONNECTED, 0);
			announce.addstring(conn.id);
			broadcast(announce, iconn);
			conn.close();
			connections.erase(connections.begin()+iconn);
		}
		break;
	case BoardMessage::ENUMERATE_USERS:
		{
			BoardMessage resp(BoardMessage::USER_ENUMERATION, connections.size());
			for(size_t i = 0; i < connections.size(); ++i){
				resp.addstring(connections[i].id);
			}
			conn.send(resp);
		}
		break;
	case BoardMessage::ENUMERATE_BOARDS:
		{
			BoardMessage resp(BoardMessage::BOARD_ENUMERATION, boards.size());
			for(size_t i = 0; i < boards.size(); ++i){
				resp.addstring(boards[i]->title);
			}
			conn.send(resp);
		}
		break;
	case BoardMessage::BOARD_CREATE:
		{
			std::string title;
			size_t len = msgsize;
			if(len > 256){
				len = 256;
			}
			title = msg.getstring(0);
			add_board(2048, 1024, title);
			
			dbgmsg("Board created: %d, title = %s", (int)(boards.size()-1), title.c_str());
			
			BoardMessage resp(BoardMessage::BOARD_ENUMERATION, boards.size());
			for(size_t i = 0; i < boards.size(); ++i){
				resp.addstring(boards[i]->title);
			}
			broadcast(resp, -1);
		}
		break;
	case BoardMessage::BOARD_DELETE:
		{
			// TODO
		}
		break;
	case BoardMessage::BOARD_GET_SIZE:
		{
			unsigned iboard = msg.id();
			BoardMessage resp(BoardMessage::BOARD_SIZE, iboard);
			if(iboard < boards.size()){
				resp.adds(boards[iboard]->width);
				resp.adds(boards[iboard]->height);
			}else{
				resp.adds(0);
				resp.adds(0);
			}
			conn.send(resp);
		}
		break;
	case BoardMessage::BOARD_GET_CONTENTS:
		{
			int method = 1;
			unsigned iboard = msg.id();
			BoardMessage resp(BoardMessage::BOARD_UPDATED, iboard);
			if(iboard < boards.size()){
				resp.adds(boards[iboard]->width);
				resp.adds(boards[iboard]->height);
				resp.adds(0); // x offset
				resp.adds(0); // y offset
				resp.adds(method); // encoding
				ImageCoder::encode(method,
					&boards[iboard]->img[0], boards[iboard]->width, boards[iboard]->width, boards[iboard]->height,
					resp.payload
				);
			}else{
				resp.adds(0);
				resp.adds(0);
				resp.adds(0);
				resp.adds(0);
				resp.adds(0);
			}
			conn.send(resp);
		}
		break;
	case BoardMessage::BOARD_UPDATE:
		{
			unsigned iboard = msg.id();
			if(iboard >= boards.size()){ return; }
			Board &board = *boards[iboard];
			
			unsigned w = msg.gets(0);
			unsigned h = msg.gets(2);
			unsigned x = msg.gets(4);
			unsigned y = msg.gets(6);
			unsigned enc = msg.gets(8);
			if(0 == enc){
				size_t expected_msg_size = 10+3*w*h;
				if(msg.size() < expected_msg_size){ return; }
			}
			// Window clamping
			if(x >= board.width){ x = board.width-1; }
			if(y >= board.height){ y = board.height-1; }
			if(x + w > board.width){ w = board.width-x; }
			if(y + h > board.height){ h = board.height-y; }
			
			ImageCoder::decode(enc,
				&msg.payload[10], msg.payload.size()-10,
				&board.img[3*(x+y*board.width)], board.width, w, h
			);
			
			// Compose response
			BoardMessage resp(BoardMessage::BOARD_UPDATED, iboard);
			resp.payload = msg.payload;
			broadcast(resp, iconn);
			dbgmsg("Board updated: %d", iboard);
		}
		break;
	default:
		return;
	}
}
