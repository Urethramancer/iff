//
//  main.cpp
//  iff
//
//  Copyright (c) 2012-2020 Ronny Bangsund. All rights reserved.
//

#include "iff.h"
#include "getopt.h"

#define PROGRAM "ifftest"
#define VERSION "0.1.0"

using namespace std;
using namespace IFFSpace;

#define IFF_ARCHIVE MAKE_ID('A','R','C','H','I','V','E',' ')

void usage();
int test();

void usage()
{
	cout << PROGRAM << " " << VERSION << endl;
	cout << "Usage:\n";
	cout << " -h, --help				Show this help/usage text.\n";
	cout << " -t, --test=FILE			Create a test file named FILE and verify it.\n";
}


int test()
{
	cout << "Writing test file '" << optarg << "'.\n";
	auto iff = new IFF(optarg, true);
	if(!iff) return 2;
	
	iff->RegisterContainer(IFF_ARCHIVE);
	if(!iff->OK()) cout << "Failed to write file!\n";
	string s = "This is some testdata.";
	iff->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());
	s = "More testdata.";
	iff->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());
	
	auto c = iff->AddChunk(IFF_ARCHIVE);
	s = "Yet more testdata.";
	c->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());
	s = "Even more testdata!";
	c->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());
	
	c = iff->AddChunk(IFF_FOLDER);
	s = "So much testdata!";
	c->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());
	s = "This is deeply nested.";
	c->AddChunk(IFF_UTF8, (char *)s.c_str(), s.size());

	// Expected counts
	auto eSize = iff->GetFileSize();
	auto eChunks = iff->NumChunks();
	cout << "File size should be " << eSize << " bytes in " << eChunks << " chunks.\n";
	iff->Save();
	iff->Reopen();
	if(!iff->OK()) return 2;

	// Actual counts
	auto aSize = iff->GetFileSize();
	auto aChunks = iff->NumChunks();
	cout << "Opened file with " << aChunks << " chunks, totalling " << aSize <<" bytes.\n";
	iff->LoadAllChunks();
	delete iff;

	if((eSize != aSize) || (eChunks != aChunks))
	{
		cout << "IFF inconsistency!\n";
		return 2;
	}
	cout << "IFF structure looks OK.\n";
	return 0;
}


int main(int argc, char * const *argv)
{
	static struct option longopts[] = {
		{"help", no_argument, nullptr, 'h'},
		{"test", required_argument, nullptr, 't'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while((ch = getopt_long(argc, argv, "ht:", longopts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'h':
				usage();
				return 0;
				break;
			case 't':
				return test();
				break;
			case 0:
				
				break;
				
			default:
				usage();
				return 0;
				break;
		}
	}
	argc -= optind;
	argv += optind;
	return 0;
}
