#cython: language_level=3
#distutils: language = c++
from enum import IntEnum
from .cvttcompilepy cimport *
from pathlib import Path
from io import BytesIO
import pathlib
import fontTools
from fontTools import ttLib 
from cpython cimport array
import array

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
          
     def __cinit__(self, font):         
         self.app_ = new Application()    
         self.app_.Create()
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef char err[ERR_BUF_SIZE]  
         cdef array.array a = array.array('B')
         if(type(font)) == str:
             font = Path(font)
         # path input case
         if isinstance(font, pathlib.PurePath):
             src = bytes(font)             
             result = self.app_.OpenFont(src, werr, ERR_BUF_SIZE)
             if result != True:             
                 raise FileNotFoundError(self.app_.wCharToChar(err, werr))
             result = self.app_.GotoFont(werr, ERR_BUF_SIZE)
             if result != True:
                 raise FileNotFoundError(self.app_.wCharToChar(err, werr))   
         # fontTools TTFont input case
         elif isinstance(font,fontTools.ttLib.ttFont.TTFont):
             font_image = BytesIO()
             font.save(font_image)
             font_size = font_image.getbuffer().nbytes
             # memoryview of font_image
             rawbytes = font_image.getbuffer()
             # array.array of memoryview
             a.frombytes(rawbytes)             
             result = self.app_.OpenMemFont(a.data.as_voidptr,font_size,werr,ERR_BUF_SIZE)
             if result != True:             
                 raise FileNotFoundError(self.app_.wCharToChar(err,werr))
             result = self.app_.GotoFont(werr, ERR_BUF_SIZE)
             if result != True:
                 raise FileNotFoundError(self.app_.wCharToChar(err,werr))    
         else:
              raise ValueError("Parameter not valid")   

     @classmethod 
     def from_file(cls, file):          
         return cls(file)

     @classmethod 
     def from_ttfont(cls, font): 
         return cls(font)

     def __dealloc__(self):         
         del self.app_

     def import_source_from_binary(self) -> None:
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef char err[ERR_BUF_SIZE]
         result = self.app_.ImportSourceFromBinary(werr, ERR_BUF_SIZE)
         if(result != True):
             raise ImportError(self.app_.wCharToChar(err, werr))

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

     def get_ttfont(self, level: StripLevel = None):
         cdef char err[ERR_BUF_SIZE]
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef array.array a = array.array('B')

         if(level is None):
            level = StripLevel.STRIP_NOTHING

         # build step separately so we can get size of final image for resize of array
         result = self.app_.BuildFont( level, werr, ERR_BUF_SIZE)
         if(result != True):
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))

         size = self.app_.GetFontSize()
         array.resize(a, size)         

         result = self.app_.GetMemFont(a.data.as_voidptr, size, werr, ERR_BUF_SIZE)
         if(result != True):
             raise FileNotFoundError(self.app_.wCharToChar(err, werr))

         b = BytesIO(a.tobytes())           

         return ttLib.TTFont(b)

     def save_font(self, file = None, level: StripLevel = None) -> None:
         cdef char err[ERR_BUF_SIZE]
         cdef wchar_t werr[ERR_BUF_SIZE]
         cdef string dest

         if(level is None):
            level = StripLevel.STRIP_NOTHING         

         if(file is not None):
            if(type(file)) == str:
                file = Path(file)
            dest = bytes(file)

         result = self.app_.SaveFont(dest, level, werr, ERR_BUF_SIZE)
         if(result != True):
            raise FileNotFoundError(self.app_.wCharToChar(err, werr))


         

         

     


         

