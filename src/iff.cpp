//
//  iff.cpp
//  iff
//
//  Copyright (c) 2012-2020 Ronny Bangsund. All rights reserved.
//

#include <iostream>
#include "iff.h"

namespace IFFSpace
{
#pragma mark IFF constructor
	// Create IFF for reading or writing
	IFF::IFF(string name, bool write)
	{
		size = 0;
		filename.assign(name);
		Reopen(write);

		// Set up known container chunk identifiers
		RegisterContainer(IFF_FOLDER);
	}


	// Did the IFF open successfully?
	bool IFF::OK()
	{
		return f.is_open();
	}


	// Get the size of the entire file
	uint64_t IFF::GetSize()
	{
		return size;
	}


#pragma mark IFF destructor
	IFF::~IFF()
	{
		f.close();
		Erase();
	}


	// Erase all data
	void IFF::Erase()
	{
		while(hooks.size())
		{
			auto h = hooks.at(0);
			hooks.erase(hooks.begin());
			delete h;
		}
		while(chunks.size())
		{
			auto c = chunks.at(0);
			chunks.erase(chunks.begin());
			delete c;
		}
	}


	// Flush, close and reopen in read or write mode.
	bool IFF::Reopen(bool write)
	{
		if(f.is_open())
		{
			f.flush();
			f.close();
		}
		Erase();
		auto flags = ios::binary;
		if(write)
		{
			// Create a new file for writing, truncate any existing file
			flags |= ios::out | ios::trunc | ios::ate;
			f.open(filename, flags);
		} else {
			// Try to open an existing file and get its total size
			// Size will be 0 if failed or empty
			flags |= ios::in;
			f.open(filename, flags);
			f.seekg(0, ios::end);
			size = (uint64_t)f.tellg();
			if(size > 16)
			{
				f.seekg(0, ios::beg);
				uint64_t h;
				f.read((char *)&h, 8);
				// Check that it's a valid IFF64
				if(h != IFF_FILEID)
				{
					f.close();
					return false;
				}
				f.read((char *)&size, sizeof(size));
				// Get an overview of chunks and their sizes
				ScanFile();
			}
		}
		return OK();
	}


	// Add a container ID to the list.
	// When encountered, this chunk type will be scanned
	// for sub-chunks instead of loading data or handling hooks.
	void IFF::RegisterContainer(uint64_t identifier)
	{
		containers[identifier] = true;
	}


	// Remove a container from the list.
	// Rarely used, but it's there for anyone who needs it.
	void IFF::UnregisterContainer(uint64_t identifier)
	{
		containers.erase(identifier);
	}


	// Register a read/write hook for a custom chunk type
	void IFF::RegisterHook(ChunkHook *hook)
	{
		hooks[hook->GetID()] = hook;
	}


	void IFF::UnregisterHook(ChunkHook *hook)
	{
		hooks.erase(hook->GetID());
	}


#pragma mark File operations
	// Look through the file, allocate chunk stubs and return number of chunks found
	void IFF::ScanFile()
	{
		if(size == 0) return;

		f.seekg(16, ios::beg);
		uint64_t pos=0;
		// pos represents size of data without IFF header
		while(pos < size)
		{
			auto c = new Chunk(IFF_UTF8, &containers);
			c->ReadHeader(&f);
			chunks.push_back(c);
			pos += 16;
			f.seekg((off_t)c->GetSize(), ios::cur);
			pos += c->GetSize();
		}
	}


	// Load data from all chunks into memory.
	//
	// Returns true if all chunks loaded into memory.
	bool IFF::LoadAllChunks()
	{
		for(auto c : chunks)
		{
			c->ReadData(&f);
		}
		return false;
	}


	// Create an empty chunk with the desired identifier
	Chunk *IFF::AddChunk(uint64_t id)
	{
		auto c = new Chunk(id, &containers);
		if(c) chunks.push_back(c);
		return c;
	}


	// Create a new chunk with data.
	// The caller is responsible for the deallocation of the source data.
	Chunk *IFF::AddChunk(uint64_t id, char *d, uint64_t s)
	{
		auto c = AddChunk(id);
		if(c) c->AddData(d, s);
		return c;
	}


	size_t IFF::NumChunks()
	{
		return chunks.size();
	}


	Chunk *IFF::GetChunk(size_t index)
	{
		return chunks.at(index);
	}


	// Sum up the size of all chunks with data,
	// adding header sizes to get the final filesize total.
	// The size variable is set to the size of all the chunks with their headers.
	// The returned size adds the 16-byte header to the calculation.
	size_t IFF::GetFileSize()
	{
		size = 0;
		for(auto c : chunks)
		{
			if(c->GetSize()) size += c->GetFullSize();
		}
		return size+16;
	}


	// Save the IFF file.
	// Saves header and all chunks with data.
	// Empty chunks are not saved.
	// The size variable is recalculated along the way.
	bool IFF::Save()
	{
		auto h = IFF_FILEID;
		f.write((char *)&h, 8);
		f.write((char *)&h, 8);
		size = 0;
		for(auto c : chunks)
		{
			if(c->GetSize())
			{
				c->WriteHeader(&f);
				c->WriteData(&f);
				size += c->GetFullSize();
			}
		}
		f.seekg(8, ios::beg);
		f.write((char *)&size, sizeof(size));
		f.flush();
		return true;
	}
} // End namespace IFF
