#ifndef BOARD_MESSAGE_H_INCLUDED
#define BOARD_MESSAGE_H_INCLUDED

#include <string>
#include <vector>

struct BoardMessage{
	enum Type{
		HANDSHAKE_CLIENT    = 0x0001, // send when a client connects to a server
		HANDSHAKE_SERVER    = 0x0002, // sent in response to HANDSHAKE_CLIENT
		CLIENT_DISCONNECT   = 0x0003, // sent by client to request disconnection
		CLIENT_CONNECTED    = 0x0004, // sent from server to announce a client connection
		CLIENT_DISCONNECTED = 0x0005, // sent from server to announce a client disconnection
		
		ENUMERATE_BOARDS    = 0x0010, // sent by client to ask for list of boards
		BOARD_ENUMERATION   = 0x0011, // server response to ENUMERATE_BOARDS
		BOARD_CREATE        = 0x0012, // sent by client to request creation of a new board
		BOARD_DELETE        = 0x0013, // sent by client to request creation of a new board
		
		BOARD_GET_SIZE      = 0x0020, // sent by client to query size of a board
		BOARD_SIZE          = 0x0021, // sent by server in response to BOARD_GET_SIZE
		BOARD_GET_CONTENTS  = 0x0022, // sent by client to get board contents, server response is BOARD_UPDATED
		
		BOARD_UPDATE        = 0x0030, // sent by client to update a board
		BOARD_UPDATED       = 0x0031, // server broadcast to send board updates
		
		INVALID = 0x0000
	};

	// 4 byte header:
	//   2 byte message type
	//   2 byte board id
	//   4 byte total message length (n + 8)
	//   n bytes payload
	uint16_t type_, id_;
	std::vector<unsigned char> payload;
	BoardMessage(){}
	BoardMessage(size_t len):payload(len){}
	BoardMessage(unsigned type, unsigned id):type_(type), id_(id){
	}
	size_t size() const{ return payload.size(); }
	uint16_t type() const{
		return type_;
	}
	uint16_t id() const{
		return id_;
	}
	uint16_t gets(size_t offset) const{
		return ntohs(*((uint16_t*)(&payload[offset])));
	}
	std::string getstring(size_t offset) const{
		return std::string((char*)&payload[offset]);
	};
	void adds(uint16_t val){
		size_t i = payload.size();
		payload.resize(i+2);
		*((uint16_t*)(&payload[i])) = htons(val);
	}
	void addstring(const std::string &str){
		size_t i = payload.size();
		size_t j = str.size()+1;
		payload.resize(i+j);
		memcpy(&payload[i], &str[0], j);
	}
	void addbytes(size_t len, const unsigned char *b){
		size_t i = payload.size();
		payload.resize(i+len);
		memcpy(&payload[i], b, len);
	}
	void addbytes(const std::vector<unsigned char> &b){
		addbytes(b.size(), &b[0]);
	}
};

#endif // BOARD_MESSAGE_H_INCLUDED
