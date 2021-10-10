
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <iostream>

#include "pch.h"
#include "vttcompile.h"

#include "getopt.h"

Application::Application(void)
{
	this->mainFont[0] = L'\0';
	this->textFont[0] = L'\0';
	this->fileName[0] = L'\0';
}

bool Application::Create()
{
	//wchar_t errMsg[maxLineSize];

	this->font = std::make_unique<TrueTypeFont>();
	if (this->font == nullptr)
		return false;

	this->glyph = std::make_unique<TrueTypeGlyph>();
	if (this->glyph == nullptr)
		return false;

	if (!this->font->Create())
		return false;

	this->glyf = std::make_unique<TextBuffer>();
	if (this->glyf == nullptr)
		return false;

	this->prep = std::make_unique<TextBuffer>();
	if (this->prep == nullptr)
		return false;

	this->talk = std::make_unique<TextBuffer>();
	if (this->talk == nullptr)
		return false;

	this->fpgm = std::make_unique<TextBuffer>();
	if (this->fpgm == nullptr)
		return false;

	this->cpgm = std::make_unique<TextBuffer>();
	if (this->cpgm == nullptr)
		return false;

	return true;
}

Application::~Application(void)
{
	
}

bool Application::OpenFont(std::string fileName, wchar_t errMsg[]) {
	auto file = std::make_unique<File>();

	this->charCode = this->glyphIndex = ILLEGAL_GLYPH_INDEX;

	this->fileName = fileName;
	file->OpenOld(this->fileName.c_str());
	if (file->Error())
		return false;

	if (!this->font->Read(file.get(), this->glyph.get(), &this->platformID, &this->encodingID, errMsg))
		return false;

	file->Close(false);

	return true;
}

bool Application::SaveFont(StripCommand strip, wchar_t errMsg[])
{
	return this->SaveFont(this->fileName, strip, errMsg);
}

bool Application::SaveFont(std::string fileN, StripCommand strip, wchar_t errMsg[])
{
	auto file = std::make_unique<File>();
	errMsg[0] = 0;

	if (!this->BuildFont(strip, errMsg))
		return false;

	file->OpenNew(fileN.c_str());
	if (file->Error())
		return false;

	if (!this->font->Write(file.get(), errMsg))
		file->Close(true);

	file->Close(true);

	return true;
}

bool Application::GotoFont(wchar_t errMsg[]) {
	int32_t errPos, errLen;
	bool legacy = false; 

	if (!this->font->GetCvt(this->cpgm.get(), errMsg)) return false;
	this->font->TheCvt()->Compile(this->cpgm.get(), NULL, legacy, &errPos, &errLen, errMsg);
	if (!this->font->GetPrep(this->prep.get(), errMsg)) return false;
	if (!this->font->GetFpgm(this->fpgm.get(), errMsg)) return false;

	return true; // by now, ignoring the fact that a stripped font will not have any of the sources above
} // Application::GotoFont

bool Application::GotoGlyph(int32_t code, bool isGlyphIndex) {
	int32_t numGlyphs = this->font->NumberOfGlyphs(), glyphIndex, charCode;
	wchar_t errMsg[maxLineSize];

	if (isGlyphIndex) {
		glyphIndex = (code + numGlyphs) % numGlyphs; // cyclical clipping
		charCode = this->font->CharCodeOf(glyphIndex);
	}
	else {
		charCode = code;
		glyphIndex = this->font->GlyphIndexOf(code);
		if (glyphIndex == ILLEGAL_GLYPH_INDEX) glyphIndex = 0;
	}
	if (this->glyphIndex != glyphIndex || this->charCode != charCode) {
		
		this->glyphIndex = glyphIndex;
		this->charCode = charCode;
		this->font->GetGlyf(glyphIndex, this->glyf.get(), errMsg);
		this->font->GetTalk(glyphIndex, this->talk.get(), errMsg);
	}
	return true; // by now, ignoring the fact that a stripped font will not have any of the sources above
} // Application::GotoGlyph

bool Application::CompileTalk(int32_t* errPos, int32_t* errLen, wchar_t errMsg[])
{
	bool legacy = false;

	bool done = TMTCompile(this->talk.get(), this->font.get(), this->glyph.get(), this->glyphIndex, this->glyf.get(), legacy, errPos, errLen, errMsg);

	return done;
}

bool Application::CompileCommon(int32_t* errPos, int32_t* errLen, wchar_t errMsg[])
{
	bool done = false;
	bool legacy = false; 
	bool variationCompositeGuard = true; 

	int32_t glyphIndex, binSize;
	unsigned char* binData;
	wchar_t tempErrMsg[maxLineSize], compErrMsg[maxLineSize];
	TextBuffer* errBuf = NULL;
	
	binData = (unsigned char*)NewP(MAXBINSIZE);
	done = binData != NULL;

	glyphIndex = 0;
	this->glyphIndex = glyphIndex;
	this->charCode = this->font->CharCodeOf(glyphIndex);
	// Init the glyph structure to valid data needed by assembler when assembling fpgm and prep. 
	if (done) done = this->font->GetGlyph(glyphIndex, this->glyph.get(), errMsg);
	if (done) done = this->font->GetTalk(glyphIndex, this->talk.get(), errMsg);
	if (done) done = this->font->GetGlyf(glyphIndex, this->glyf.get(), errMsg);

	if (done) {
		errBuf = new TextBuffer();
		done = errBuf != NULL;
	}

	if (done) {
		if (this->font->TheCvt()->Compile(this->cpgm.get(), this->prep.get(), legacy, errPos, errLen, compErrMsg)) {
			// Compile updates cvt binary autonomously
			this->font->UpdateAdvanceWidthFlag(this->font->TheCvt()->LinearAdvanceWidths());
		}
		else {
			swprintf(tempErrMsg, L"Ctrl Pgm, line %li: %s", this->cpgm->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}

	if (this->font->IsVariationFont()) {
		this->font->ReverseInterpolateCvarTuples();

		// Check tuples that don't result in any deltas and and are not represented in "edited" to reset relavence for cvar. 
		auto tuples = this->font->GetInstanceManager()->GetCvarTuples();
		for (auto& tuple : *tuples)
		{
			// Even though a tuple may have no deltas make sure we don't remove any instanced with user edited values
			// where the the edited value could have been optomized to another tuple. 
			bool edited = false;
			for (auto& editedValue : tuple->editedCvtValues)
			{
				edited |= editedValue.Edited();
			}

			// No data and not edits then not needed in cvar
			if (!tuple->HasData() && !edited) // && tuple->IsCvar()) // implied 
			{
				tuple->SetAsCvar(false);
			}
		}
	} 

	if (done) {
		if (TTAssemble(asmFPGM, this->fpgm.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg))
			done = this->font->UpdateBinData(asmFPGM, binSize, binData);
		else {
			done = this->font->UpdateBinData(asmFPGM, 0, NULL);
			swprintf(tempErrMsg, L"Font Pgm, line %li: %s", this->fpgm->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}

	if (done) {
		if (TTAssemble(asmPREP, this->prep.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg))
			done = this->font->UpdateBinData(asmPREP, binSize, binData);
		else {
			done = this->font->UpdateBinData(asmPREP, 0, NULL);
			swprintf(tempErrMsg, L"Pre Pgm, line %li: %s", this->prep->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}

	if (binData != NULL) DisposeP((void**)&binData);

	done = done && errBuf->Length() == 0;

	if (!done) {

		if (errBuf->Length() > 0) {
			std::wstring errStr;

			errBuf->GetText(errStr);

			fwprintf(stderr, errStr.c_str());
			fwprintf(stderr, L"\n");
		}
	}

	if (errBuf != NULL) delete errBuf;

	return done;
}

bool Application::CompileGlyphRange(unsigned short g1, unsigned short g2, int32_t* errPos, int32_t* errLen, bool quiet, wchar_t errMsg[])
{
	int32_t glyphIndex, binSize, fromGlyph, toGlyph, numGlyphs = this->font->NumberOfGlyphs();
	bool done = false;
	unsigned char* binData;
	wchar_t tempErrMsg[maxLineSize], compErrMsg[maxLineSize];
	TextBuffer* errBuf = NULL;
	bool legacyCompile = false;
	bool variationCompositeGuard = true; 

	binData = (unsigned char*)NewP(MAXBINSIZE);
	done = binData != NULL;

	glyphIndex = 0;
	this->glyphIndex = glyphIndex;
	this->charCode = this->font->CharCodeOf(glyphIndex);

	done = this->CompileCommon(errPos, errLen, errMsg);
	if (!done)
		return done;

	if (done) {
		errBuf = new TextBuffer();
		done = errBuf != NULL;
	}

	fromGlyph = g1;
	toGlyph = g2;
	for (glyphIndex = fromGlyph; glyphIndex <= toGlyph && glyphIndex < numGlyphs && done; glyphIndex++) {
		if (!quiet && ((glyphIndex + 1) % 10 == 0)) wprintf_s(L".");
		if (!quiet && ((glyphIndex + 1) % 200 == 0)) wprintf_s(L"\n");

		this->glyphIndex = glyphIndex;
		this->charCode = this->font->CharCodeOf(glyphIndex);
		done = this->font->GetGlyph(glyphIndex, this->glyph.get(), errMsg);
		if (done) done = this->font->GetTalk(glyphIndex, this->talk.get(), errMsg);
		if (done) done = this->font->GetGlyf(glyphIndex, this->glyf.get(), errMsg);

		if (done) {
			if (!TMTCompile(this->talk.get(), this->font.get(), this->glyph.get(), this->glyphIndex, this->glyf.get(), legacyCompile, errPos, errLen, compErrMsg)) {
				swprintf(tempErrMsg, L"VTT Talk, glyph %li (Unicode 0x%lx), line %li: %s", this->glyphIndex, this->charCode, this->talk->LineNumOf(*errPos), compErrMsg);
				errBuf->AppendLine(tempErrMsg);
				swprintf(tempErrMsg, L"/* Error in VTT Talk, line %li: %s */", this->talk->LineNumOf(*errPos), compErrMsg);
				this->glyf->SetText((int32_t)STRLENW(tempErrMsg), tempErrMsg); // prevent follow-up errors
			}
		}

		binSize = 0;
		if (done) {
			if (TTAssemble(asmGLYF, this->glyf.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg)) {
				done = this->font->UpdateBinData(asmGLYF, binSize, binData);
			}
			else
			{
				done = this->font->UpdateBinData(asmGLYF, 0, NULL);
				swprintf(tempErrMsg, L"Glyf Pgm, glyph %li (Unicode 0x%lx), line %li: %s", this->glyphIndex, this->charCode, this->glyf->LineNumOf(*errPos), compErrMsg);
				errBuf->AppendLine(tempErrMsg);
			}
		}

		if (done)
		{
			done = this->BuildFont(stripNothing, compErrMsg);
		}
	}
	if (!quiet && (glyphIndex % 100 != 0)) wprintf_s(L"\n");

	if (binData != NULL) DisposeP((void**)&binData);

	done = done && errBuf->Length() == 0;

	if (!done) {
		if (errBuf->Length() > 0) {
			std::wstring errStr;

			errBuf->GetText(errStr);

			fwprintf(stderr, errStr.c_str());
			fwprintf(stderr, L"\n");
		}
	}

	if (errBuf != NULL) delete errBuf;

	return done;
}

bool Application::CompileAll(int32_t* errPos, int32_t* errLen, bool quiet, wchar_t errMsg[])
{
	int32_t glyphIndex, binSize, fromGlyph, fromChar, numGlyphs = this->font->NumberOfGlyphs();
	bool done;
	unsigned char* binData;
	wchar_t tempErrMsg[maxLineSize], compErrMsg[maxLineSize];
	TextBuffer* errBuf = NULL;
	bool legacyCompile = false; 
	bool variationCompositeGuard = true; 

	binData = (unsigned char*)NewP(MAXBINSIZE);
	done = binData != NULL;

	glyphIndex = 0;
	this->glyphIndex = glyphIndex;
	this->charCode = this->font->CharCodeOf(glyphIndex);
	// Init the glyph structure to valid data needed by assembler when assembling fpgm and prep. 
	if (done) done = this->font->GetGlyph(glyphIndex, this->glyph.get(), errMsg);
	if (done) done = this->font->GetTalk(glyphIndex, this->talk.get(), errMsg);
	if (done) done = this->font->GetGlyf(glyphIndex, this->glyf.get(), errMsg);

	if (done) {
		done = this->font->InitIncrBuildSfnt(false, errMsg);
	}

	if (done) {
		errBuf = new TextBuffer();
		done = errBuf != NULL;
	}

	if (done) {
		if (this->font->TheCvt()->Compile(this->cpgm.get(), this->prep.get(), legacyCompile, errPos, errLen, compErrMsg)) {
			// Compile updates cvt binary autonomously
			this->font->UpdateAdvanceWidthFlag(this->font->TheCvt()->LinearAdvanceWidths());
		}
		else {
			swprintf(tempErrMsg, L"Ctrl Pgm, line %li: %s", this->cpgm->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}


	if (this->font->IsVariationFont()) {
		this->font->ReverseInterpolateCvarTuples();

		// Check tuples that don't result in any deltas and and are not represented in "edited" to reset relavence for cvar. 
		auto tuples = this->font->GetInstanceManager()->GetCvarTuples();
		for (auto& tuple : *tuples)
		{
			// Even though a tuple may have no deltas make sure we don't remove any instanced with user edited values
			// where the the edited value could have been optomized to another tuple. 
			bool edited = false;
			for (auto& editedValue : tuple->editedCvtValues)
			{
				edited |= editedValue.Edited();
			}

			// No data and not edits then not needed in cvar
			if (!tuple->HasData() && !edited) // && tuple->IsCvar()) // implied 
			{
				tuple->SetAsCvar(false);
			}
		}
	} 

	if (done) {
		if (TTAssemble(asmFPGM, this->fpgm.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg))
			done = this->font->UpdateBinData(asmFPGM, binSize, binData);
		else {
			done = this->font->UpdateBinData(asmFPGM, 0, NULL);
			swprintf(tempErrMsg, L"Font Pgm, line %li: %s", this->fpgm->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}

	if (done) {
		if (TTAssemble(asmPREP, this->prep.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg))
			done = this->font->UpdateBinData(asmPREP, binSize, binData);
		else {
			done = this->font->UpdateBinData(asmPREP, 0, NULL);
			swprintf(tempErrMsg, L"Pre Pgm, line %li: %s", this->prep->LineNumOf(*errPos), compErrMsg);
			errBuf->AppendLine(tempErrMsg);
		}
	}

	fromGlyph = this->glyphIndex; fromChar = this->charCode;
	for (glyphIndex = 0; glyphIndex < numGlyphs && done; glyphIndex++) {
		if (!quiet && ((glyphIndex + 1) % 10 == 0)) wprintf_s(L".");
		if (!quiet && ((glyphIndex + 1) % 200 == 0)) wprintf_s(L"\n");
		//this->MakeProgress(glyphIndex, glyphIndex);
		this->glyphIndex = glyphIndex;
		this->charCode = this->font->CharCodeOf(glyphIndex);
		done = this->font->GetGlyph(glyphIndex, this->glyph.get(), errMsg);
		if (done) done = this->font->GetTalk(glyphIndex, this->talk.get(), errMsg);
		if (done) done = this->font->GetGlyf(glyphIndex, this->glyf.get(), errMsg);

		if (done) {
			if (!TMTCompile(this->talk.get(), this->font.get(), this->glyph.get(), this->glyphIndex, this->glyf.get(), legacyCompile, errPos, errLen, compErrMsg)) {
				swprintf(tempErrMsg, L"VTT Talk, glyph %li (Unicode 0x%lx), line %li: %s", this->glyphIndex, this->charCode, this->talk->LineNumOf(*errPos), compErrMsg);
				errBuf->AppendLine(tempErrMsg);
				swprintf(tempErrMsg, L"/* Error in VTT Talk, line %li: %s */", this->talk->LineNumOf(*errPos), compErrMsg);
				this->glyf->SetText((int32_t)STRLENW(tempErrMsg), tempErrMsg); // prevent follow-up errors
			}
		}

		binSize = 0;
		if (done) {
			if (!TTAssemble(asmGLYF, this->glyf.get(), this->font.get(), this->glyph.get(), MAXBINSIZE, binData, &binSize, variationCompositeGuard, errPos, errLen, compErrMsg)) {
				swprintf(tempErrMsg, L"Glyf Pgm, glyph %li (Unicode 0x%lx), line %li: %s", this->glyphIndex, this->charCode, this->glyf->LineNumOf(*errPos), compErrMsg);
				errBuf->AppendLine(tempErrMsg);
			}
		}

		if (done) done = this->font->AddGlyphToNewSfnt(this->font->CharGroupOf(glyphIndex), glyphIndex, this->glyph.get(), binSize, binData, this->glyf.get(), this->talk.get(), errMsg);
	}
	if (!quiet && (glyphIndex % 100 != 0)) wprintf_s(L"\n");

	done = this->font->TermIncrBuildSfnt(!done, this->prep.get(), this->cpgm.get(), this->fpgm.get(), errMsg);

	if (binData != NULL) DisposeP((void**)&binData);

	done = done && errBuf->Length() == 0;

	if (!done) {
		// this error is supposed to report the reason that precluded further execution
		//if (STRLENW(errMsg) > 0)
		//	ErrorBox(errMsg);
		// these errors are supposed to report failed compilations (give the user a chance to fix the code)
		if (errBuf->Length() > 0) {
			std::wstring errStr;

			errBuf->GetText(errStr);

			fwprintf(stderr, errStr.c_str());
			fwprintf(stderr, L"\n");
		}
	}

	if (errBuf != NULL) delete errBuf;

	return done;
}

bool Application::BuildFont(StripCommand strip, wchar_t errMsg[]) {

	// If we did not compile and are just here to strip data then perform lazy initialization.
	if (this->glyphIndex == ILLEGAL_GLYPH_INDEX)
	{
		this->glyphIndex = 0;
		this->charCode = this->font->CharCodeOf(this->glyphIndex);
	}

	// Init the glyph structure to valid data needed by assembler when assembling fpgm and prep. 
	bool done = true;
	if (done) done = this->font->GetGlyph(this->glyphIndex, this->glyph.get(), errMsg);
	if (done) done = this->font->GetTalk(this->glyphIndex, this->talk.get(), errMsg);
	if (done) done = this->font->GetGlyf(this->glyphIndex, this->glyf.get(), errMsg);

	return this->font->BuildNewSfnt(strip, anyGroup, this->glyphIndex, this->glyph.get(), this->glyf.get(),
		this->prep.get(), this->cpgm.get(), this->talk.get(), this->fpgm.get(), errMsg);
}

int ShowUsage(wchar_t* strErr)
{
	if (strErr)
	{
		wprintf(L"ERROR: %s\n\n", strErr);
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
			g1 = g2 = std::stoul(sg1);
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
			g2 = std::stoul(sg2);
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
		fwprintf(stderr, L"Can not open font file %s!\n", inFile.c_str());
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
		int32_t errPos, errLen;

		if (!application->CompileAll(&errPos, &errLen, bQuiet, errMsg))
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
		int32_t errPos, errLen;

		if (!application->CompileGlyphRange((unsigned short)g1, (unsigned short)g2, &errPos, &errLen, bQuiet, errMsg))
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


