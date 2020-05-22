//
//  compression.cpp
//  IFF compression methods.
//
//  Created by Ronny Bangsund on 04/11/2012.
//  Copyright (c) 2012 Neural Short-Circuit. All rights reserved.
//

#include "iff.h"
#include "zlib.h"

namespace IFFSpace
{
	using namespace std;

	bool Chunk::WriteDataZlib(fstream *f)
	{
		#define BUFSIZE 128 * 1024
		char buf[BUFSIZE];
		uint64_t realsize = size;	// Uncompressed size
		pos = (uint64_t)f->tellp();
		// First uint64 of the data is the uncompressed size (little endian).
		size = 8;
		f->write((const char *)&size, 8);

		z_stream z;
		z.zalloc = 0;
		z.zfree = 0;
		if(deflateInit(&z, Z_BEST_COMPRESSION) != Z_OK) return false;
		z.next_in = (unsigned char *)data;
		z.avail_in = (uint)realsize;
		auto flush = Z_NO_FLUSH;
		do
		{
			z.next_out = (unsigned char *)&buf;
			z.avail_out = BUFSIZE;
			if(z.avail_in == 0) flush = Z_FINISH;
			if(deflate(&z, flush) == Z_STREAM_ERROR) return false;

			f->write(buf, (streamsize)z.total_out);
			size += z.total_out;
		} while(flush != Z_FINISH);
		deflateEnd(&z);
		// Rewind to the header and write the compressed size.
		f->seekp((off_t)pos - 8, ios::beg);
		f->write((char *)&size, 8);
		f->write((char *)&realsize, 8);
		f->seekp((off_t)pos, ios::beg);
		f->seekp((off_t)size, ios::cur);
		return true;
	}
} // End namespace IFFSpace
