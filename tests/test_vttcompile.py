# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import vttcompilepy as vtt
from fontTools.ttLib import TTFont
from pathlib import Path
import pytest

TESTDATA = Path(__file__).parent / "data"
IN_SELAWIK = TESTDATA / "Selawik-variable.ttf"
OUT_COMPILED = TESTDATA / "out_c.ttf"
OUT_COMPILED_STRIPPED = TESTDATA / "out_c_s.ttf"

compiler = vtt.Compiler(IN_SELAWIK)
compiler.compile_all()
compiler.save_font(OUT_COMPILED, vtt.StripLevel.STRIP_NOTHING)
compiler.save_font(OUT_COMPILED_STRIPPED, vtt.StripLevel.STRIP_SOURCE)

@pytest.fixture
def original_font():
    return TTFont(IN_SELAWIK)

@pytest.fixture
def compiled_font():
    return TTFont(OUT_COMPILED)

@pytest.fixture
def compiled_stripped_font():
    return TTFont(OUT_COMPILED_STRIPPED)

def test_compiled(original_font, tmp_path: Path, compiled_font):
   ttorig = original_font
   ttcomp = compiled_font

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


def test_stripped(original_font, tmp_path: Path, compiled_stripped_font):
    ttorig = original_font
    ttstrip = compiled_stripped_font

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

    






