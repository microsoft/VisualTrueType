import vttcompilepy as vtt

import argparse
import sys
from pathlib import Path

def main(args=None):
    if args is None:
        args = sys.argv[1:]
    parser = argparse.ArgumentParser()

    parser.add_argument("input", help="Input file")
    parser.add_argument("output", help="Output file")

    cgroup = parser.add_argument_group("Compile options")
    cgroup.add_argument("-a", "--all", action="store_true", help ="Compile everything")
    cgroup.add_argument("-l", "--legacy", action="store_true", help ="Rare cases for very old fonts")
    cgroup.add_argument("-v", "--variationcompositeguard", action="store_true", help ="Disable variation composite guard (default: enabled)")

    sgroup = parser.add_argument_group("Strip options")
    sgroup.add_argument("-s", "--source", action="store_true", help="Strip source")
    sgroup.add_argument("-b", "--hints", action="store_true", help="Strip source and hints")
    sgroup.add_argument("-c", "--cache", action="store_true", help="Strip source and hints and cache tables")
    
    args = parser.parse_args(args)

    print(args.input)
    print(args.output)
    inpath = Path(args.input)
    outpath = Path(args.output)

    compiler = vtt.Compiler(inpath)

    legacy = False
    variationCompositeGuard = True 

    if args.legacy:
        legacy = True

    if args.variationcompositeguard:
        variationCompositeGuard = False

    if args.all:
        compiler.compile_all(legacy, variationCompositeGuard)

    #StripCommand strip = stripNothing;
	#if (bStripCache) strip = stripBinary;
	#else if (bStripHints) strip = stripHints;
	#else if (bStripSource) strip = stripSource;

    level = vtt.StripLevel.STRIP_NOTHING
    
    if args.cache:
        level = vtt.StripLevel.STRIP_BINARY
    elif args.hints:
        level = vtt.StripLevel.STRIP_HINTS
    elif args.source:
        level = vtt.StripLevel.STRIP_SOURCE

    compiler.save_font(outpath, level)

if __name__ == "__main__":
    sys.exit(main())
