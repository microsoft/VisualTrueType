# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import vttcompilepy as vtt
from fontTools.ttLib import TTFont
from pathlib import Path
import pytest

TESTDATA = Path(__file__).parent / "data"
IN_SELAWIK = TESTDATA / "Selawik-variable.ttf"

@pytest.fixture
def original_font():
    return TTFont(IN_SELAWIK)

@pytest.fixture
def compiled_font_file():
    compiler = vtt.Compiler(IN_SELAWIK)
    compiler.compile_all()
    out_compiled = TESTDATA / "out_c1.ttf"
    compiler.save_font(out_compiled, vtt.StripLevel.STRIP_NOTHING)
    return TTFont(out_compiled)

@pytest.fixture
def compiled_font_file_str():
    in_selawik_str = str(IN_SELAWIK)
    compiler = vtt.Compiler(in_selawik_str)
    compiler.compile_all()
    out_compiled_str = str(TESTDATA / "out_c2.ttf")
    compiler.save_font(out_compiled_str, vtt.StripLevel.STRIP_NOTHING)
    return TTFont(out_compiled_str)

@pytest.fixture
def compiled_font_mem():
    tt = TTFont(IN_SELAWIK)
    compiler = vtt.Compiler(tt)
    compiler.compile_all()
    tt1 = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    return tt1

@pytest.fixture
def compiled_font_file_mem():
    compiler = vtt.Compiler(IN_SELAWIK)
    compiler.compile_all()
    tt1 = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    return tt1

@pytest.fixture
def compiled_font_mem_file():
    tt = TTFont(IN_SELAWIK)
    compiler = vtt.Compiler(tt)
    compiler.compile_all()
    out_compiled = TESTDATA / "out_c3.ttf"
    compiler.save_font(out_compiled, vtt.StripLevel.STRIP_NOTHING)
    return TTFont(out_compiled)

@pytest.fixture
def compiled_stripped_font_file():
    compiler = vtt.Compiler(IN_SELAWIK)
    compiler.compile_all()
    out_compiled_stripped = TESTDATA / "out_c_s1.ttf"
    compiler.save_font(out_compiled_stripped, vtt.StripLevel.STRIP_SOURCE)
    return TTFont(out_compiled_stripped)

@pytest.fixture
def compiled_stripped_font_mem():
    tt = TTFont(IN_SELAWIK)
    compiler = vtt.Compiler(tt)
    compiler.compile_all()
    tt1 = compiler.get_ttfont(vtt.StripLevel.STRIP_SOURCE)
    return tt1

@pytest.fixture
def compiled_source_from_bin_font_mem():
    tt = TTFont(IN_SELAWIK)
    compiler = vtt.Compiler(tt)
    compiler.import_source_from_binary()
    compiler.compile_all()
    tt1 = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    return tt1


def compare_fonts(ttorig, ttcomp) -> None:
   assert ttorig['maxp'].numGlyphs == ttcomp['maxp'].numGlyphs
   assert ttorig['maxp'] == ttcomp['maxp']
   assert ttorig['fpgm'] == ttcomp['fpgm']
   assert ttorig['prep'] == ttcomp['prep']

   glyf_orig = ttorig['glyf']
   glyf_comp = ttcomp['glyf']

   for glyph1 in ttorig.glyphOrder:
       print(glyph1)
       assert glyf_orig[glyph1].isComposite() == glyf_comp[glyph1].isComposite()
       assert glyf_orig[glyph1].getCoordinates(glyf_orig) == glyf_comp[glyph1].getCoordinates(glyf_comp)
       assert glyf_orig[glyph1].getComponentNames(glyf_orig) == glyf_comp[glyph1].getComponentNames(glyf_comp)
       assert hasattr(glyf_orig[glyph1],'program') == hasattr(glyf_comp[glyph1],'program') 
       haveInstructions = hasattr(glyf_orig[glyph1], "program")
       if haveInstructions:
           #orig_codes = glyf_orig[glyph1].program.getBytecode()
           orig_assembly = glyf_orig[glyph1].program.getAssembly()
           print(orig_assembly)
           #comp_codes = glyf_comp[glyph1].program.getBytecode()
           comp_assembly = glyf_comp[glyph1].program.getAssembly()
           print(comp_assembly)
           assert orig_assembly == comp_assembly

def check_stripped(ttorig, ttstrip) -> None:
    assert("TSI0" in ttorig)
    assert("TSI0" not in ttstrip)

    assert("TSI1" in ttorig)
    assert("TSI1" not in ttstrip)

    assert("TSI2" in ttorig)
    assert("TSI2" not in ttstrip)

    assert("TSI3" in ttorig)
    assert("TSI3" not in ttstrip)

    assert("TSI5" in ttorig)
    assert("TSI5" not in ttstrip)


def test_compiled_file_file_path(original_font, tmp_path: Path, compiled_font_file):
   compare_fonts(original_font, compiled_font_file)  

def test_compiled_file_file_str(original_font, tmp_path: Path, compiled_font_file_str):
   compare_fonts(original_font, compiled_font_file_str)  

def test_stripped_file_file(original_font, tmp_path: Path, compiled_stripped_font_file):
    check_stripped(original_font, compiled_stripped_font_file)

def test_compiled_file_mem(original_font, compiled_font_file_mem):
   compare_fonts(original_font, compiled_font_file_mem)  

def test_compiled_mem_file(original_font, compiled_font_mem_file):
   compare_fonts(original_font, compiled_font_mem_file)  

def test_compile_mem_mem(original_font, compiled_font_mem):
    compare_fonts(original_font, compiled_font_mem)

def test_stripped_mem_mem(original_font, compiled_stripped_font_mem):
    check_stripped(original_font, compiled_stripped_font_mem)

def test_import_source_from_binary_mem(original_font, compiled_source_from_bin_font_mem):
    compare_fonts(original_font, compiled_source_from_bin_font_mem)
    