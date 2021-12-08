# Project

VTTCompilePy is a Python extension built using Cython. It provides streamlined bindings for
various compilers from Visual TrueType. 

VTTCompilePy was developed to support a Python based font development environment. In addition to the Python interface,
a command line interface is also installed. Usage is available with "vttcompilepy --help". 

### Example

```python
    import sys 
    from pathlib import Path
    import vttcompilepy as vtt

    TESTDATA = Path(__file__).parent / "data"
    IN_PATH = TESTDATA / "selawik-variable.ttf"
    OUT_PATH = TESTDATA / "out.ttf"

    print(bytes(IN_PATH))

    print('VTTCompilePy Test Client')

    compiler = vtt.Compiler(IN_PATH)

    compiler.compile_all()

    compiler.save_font(OUT_PATH, vtt.StripLevel.STRIP_NOTHING)

```

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
