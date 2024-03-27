import sys
from fontTools.ttLib import TTFont
import vttcompilepy as vtt
import argparse


def add_quit_to_glyph_program(
    font_path, output_path, compile, start_index, end_index, remove, iterate
):
    try:
        # Load the font
        font = TTFont(font_path)

        # Access the TSI3 table (if it exists)
        if "TSI3" in font:
            tsi3_table = font["TSI3"]
            glyph_set = font.getGlyphSet()
            num_glyphs = len(glyph_set)
            if end_index is None:
                end_index = num_glyphs - 1
            glyph_ids = list(range(start_index, end_index + 1))
            glyph_names = font.getGlyphNameMany(glyph_ids)
            added = 0
            removed = 0

            for glyph_name, glyph_id in zip(glyph_names, glyph_ids):
                if glyph_name not in tsi3_table.glyphPrograms:
                    print(f"{glyph_name} id = {glyph_id} does not have a glyph program")
                    continue
                if iterate:
                    try:
                        print(f"Iterate - trying {glyph_name} id = {glyph_id}")
                        compiler = vtt.Compiler(font)
                        compiler.compile_glyph_range(glyph_id, glyph_id)

                    except Exception:
                        # If compilation fails, add quit()
                        # Check if the glyph program already starts with "quit()"
                        if not tsi3_table.glyphPrograms[glyph_name].startswith(
                            "quit()"
                        ):
                            tsi3_table.glyphPrograms[glyph_name] = (
                                "quit() \n" + tsi3_table.glyphPrograms[glyph_name]
                            )
                            print(f"Added quit() to {glyph_name} id = {glyph_id}")
                            added += 1

                # Add or remove "quit()" to/from the beginning of each glyphProgram
                elif remove:
                    # Check if the glyph program contains "quit()"
                    if "quit()" in tsi3_table.glyphPrograms[glyph_name]:
                        tsi3_table.glyphPrograms[glyph_name] = tsi3_table.glyphPrograms[
                            glyph_name
                        ].replace("quit()", "")
                        print(f"Removed quit() from {glyph_name} id = {glyph_id}")
                        removed += 1
                else:
                    # Check if the glyph program already starts with "quit()"
                    if not tsi3_table.glyphPrograms[glyph_name].startswith("quit()"):
                        tsi3_table.glyphPrograms[glyph_name] = (
                            "quit() \n" + tsi3_table.glyphPrograms[glyph_name]
                        )
                        print(f"Added quit() to {glyph_name} id = {glyph_id}")
                        added += 1

            if compile or iterate:
                # Compile the font
                compiler = vtt.Compiler(font)
                compiler.compile_all()
                font = compiler.get_ttfont(vtt.StripLevel.STRIP_NOTHING)
                print("Font compiled")

            # Save the modified font to the output path
            font.save(output_path)

            print(f"Modified font saved to {output_path}")
            print(f"Added quit() to {added} glyphs")
            print(f"Removed quit() from {removed} glyphs")
        else:
            print("The font does not have a TSI3 table")

    except Exception as e:
        print(f"Error: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Add or remove quit to/from glyph program."
    )
    parser.add_argument("input_font_path", help="The path to the input font file.")
    parser.add_argument("output_font_path", help="The path to the output font file.")
    parser.add_argument("--compile", action="store_true", help="Compile the font.")
    parser.add_argument("--start", type=int, default=0, help="Start glyph index.")
    parser.add_argument("--end", type=int, default=None, help="End glyph index.")
    parser.add_argument(
        "--remove", action="store_true", help="Remove quit() from glyph program."
    )
    parser.add_argument(
        "--iterate", action="store_true", help="Only add quit() to glyphs that need it."
    )
    args = parser.parse_args()

    add_quit_to_glyph_program(
        args.input_font_path,
        args.output_font_path,
        args.compile,
        args.start,
        args.end,
        args.remove,
        args.iterate,
    )


if __name__ == "__main__":
    main()
