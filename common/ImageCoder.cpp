#include "ImageCoder.h"
#include <cstring>
#include <cstdio>
#include "fastlz.h"

typedef int (*encoderproc)(
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
);
typedef int (*decoderproc)(
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
);

int raw_enc(
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
);
int raw_dec(
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
);
int rle_enc(
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
);
int rle_dec(
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
);

struct endecpair{
	encoderproc encoder;
	decoderproc decoder;
};

endecpair endec[2] = {
	{ &raw_enc, &raw_dec },
	{ &rle_enc, &rle_dec }
};

int ImageCoder::encode(int method,
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
){
	if(method < 0 || method > 1){ return -1; }
	return endec[method].encoder(rgb, stride, w, h, buffer);
}

int ImageCoder::decode(int method,
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
){
	if(method < 0 || method > 1){ return -1; }
	return endec[method].decoder(buffer, buflen, rgb, stride, w, h);
}


int raw_enc(
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
){
	size_t off = buffer.size();
	buffer.resize(off + 3*w*h);
	unsigned char *dst = &buffer[off];
	const unsigned char *row = rgb;
	for(unsigned j = 0; j < h; ++j){
		memcpy(dst, row, 3*w);
		row += 3*stride;
		dst += 3*w;
	}
	return 0;
}
int raw_dec(
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
){
	if(3*w*h != buflen){ return -2; }
	unsigned char *dst = rgb;
	const unsigned char *row = &buffer[0];
	for(unsigned j = 0; j < h; ++j){
		memcpy(dst, row, 3*w);
		dst += 3*stride;
		row += 3*w;
	}
	return 0;
}

unsigned encode_byte_continuation(
	unsigned int val, 
	std::vector<unsigned char> &buffer
){
	unsigned count = 0;
	while(val > 127){
		buffer.push_back(0x80 | (val & 0x7F));
		val >>= 7;
		++count;
	}
	buffer.push_back(val);
	++count;
	return count;
}
unsigned int decode_byte_continuation(
	const unsigned char *buffer, unsigned buflen
){
	unsigned int val = 0;
	unsigned int shift = 0;
	while(buflen --> 0){
		val |= (((*buffer) & 0x7F) << shift);
		if(0 == (0x80 & (*buffer))){ break; }
		++buffer;
		shift += 7;
	}
	return val;
}

int rle_enc(
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
){
	size_t off = buffer.size();
	/*
	const unsigned char *row = rgb;
	for(unsigned j = 0; j < h; ++j){
		const unsigned char *ptr = row;
		// Push first pixel
		buffer.push_back(ptr[0]);
		buffer.push_back(ptr[1]);
		buffer.push_back(ptr[2]);
		unsigned count = 1;
		for(unsigned int i = 1; i < w; ++i){
			if(
				ptr[0] == buffer[off+0] &&
				ptr[1] == buffer[off+1] &&
				ptr[2] == buffer[off+2]
			){ // run of length 2 or more
				count++;
			}else{ // run ended
				if(count > 1){
					off += encode_byte_continuation(count, buffer);
				}
				buffer.push_back(ptr[0]);
				buffer.push_back(ptr[1]);
				buffer.push_back(ptr[2]);
				off += 3;
				count = 1;
			}
			ptr += 3;
		}
		if(count > 1){
			off += encode_byte_continuation(count, buffer);
		}
		
		// Determine row repeat and append it
		unsigned rowrep = 1;
		unsigned jnext = j+1;
		const unsigned char *rownext = row+1;
		while(jnext < h && 0 == memcmp(row, rownext, 3*w)){
			jnext++;
			rownext += 3*stride;
			rowrep++;
		}
		off += encode_byte_continuation(rowrep, buffer);
		row = rownext;
		
		j = jnext-1; // subtract 1 to compensate for loop update
	}
	*/
	
	size_t bufsize = 3*w*h + (3*w*h+19/20);
	buffer.resize(off + bufsize);
	int sz;
	if(stride == w){
		sz = fastlz_compress_level(2, rgb, 3*w*h, &buffer[off]);
	}else{
		std::vector<unsigned char> buf(3*w*h);
		unsigned char *row = &buf[0];
		for(unsigned j = 0; j < h; ++j){
			memcpy(row, rgb, 3*w);
			rgb += 3*stride;
			row += 3*w;
		}
		sz = fastlz_compress_level(2, &buf[0], 3*w*h, &buffer[off]);
	}
	buffer.resize(off+sz);
	
	return 0;
}
int rle_dec(
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
){
	int sz;
	if(stride == w){
		sz = fastlz_decompress(buffer, buflen, rgb, 3*w*h);
	}else{
		std::vector<unsigned char> buf(3*w*h);
		sz = fastlz_decompress(buffer, buflen, &buf[0], 3*w*h);
		unsigned char *row = &buf[0];
		for(unsigned j = 0; j < h; ++j){
			memcpy(rgb, row, 3*w);
			rgb += 3*stride;
			row += 3*w;
		}
	}
	/*
	size_t off = 0;
	unsigned char *row = rgb;
	for(unsigned j = 0; j < h; ++j){
		unsigned char *ptr = row;
		unsigned rowcount = 1;
		ptr[0] = buffer[off+0];
		ptr[1] = buffer[off+1];
		ptr[2] = buffer[off+2];
		...
		while(rowcount < w){
			unsigned count = buffer[off+3];
			for(unsigned i = 0; i < count; ++i){
				if(rowcount >= w){ break; } // error condition
				ptr[0] = buffer[off+0];
				ptr[1] = buffer[off+1];
				ptr[2] = buffer[off+2];
				ptr += 3;
				++rowcount;
			}
			off += 4;
		}
		row += 3*stride;
	}
	*/
	return 0;
}
