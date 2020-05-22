//
//  chunks.cpp
//  iff
//
//  Created by Ronny Bangsund on 29/10/2012.
//  Copyright (c) 2012 Neural Short-Circuit. All rights reserved.
//

#include <stdio.h>
#include "iff.h"


namespace IFFSpace
{
#pragma mark Chunk constructor
	// Initialise chunk with an identifier
	Chunk::Chunk(uint64_t identifier, ContainerMap *cm)
	{
		id = identifier;
		size = 0;
		data = nullptr;
		containers = cm;
	}


#pragma mark Chunk destructor
	Chunk::~Chunk()
	{
		//if(data) delete data;
	}


#pragma mark Reading Chunk data
	// Read chunk identifier and size.
	// Returns true if successful.
	bool Chunk::ReadHeader(fstream *f)
	{
		f->read((char *)&id, 8);
		if(f->good()) f->read((char *)&size, sizeof(size));
		// Setting the pos variable means data can be loaded out
		// of order by calling programs, rather than having to
		// parse each chunk again.
		pos = (uint64_t)f->tellg();
		if(containers->find(id) != containers->end())
		{
			while(pos < size)
			{
				auto c = new Chunk(IFF_UTF8, containers);
				if(c)
				{
					chunks.push_back(c);
					c->ReadHeader(f);
					f->seekg((off_t)c->GetSize(), ios::cur);
					pos = (uint64_t)f->tellg();
				} else {
					return false;
				}
			}
		}
		return f->good();
	}


	// Read the data into memory
	// Returns true if successful
	// Returns false on memory allocation failure etc.
	bool Chunk::ReadData(fstream *f)
	{
		// There's nothing to load. User is confused.
		if((size == 0) && (chunks.size() == 0)) return false;

		// Already loaded, everything's fine
		if(data) return true;

		if(containers->find(id) != containers->end())
		{
			for(auto c : chunks) c->ReadData(f);
		} else {
			data = new char[size];
			if(!data) return false;

			f->seekg((off_t)pos, ios::beg);
			f->read((char *)data, (streamsize)size);
		}
		return f->good();
	}


	uint64_t Chunk::GetID()
	{
		return id;
	}


	// Return size of contents.
	uint64_t Chunk::GetSize()
	{
		if((size == 0) && (chunks.size()))
		{
			// There are sub-chunks, so recalculate.
			for(auto c : chunks) size += c->GetFullSize();
		}
		return size;
	}


	// Return size with chunk header.
	uint64_t Chunk::GetFullSize()
	{
		return GetSize()+16;
	}


#pragma mark Writing Chunk data
	// Write the identifier and size
	// Returns false on failure
	bool Chunk::WriteHeader(std::fstream *f)
	{
		f->write((char *)&id, 8);
		if(f->good())
		{
			f->write((char *)&size, sizeof(size));
			pos = (uint64_t)f->tellp();
		}
		return f->good();
	}


	bool Chunk::WriteData(fstream *f)
	{
		if(containers->find(id) != containers->end())
		{
			for(auto c : chunks)
			{
				c->WriteHeader(f);
				c->WriteData(f);
			}
		} else {
			switch(id)
			{
				case IFF_COMP_UTF8:
				case IFF_COMP_UTF16:
				case IFF_COMP_UTF32:
					// Compress with zlib
					WriteDataZlib(f);
					break;
					
				default:
					f->write((char *)data, (streamsize)size);
					break;
			}
		}
		return f->good();
	}


	// Free the buffers
	void Chunk::Clear()
	{
		if(data)
		{
			delete data;
			data = nullptr;
		}
	}


#pragma mark Chunk memory management
	// Set chunk's data to data and size in arguments.
	// The chunk is now responsible for deallocating the memory when appropriate.
	void Chunk::SetData(char *d, uint64_t s)
	{
		data = new char[s];
		if(data)
		{
			memcpy(data, d, s);
			size = s;
		}
	}


	// Append data to the chunk and return the new size of the chunk.
	// Reallocates data if needed, and returns 0 if allocation fails.
	// Commonly used for compression. Caller is responsible for
	// freeing the source buffer afterwards.
	uint64_t Chunk::AddData(char *d, uint64_t s)
	{
		if(size == 0)
		{
			data = new char[s];
			if(!data) return 0;

			size = s;
			memcpy(data, d, s);
		} else {
			auto ndata = new char[size+s];
			if(!ndata) return 0;

			memcpy(ndata, data, size);
			memcpy((char *)ndata+size, d, s);
			delete data;
			data = ndata;
			size += s;
		}
		return size;
	}


	// Add an empty sub-chunk
	Chunk *Chunk::AddChunk(uint64_t identifier)
	{
		auto c = AddChunk(identifier, nullptr, 0);
		return c;
	}


	// Copy data as a sub-chunk.
	// Caller is responsible for freeing the source buffer.
	Chunk *Chunk::AddChunk(uint64_t identifier, char *d, uint64_t s)
	{
		// The old size is no longer valid. This will make
		// it be recalculated next time it's needed.
		size = 0;
		// Destroy data
		if(data) delete data;
		auto c = new Chunk(identifier, containers);
		if(c)
		{
			c->SetData(d, s);
			chunks.push_back(c);
		}
		return c;
	}


	size_t Chunk::NumChunks()
	{
		return chunks.size();
	}
	
	
	Chunk *Chunk::GetChunk(size_t index)
	{
		return chunks.at(index);
	}

} // End namespace IFF
