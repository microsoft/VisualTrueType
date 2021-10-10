// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef TMTParser_dot_h
#define TMTParser_dot_h

class TMTParser {
public:
	virtual void Parse(bool *changedSrc, int32_t *errPos, int32_t *errLen, wchar_t error[]);
#if _DEBUG
	virtual void RemoveAltCodePath(bool *changedSrc, int32_t *errPos, int32_t *errLen, wchar_t error[]);
#endif
	virtual void InitTMTParser(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, bool legacyCompile, short generators, TTGenerator *gen[]);
	virtual void TermTMTParser(void);
	TMTParser(void);
	virtual ~TMTParser(void);
};

TMTParser *NewTMTSourceParser(void);
bool TMTCompile(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, int32_t glyphIndex, TextBuffer *glyfText, bool legacyCompile, int32_t *errPos, int32_t *errLen, wchar_t errMsg[]); // returns true if compilation completed successfully

#if _DEBUG
bool TMTRemoveAltCodePath(TextBuffer *talkText, TrueTypeFont *font, TrueTypeGlyph *glyph, int32_t *errPos, int32_t *errLen, wchar_t errMsg[]);
#endif

#endif // TMTParser_dot_h