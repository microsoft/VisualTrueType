// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <iostream>

#include "pch.h"
#include "application.h"

#include "getopt.h"

int ShowUsage(wchar_t* strErr)
{
	if (strErr)
	{
		wprintf(L"ERROR: " WIDE_STR_FORMAT L"\n\n", strErr);
	}
	wprintf(L"USAGE: vttcompile [options] <in.ttf> [out.ttf] \n");
	wprintf(L"\t-a compile everything \n");
	wprintf(L"\t-gXXXX compile everything for glyph id (base 10) \n");
	wprintf(L"\t-rXXXX compile everything for glyph range starting with glyph specified  \n\t       with -g up to and including glyph specified with -r \n ");
	wprintf(L"\t-s strip source \n");
	wprintf(L"\t-b strip source and hints \n");
	wprintf(L"\t-c strip source, hints and cache tables \n");
	wprintf(L"\t-q quiet mode \n");
	wprintf(L"\t-? this help message \n\n");
	wprintf(L"\n");
	return -1;
}

int main(int argc, char* argv[])
{
	char c;
	bool bStripSource = false;
	bool bStripHints = false;
	bool bStripCache = false;
	bool bCompileAll = false;
	bool bQuiet = false;
	std::string sg1, sg2;
	uint32_t g1 = 0, g2 = 0;
	bool haveGlyph = false;
	bool haveRange = false;

	int argOffset = 0;

	wchar_t errMsg[1024];
	errMsg[0] = 0;

	std::string inFile;
	std::string outFile;

	std::wstring szOutText;

	wprintf(L"Microsoft Visual TrueType Command Line Interface Version " VTTVersionString L" \n");
	wprintf(L"Copyright (C) Microsoft Corporation. Licensed under the MIT License.\n");

	if (argc == 1) return ShowUsage(NULL);

	CommandLineOptions cmd(argc, argv, "?HhAaBbSsCcqQg:G:r:R");

	while ((c = cmd.GetOption()) != END)
	{
		switch (c)
		{
		case 'A':
		case 'a':
			bCompileAll = true;
			break;

		case 'B':
		case 'b':
			bStripHints = true;
			break;

		case 'S':
		case 's':
			bStripSource = true;
			break;

		case 'c':
		case 'C':
			bStripCache = true;
			break;

		case 'q':
		case 'Q':
			bQuiet = true;
			break;

		case 'g':
		case 'G':
			sg1 = cmd.GetOptionArgument();
			break;

		case 'r':
		case 'R':
			sg2 = cmd.GetOptionArgument();
			break;

		case 'H':
		case 'h':
		case '?':
		default:
			ShowUsage(NULL);
			exit(EXIT_SUCCESS);
		}
	}

	if (argv[cmd.GetArgumentIndex() + argOffset] == NULL)
	{
		ShowUsage(NULL);
		exit(EXIT_SUCCESS);
	}

	if (!sg1.empty())
	{
		try
		{
			g1 = g2 = static_cast<uint32_t>(std::stoul(sg1));
		}
		catch (...)
		{
			fwprintf(stderr, L"Can not parse glyph number!\n");
			exit(EXIT_FAILURE);
		}
		haveGlyph = true;
	}

	if (!sg2.empty())
	{
		try
		{
			g2 = static_cast<uint32_t>(std::stoul(sg2));
		}
		catch (...)
		{
			fwprintf(stderr, L"Can not parse glyph number!\n");
			exit(EXIT_FAILURE);
		}
		haveRange = true;
	}

	inFile = argv[cmd.GetArgumentIndex() + argOffset];
	argOffset++;

	if (argv[cmd.GetArgumentIndex() + argOffset] == NULL)
		outFile = inFile;
	else
		outFile = argv[cmd.GetArgumentIndex() + argOffset];

	auto application = std::make_unique<Application>();
	if (!application->Create()) {
		fwprintf(stderr, L"Can not load application!\n");
		exit(EXIT_FAILURE);
	}

	if (!application->OpenFont(inFile, errMsg))
	{
		fwprintf(stderr, errMsg);
		fwprintf(stderr, L"\n");
		fwprintf(stderr, L"Can not open font file " NARROW_STR_FORMAT L"!\n", inFile.c_str());
		exit(EXIT_FAILURE);
	}

	if (!application->GotoFont(errMsg))
	{
		fwprintf(stderr, errMsg);
		fwprintf(stderr, L"Can not initialize font file! \n");
		fwprintf(stderr, L"\n");
		exit(EXIT_FAILURE);
	}

	if (bCompileAll)
	{
		if (!application->CompileAll(bQuiet, errMsg))
		{
			fwprintf(stderr, errMsg);
			fwprintf(stderr, L"\n");
			fwprintf(stderr, L"Can not complete compile operation! \n");
			fwprintf(stderr, L"\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (haveGlyph)
	{
		if (!application->CompileGlyphRange((unsigned short)g1, (unsigned short)g2, bQuiet, errMsg))
		{
			fwprintf(stderr, errMsg);
			fwprintf(stderr, L"\n");
			fwprintf(stderr, L"Can not complete compile operation! \n");
			fwprintf(stderr, L"\n");
			exit(EXIT_FAILURE);
		}
	}

	StripCommand strip = stripNothing;
	if (bStripCache) strip = stripBinary;
	else if (bStripHints) strip = stripHints;
	else if (bStripSource) strip = stripSource;

	if (!application->SaveFont(outFile, strip, errMsg))
	{
		fwprintf(stderr, errMsg);
		fwprintf(stderr, L"\n");
		fwprintf(stderr, L"Can not save file! \n");
		fwprintf(stderr, L"\n");
		exit(EXIT_FAILURE);
	}

	wprintf(L"Success \n");

	exit(EXIT_SUCCESS);
	return 0;
}


