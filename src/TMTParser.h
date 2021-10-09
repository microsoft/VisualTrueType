// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TMTParser_dot_h
#define TMTParser_dot_h

class TMTParser {
public:
	virtual void Parse(bool *changedSrc, int *errPos, int *errLen, wchar_t error[]);
#if _DEBUG
	virtual void RemoveAltCodePath(bool *changedSrc, int *errPos, int *errLen, wchar_t error[]);
#endif
	virtual void InitTMTParser(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, bool legacyCompile, short generators, TTGenerator *gen[]);
	virtual void TermTMTParser(void);
	TMTParser(void);
	virtual ~TMTParser(void);
};

TMTParser *NewTMTSourceParser(void);
bool TMTCompile(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, int glyphIndex, TextBuffer *glyfText, bool legacyCompile, int *errPos, int *errLen, wchar_t errMsg[]); // returns true if compilation completed successfully

#if _DEBUG
bool TMTRemoveAltCodePath(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, int *errPos, int *errLen, wchar_t errMsg[]);
#endif

#endif // TMTParser_dot_h