#ifndef IMAGE_CODER_H_INCLUDED
#define IMAGE_CODER_H_INCLUDED

#include <vector>

namespace ImageCoder{

int encode(int method,
	const unsigned char *rgb, unsigned stride, unsigned w, unsigned h,
	std::vector<unsigned char> &buffer
);

int decode(int method,
	const unsigned char *buffer, unsigned buflen,
	unsigned char *rgb, unsigned stride, unsigned w, unsigned h
);

} // namespace ImageCoder

#endif // IMAGE_CODER_H_INCLUDED
