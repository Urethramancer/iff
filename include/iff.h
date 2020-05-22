//
//  iff.h
//  iff
//
//  Copyright (c) 2012-2020 Ronny Bangsund. All rights reserved.
//

#ifndef iff_iff_h
#define iff_iff_h

#include "iostream"
#include "string"
#include "fstream"
#include "map"
#include "vector"


namespace IFFSpace
{
	using namespace std;
#pragma mark Core chunk definitions
	// The ID maker - turns 8 characters into a little-endian uint64
	// which will save nicely as a readable string.
#define MAKE_ID(a,b,c,d,e,f,g,h) \
((uint64_t)(h) << 56 | (uint64_t)(g) << 48 | (uint64_t)(f) << 40 | (uint64_t)(e) << 32 \
| (uint64_t)(d) << 24 | (uint64_t)(c) << 16 | (uint64_t)(b) << 8 | (uint64_t)(a))
	
#define IFF_FILEID MAKE_ID('I','F','F','6','4','B','I','T')		// An interchange file, containing any number of chunks (except zero)
	
#define IFF_ASCII MAKE_ID('A','S','C','I','I',' ',' ',' ')		// 7-bit ASCII text. Not a recommended format.
#define IFF_UTF8 MAKE_ID('U','T','F','8',' ',' ',' ',' ')		// UTF-8 text.
#define IFF_COMP_UTF8 MAKE_ID('C','O','M','P','U','T','F','8')	// Compressed UTF-8 text (first uint64 is the uncompressed size).
#define IFF_UTF16 MAKE_ID('U','T','F','1','6',' ',' ',' ')
#define IFF_COMP_UTF16	MAKE_ID('C','M','P','U','T','F','1','6')
#define IFF_UTF32 MAKE_ID('U','T','F','3','2',' ',' ',' ')
#define IFF_COMP_UTF32 MAKE_ID('C','M','P','U','T','F','3','2')

#define IFF_FOLDER MAKE_ID('F','O','L','D','E','R',' ',' ')	// Collection of chunk data and properties for this data. Nest these for hierarchical data, like directory structures.
#define IFF_VERSION MAKE_ID('V','E','R','S','I','O','N',' ')	// Version of a file or the current chunk.
#define IFF_NAME MAKE_ID('N','A','M','E',' ',' ',' ',' ')		// Filename, directory name etc. for the current chunk.
#define IFF_PROP MAKE_ID('P','R','O','P',' ',' ',' ',' ')		// Property name. Usually paired with a value inside a sub-chunk.
#define IFF_VALUE MAKE_ID('V','A','L','U','E',' ',' ',' ')		// Property value. Data is for the originating program to define.
#define IFF_RELFILE	MAKE_ID('R','E','L','F','I','L','E',' ')	// Filename relative to the IFF.
#define IFF_ANNOTATION MAKE_ID('A','N','N','O',' ',' ',' ',' ')	// Comment or annotation for the current chunk (UTF-8)
#define IFF_AUTHOR MAKE_ID('A','U','T','H','O','R',' ',' ')		// Author of file. Usually a person. Use one per person. (UTF-8)
#define IFF_ORIGIN MAKE_ID('O','R','I','G','I','N',' ',' ')		// A string with the program name and version used to create the file. (UTF-8)

	enum {
		IFF_COMPRESSION_NONE=0,
		IFF_COMPRESSION_ZLIB,		// Zlib (fast, sometimes best compression ratios for small chunks)
		IFF_COMPRESSION_BZIP		// Bzip2 (slow, but frequently better compression ratios)
	};

	#pragma mark Base classes
	//
	// Hook class
	// Hooks are functions
	class ChunkHook
	{
		// Identifier this is a hook for
		uint64_t		id;
	public:
		ChunkHook(uint64_t identifier) { id = identifier; }
		virtual ~ChunkHook();

		uint64_t GetID() { return id; }
		virtual bool ReadData(fstream *f, char **data, uint64_t *size);
		virtual bool WriteData(fstream *f, char *data, uint64_t size);
	};

	typedef map<uint64_t, ChunkHook *> HookMap;
	typedef map<uint64_t, bool> ContainerMap;


	//
	// Chunk class
	// IFFs have one or more of these. The IFF class refuses to write
	// empty structures.
	// The IFF class calls ReadHeader(), then either calls a hook
	// for the chunk type if supported, or lets the default chunk
	// reader handle loading of chunk data.
	//
	// ReadData() is fine for data which needs little to no
	// interpretation before use. Implement a hook reader to
	// handle compressed files, number data which needs conversion
	// before going into array buffers, or script code.
	//
	class Chunk;
	typedef vector<Chunk *> ChunkList;
	class Chunk
	{
		// Chunk identifier (8 bytes)
		uint64_t		id;
		uint64_t		size;			// Size of data. If this is an ARCHIVE, it's the size of all subchunks.
		// Position in file when reading (found while scanning)
		uint64_t		pos;
		char			*data;			// The chunk contents, if loaded into memory
		ChunkList		chunks;			// Sub-chunks (data is always NULL when these are used)
		ContainerMap	*containers;
	public:
		Chunk(uint64_t identifier, ContainerMap *cm);
		Chunk(ContainerMap *cm) : Chunk(IFF_NAME, cm) {};
		~Chunk();

		uint64_t GetID();
		uint64_t GetSize();
		uint64_t GetFullSize();

		void SetData(char *d, uint64_t s);
		uint64_t AddData(char *d, uint64_t s);
		Chunk *AddChunk(uint64_t identifier);
		Chunk *AddChunk(uint64_t identifier, char *d, uint64_t s);
		size_t NumChunks();
		Chunk *GetChunk(size_t index);
		bool WriteHeader(fstream *f);
		bool WriteData(fstream *f);
		bool WriteDataZlib(fstream *f);
		bool ReadHeader(fstream *f);
		bool ReadData(fstream *f);
		void Clear();
	};


	//
	// Interchange file class
	// This holds the name of an IFF, its filehandle,
	// pointers to chunk headers with potentially more data,
	// and custom hooks which handle user-defined chunk types.
	//
	class IFF
	{
		string			filename;
		fstream			f;
		uint64_t		size;		// Size of rest of file contents
		HookMap			hooks;		// Custom handlers of chunks
		ChunkList		chunks;		// All the actual contents
		ContainerMap	containers;	// Chunks with sub-chunks
		
	public:
		IFF(string name, bool write=false);
		~IFF();
		bool OK();
		void Erase();
		bool Reopen(bool write=false);
		uint64_t GetSize();
		void RegisterContainer(uint64_t identifier);
		void UnregisterContainer(uint64_t identifier);
		void RegisterHook(ChunkHook *hook);
		void UnregisterHook(ChunkHook *hook);
		
		void ScanFile();
		bool LoadAllChunks();

		Chunk *AddChunk(uint64_t id);
		Chunk *AddChunk(uint64_t id, char *d, uint64_t s);
		size_t NumChunks();
		Chunk *GetChunk(size_t index);
		size_t GetFileSize();
		bool Save();
	};
}	// End of IFFSpace
#endif
