# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import vttcompilepy as vtt
from fontTools.ttLib import TTFont
from pathlib import Path
import struct
import pytest
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib.tables.DefaultTable import DefaultTable

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
def compiled_source_from_bin_font_file():
    compiler = vtt.Compiler(IN_SELAWIK)
    compiler.import_source_from_binary()
    out_import = TESTDATA / "out_c_c4i.ttf"
    compiler.save_font(out_import, vtt.StripLevel.STRIP_NOTHING)
    compiler.compile_all()
    out_import_compiled = TESTDATA / "out_c_c4.ttf"
    compiler.save_font(out_import_compiled, vtt.StripLevel.STRIP_NOTHING)
    return TTFont(out_import_compiled)


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

def test_import_source_from_binary_file(original_font, compiled_source_from_bin_font_file):
    compare_fonts(original_font, compiled_source_from_bin_font_file)

def test_import_source_from_binary_mem(original_font, compiled_source_from_bin_font_mem):
    compare_fonts(original_font, compiled_source_from_bin_font_mem)


@pytest.mark.parametrize("num_metrics", [0, 3000])
def test_invalid_horizontal_metric_count(tmp_path, num_metrics):
    with TTFont(IN_SELAWIK) as font:
        hhea_offset = font.reader.tables["hhea"].offset
    data = bytearray(IN_SELAWIK.read_bytes())
    struct.pack_into(">H", data, hhea_offset + 34, num_metrics)
    path = tmp_path / "invalid-metrics.ttf"
    path.write_bytes(data)

    message = ("invalid horizontal metric count" if num_metrics == 0
               else "invalid horizontal metrics table length")
    with pytest.raises(FileNotFoundError, match=message):
        vtt.Compiler(path)


@pytest.mark.parametrize("invalid_field", ["length", "offset"])
def test_invalid_horizontal_metrics_extent(tmp_path, invalid_field):
    data = bytearray(IN_SELAWIK.read_bytes())
    num_tables = struct.unpack_from(">H", data, 4)[0]
    for entry in range(12, 12 + 16 * num_tables, 16):
        if data[entry:entry + 4] == b"hmtx":
            if invalid_field == "length":
                length = struct.unpack_from(">I", data, entry + 12)[0]
                struct.pack_into(">I", data, entry + 12, length - 1)
            else:
                struct.pack_into(">I", data, entry + 8, len(data))
            break
    path = tmp_path / "invalid-metrics-extent.ttf"
    path.write_bytes(data)

    message = ("invalid horizontal metrics table length" if invalid_field == "length"
               else "invalid table directory or table extent")
    with pytest.raises(FileNotFoundError, match=message):
        vtt.Compiler(path)


@pytest.mark.parametrize("num_metrics", [1, 1100])
@pytest.mark.parametrize("private_indices", [False, True])
def test_large_horizontal_metrics(tmp_path, num_metrics, private_indices):
    glyph_order = [".notdef"] + ["glyph%04d" % i for i in range(1, 1100)]
    builder = FontBuilder(1000, isTTF=True)
    builder.setupGlyphOrder(glyph_order)
    builder.setupCharacterMap({ord("H"): glyph_order[1], ord("x"): glyph_order[2]})
    glyphs = {name: TTGlyphPen(None).glyph() for name in glyph_order}
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((100, 0))
    pen.lineTo((100, 100))
    pen.lineTo((0, 100))
    pen.closePath()
    glyphs[".notdef"] = pen.glyph()
    builder.setupGlyf(glyphs)
    metrics = {
        name: (500 + min(i, num_metrics - 1), i % 21 - 10)
        for i, name in enumerate(glyph_order)
    }
    builder.setupHorizontalMetrics(metrics)
    builder.setupHorizontalHeader(ascent=800, descent=-200)
    builder.setupNameTable({"familyName": "Metrics test", "styleName": "Regular"})
    builder.setupOS2()
    builder.setupPost()
    builder.setupMaxp()
    if private_indices:
        # Older source indices can cover fewer glyphs than maxp/loca.
        indices = b"".join(
            struct.pack(">HHI", code, 0, offset)
            for code, offset in [
                (0, 0), (0xFFFE, 0xABFC1F34), (0xFFFA, 0),
                (0xFFFB, 0), (0xFFFC, 0), (0xFFFD, 0),
            ]
        )
        for tag in ("TSI0", "TSI2"):
            table = DefaultTable(tag)
            table.data = indices
            builder.font[tag] = table
    path = tmp_path / "large-metrics.ttf"
    builder.save(path)

    compiler = vtt.Compiler(path)
    result = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    assert result["hmtx"].metrics == metrics


def table_directory_entry(data, tag):
    num_tables = struct.unpack_from(">H", data, 4)[0]
    for entry in range(12, 12 + 16 * num_tables, 16):
        if data[entry:entry + 4] == tag:
            return entry
    raise AssertionError("Missing table: %r" % tag)


@pytest.mark.parametrize("variant", ["glyph-count", "hhea-count", "hmtx-offset", "name-length"])
def test_reported_malformed_fonts(tmp_path, variant):
    data = bytearray(IN_SELAWIK.read_bytes())
    hhea = table_directory_entry(data, b"hhea")
    hhea_offset = struct.unpack_from(">I", data, hhea + 8)[0]
    if variant == "glyph-count":
        maxp = table_directory_entry(data, b"maxp")
        maxp_offset = struct.unpack_from(">I", data, maxp + 8)[0]
        struct.pack_into(">H", data, maxp_offset + 4, 3000)
        struct.pack_into(">I", data, table_directory_entry(data, b"loca") + 12, 12004)
        struct.pack_into(">I", data, table_directory_entry(data, b"TSI0") + 12, 8)
        struct.pack_into(">H", data, hhea_offset + 34, 1)
        message = "invalid table directory or table extent"
    elif variant == "hhea-count":
        struct.pack_into(">H", data, hhea_offset + 34, 3000)
        message = "invalid horizontal metrics table length"
    elif variant == "hmtx-offset":
        struct.pack_into(">I", data, table_directory_entry(data, b"hmtx") + 8, 0x200000)
        message = "invalid table directory or table extent"
    else:
        struct.pack_into(">I", data, table_directory_entry(data, b"name") + 12, 0x7FFF0000)
        message = "invalid table directory or table extent"
    path = tmp_path / (variant + ".ttf")
    path.write_bytes(data)

    with pytest.raises(FileNotFoundError, match=message):
        vtt.Compiler(path)


@pytest.mark.parametrize("size", [0, 11, 12, 27])
def test_truncated_font_directory(tmp_path, size):
    path = tmp_path / "truncated-directory.ttf"
    path.write_bytes(IN_SELAWIK.read_bytes()[:size])
    with pytest.raises(FileNotFoundError, match="invalid table directory or table extent"):
        vtt.Compiler(path)


@pytest.mark.parametrize("tag,size", [(b"head", 53), (b"hhea", 35), (b"maxp", 31)])
def test_truncated_required_header(tmp_path, tag, size):
    data = bytearray(IN_SELAWIK.read_bytes())
    struct.pack_into(">I", data, table_directory_entry(data, tag) + 12, size)
    path = tmp_path / "truncated-header.ttf"
    path.write_bytes(data)
    with pytest.raises(FileNotFoundError, match="missing or truncated head, hhea or maxp"):
        vtt.Compiler(path)


@pytest.mark.parametrize("offset,length", [
    (0, 4), (0xFFFFFFFF, 4), (0xFFFFFFF0, 32), (12, 4),
])
def test_invalid_retained_table_extent(tmp_path, offset, length):
    data = bytearray(IN_SELAWIK.read_bytes())
    entry = table_directory_entry(data, b"name")
    struct.pack_into(">II", data, entry + 8, offset, length)
    path = tmp_path / "invalid-retained-table.ttf"
    path.write_bytes(data)
    with pytest.raises(FileNotFoundError, match="invalid table directory or table extent"):
        vtt.Compiler(path)


@pytest.mark.parametrize("compile_all", [False, True])
@pytest.mark.parametrize("empty", [False, True])
def test_retained_table_round_trip(tmp_path, compile_all, empty):
    with TTFont(IN_SELAWIK) as font:
        table = DefaultTable("TEST")
        table.data = b"" if empty else b"retained table bytes"
        font["TEST"] = table
        path = tmp_path / "retained-table.ttf"
        font.save(path)
    if empty:
        data = bytearray(path.read_bytes())
        struct.pack_into(">I", data, table_directory_entry(data, b"TEST") + 8, 0xFFFFFFFF)
        path.write_bytes(data)

    compiler = vtt.Compiler(path)
    if compile_all:
        compiler.compile_all()
    result = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    assert result.getTableData("TEST") == table.data


@pytest.mark.parametrize("compile_all", [False, True])
@pytest.mark.parametrize("missing_sources", [False, True])
def test_rebuild_directory_capacity(tmp_path, compile_all, missing_sources):
    with TTFont(IN_SELAWIK) as font:
        if missing_sources:
            for tag in ("TSI0", "TSI1", "TSI2", "TSI3", "TSI5"):
                del font[tag]
        # 61 entries fit the internal 1,000-byte directory; rebuilding may add more.
        target_count = 61 if missing_sources else 62
        for i in range(target_count - (len(font.keys()) - 1)):
            tag = "X%03d" % i
            table = DefaultTable(tag)
            table.data = b""
            font[tag] = table
        path = tmp_path / "full-directory.ttf"
        font.save(path)
    compiler = vtt.Compiler(path)
    with pytest.raises((FileNotFoundError, vtt.CompileError), match="directory capacity"):
        if compile_all:
            compiler.compile_all()
        else:
            compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)


@pytest.mark.parametrize("compile_all", [False, True])
@pytest.mark.parametrize("variant", ["excess-metrics", "excess-loca", "stale-maxima"])
def test_intermediate_font_round_trip(tmp_path, compile_all, variant):
    with TTFont(IN_SELAWIK) as original:
        glyph_order = original.getGlyphOrder()
        metrics = original["hmtx"].metrics
        loca_format = "H" if original["head"].indexToLocFormat == 0 else "I"
        max_points = original["maxp"].maxPoints
        max_contours = original["maxp"].maxContours
    data = bytearray(IN_SELAWIK.read_bytes())
    if variant == "excess-metrics":
        # All declared records exist, even though some have no current glyph.
        contents = b"".join(struct.pack(">Hh", *metrics[name]) for name in glyph_order)
        contents += struct.pack(">Hh", 500, 0) * (1100 - len(glyph_order))
        entry = table_directory_entry(data, b"hmtx")
        hhea = table_directory_entry(data, b"hhea")
        hhea_offset = struct.unpack_from(">I", data, hhea + 8)[0]
        struct.pack_into(">H", data, hhea_offset + 34, 1100)
    elif variant == "excess-loca":
        # Preserve the historical tolerance for extra empty glyph locations.
        entry = table_directory_entry(data, b"loca")
        offset, length = struct.unpack_from(">II", data, entry + 8)
        contents = data[offset:offset + length]
        entry_size = struct.calcsize(loca_format)
        contents += contents[-entry_size:] * (1101 - length // entry_size)
    else:
        maxp = table_directory_entry(data, b"maxp")
        offset = struct.unpack_from(">I", data, maxp + 8)[0]
        struct.pack_into(">HH", data, offset + 6, 0, 0)
    if variant != "stale-maxima":
        data.extend(b"\0" * (-len(data) % 4))
        struct.pack_into(">II", data, entry + 8, len(data), len(contents))
        data.extend(contents)
    path = tmp_path / (variant + ".ttf")
    path.write_bytes(data)

    compiler = vtt.Compiler(path)
    if compile_all:
        compiler.compile_all()
    result = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
    assert result.getGlyphOrder() == glyph_order
    assert result["hmtx"].metrics == metrics
    assert 0 < result["hhea"].numberOfHMetrics <= len(glyph_order)
    assert len(result["loca"].locations) == len(glyph_order) + 1
    assert result["maxp"].maxPoints == max_points
    assert result["maxp"].maxContours == max_contours


def test_missing_glyph_locations(tmp_path):
    # An absent/zero-length loca table is rejected outright: there is no
    # readable glyph-location data at all.
    data = bytearray(IN_SELAWIK.read_bytes())
    struct.pack_into(">I", data, table_directory_entry(data, b"loca") + 12, 0)
    path = tmp_path / "missing-glyph-locations.ttf"
    path.write_bytes(data)
    with pytest.raises(FileNotFoundError, match="missing glyph locations"):
        vtt.Compiler(path)


def test_zero_glyph_loca_is_not_treated_as_missing(tmp_path):
    # A loca table present with just the one (0-glyph) entry is structurally
    # valid data, not "missing" data; it is rejected later for having zero
    # glyphs rather than being mislabeled as a missing table.
    data = bytearray(IN_SELAWIK.read_bytes())
    struct.pack_into(">I", data, table_directory_entry(data, b"loca") + 12, 2)
    path = tmp_path / "zero-glyph-loca.ttf"
    path.write_bytes(data)
    with pytest.raises(FileNotFoundError, match="invalid horizontal metric count"):
        vtt.Compiler(path)
    