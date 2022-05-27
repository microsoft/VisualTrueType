#cython: language_level=3
#distutils: language = c++
from libc.stdint cimport uint8_t, uint16_t, uint32_t, int32_t
from libcpp cimport bool
from libc.stddef cimport wchar_t
from libcpp.string cimport string

cdef extern from "pch.h":
 
    ctypedef enum StripCommand:
        stripNothing
        stripSource
        stripHints
        stripBinary
        stripEverything

cdef extern from "application.h":

    cdef cppclass Application:
        Application() except +
        bool Create()
        bool OpenFont(string fileName, wchar_t* errMsg, size_t errMsgLen)
        bool SaveFont(StripCommand strip, wchar_t* errMsg, size_t errMsgLen)
        bool SaveFont(string fileName, StripCommand strip, wchar_t* errMsg, size_t errMsgLen)

        bool GotoFont(wchar_t* errMsg, size_t errMsgLen)

        bool CompileGlyphRange(uint16_t g1, uint16_t g2, bool quiet, bool legacy, bool variationCompositeGuard, wchar_t* errMsg, size_t errMsgLen)
        bool CompileAll(bool quiet, bool legacy, bool variationCompositeGuard, wchar_t* errMsg, size_t errMsgLen)

        char* wCharToChar(char* out1, const wchar_t* in1)
