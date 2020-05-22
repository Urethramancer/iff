//
//  iffcomp.cpp
//	IFF compressor.
//
//  Copyright (c) 2012-2020 Ronny Bangsund. All rights reserved.
//

#include "iff.h"
#include "getopt.h"

#define PROGRAM "iffcomp"
#define VERSION "0.1.0"

using namespace std;
using namespace IFFSpace;

#define IFF_ARCHIVE MAKE_ID('A','R','C','H','I','V','E',' ')

void usage();

void usage()
{
	cout << PROGRAM << " " << VERSION << endl;
	cout << "Usage:\n";
	cout << " -h, --help				Show this help/usage text.\n";
	cout << " -a, --archive				IFF archive to create.\n";
	cout << " -f, --file=FILE			File to compress and add.\n";
}


typedef std::vector<std::string> Files;

int main(int argc, char * const *argv)
{
	static struct option longopts[] = {
		{"help", no_argument, nullptr, 'h'},
		{"file", required_argument, nullptr, 'f'},
		{nullptr, 0, nullptr, 0}
	};

	string archive = "";
	Files files;
	int ch;
	while((ch = getopt_long(argc, argv, "ha:f:", longopts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'h':
				usage();
				return 0;
				break;
			case 'a':
				archive = optarg;
				break;
			case 'f':
				files.push_back(optarg);
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
	cout << "Adding " << files.size() << " files to archive " << archive << endl;
	auto iff = new IFF(archive.c_str(), true);
	if(!iff)
	{
		cout << "Error opening '" << archive << "<\n";
		return 2;
	}

	if(archive.size() == 0)
	{
		cout << "No archive specified.\n";
		return 1;
	}

	if(files.size() == 0)
	{
		cout << "No files to add.\n";
		return 1;
	}

	iff->RegisterContainer(IFF_ARCHIVE);
	if(!iff->OK())
	{
		cout << "Failed to write file!\n";
		return 2;
	}

	string s = "This is some testdata with a suitably long string to test the compression methods in the IFF classes.";
	iff->AddChunk(IFF_NAME, (char *)files[0].c_str(), files[0].size());
	iff->AddChunk(IFF_COMP_UTF8, (char *)s.c_str(), s.size());
	iff->Save();

	// Verify
	iff->Reopen();
	if(!iff->OK())
	{
		cout << "Couldn't reopen!\n";
		return 2;
	}

	// Actual counts
	auto aSize = iff->GetFileSize();
	auto aChunks = iff->NumChunks();
	cout << "Opened file with " << aChunks << " chunks, totalling " << aSize <<" bytes.\n";

	delete iff;
	return 0;
}
