#cython: language_level=3
#distutils: language = c++
from enum import IntEnum
from .cvttcompilepy cimport *
from pathlib import Path

DEF ERR_BUF_SIZE = 1024

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

     #def __cinit__(self):
     #   self.app_ = new Application()

     def __cinit__(self, path: Path):
         self.app_ = new Application()
         self.app_.Create()    
         cdef string src = bytes(path)
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef char err[ERR_BUF_SIZE]
         result = self.app_.OpenFont(src, werr, ERR_BUF_SIZE)
         if result != True:             
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))
         result = self.app_.GotoFont(werr, ERR_BUF_SIZE)
         if result != True:
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))     

     def __dealloc__(self):
         del self.app_

     def compile_all(self, legacy: bint = False, variationCompositeGuard: bint = True) -> None:
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef char err[ERR_BUF_SIZE]
         result = self.app_.CompileAll(True, legacy, variationCompositeGuard, werr, ERR_BUF_SIZE)
         if(result != True):
             raise CompileError(self.app_.wCharToChar(err, werr))

     def compile_glyph_range(self, start: int = 0, end: int = 0, legacy: bint = False, variationCompositeGuard: bint = True) -> None:
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef char err[ERR_BUF_SIZE]
         result = self.app_.CompileGlyphRange(start, end, True, legacy, variationCompositeGuard, werr, ERR_BUF_SIZE)
         if(result != True):
             raise CompileError(self.app_.wCharToChar(err, werr))

     def save_font(self, path: Path = None, level: StripLevel = None) -> None:
         cdef char err[ERR_BUF_SIZE]
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef string dest

         if(level is None):
            level = StripLevel.STRIP_NOTHING

         if(path is None):
            result = self.app_.SaveFont(level, werr, ERR_BUF_SIZE)
            if(result != True):
                raise FileNotFoundError(self.app_.wCharToChar(err, werr))
         else:
            dest = bytes(path)
            result = self.app_.SaveFont(dest, level, werr, ERR_BUF_SIZE)
            if(result != True):
                raise FileNotFoundError(self.app_.wCharToChar(err, werr))


         

         

     


         

