#cython: language_level=3
#distutils: language = c++
from enum import IntEnum
from .cvttcompilepy cimport *
from pathlib import Path

class CompileError(Exception):
    pass

class StripLevel(IntEnum):
     STRIP_NOTHING = stripNothing
     STRIP_SOURCE = stripSource
     STRIP_HINTS = stripHints
     STRIP_BINARY = stripBinary
     STRIP_EVERYTHING = stripEverything

cdef class Compiler:
     cdef Application* app_ # Hold C++ instance

     def __cinit__(self):
        self.app_ = new Application()

     def __cinit__(self, path: Path):
         self.app_ = new Application()
         self.app_.Create()    
         cdef string src = bytes(path)
         cdef wchar_t werr[1024]
         cdef char err[1024]
         result = self.app_.OpenFont(src, werr)
         if result != True:             
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))
         result = self.app_.GotoFont(werr)
         if result != True:
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))     

     def __dealloc__(self):
         del self.app_

     def compile_all(self) -> None:
         cdef wchar_t werr[1024]
         cdef char err[1024]
         result = self.app_.CompileAll(True, werr)
         if(result != True):
             raise CompileError(self.app_.wCharToChar(err, werr))

     def compile_glyph_range(self, start: int = 0, end: int = 0) -> None:
         cdef wchar_t werr[1024]
         cdef char err[1024]
         result = self.app_.CompileGlyphRange(start, end, True, werr)
         if(result != True):
             raise CompileError(self.app_.wCharToChar(err, werr))

     def save_font(self, path: Path, level: StripLevel) -> None:
         cdef string dest = bytes(path)
         cdef wchar_t werr[1024]
         cdef char err[1024]
         result = self.app_.SaveFont(dest, level, werr)
         if(result != True):
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))

     def save_font(self, level: StripLevel) -> None:
         cdef wchar_t werr[1024]
         cdef char err[1024]
         result = self.app_.SaveFont(level, werr)
         if(result != True):
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))


         

         

     


         

