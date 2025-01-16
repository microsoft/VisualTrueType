# -*- coding: utf-8 -*-
from io import open
import os
import sys
import platform
from setuptools import Extension, setup
from Cython.Build import cythonize

# from _version import __version__ as vttpy_version

here = os.path.abspath(os.path.dirname(__file__))

# Get the long description from the pypi.md file
with open(os.path.join(here, "pypi.md"), encoding="utf-8") as f:
    long_description = f.read()

define_macros = [("UNICODE", 1), ("_UNICODE", 1)]
linetrace = False
if int(os.environ.get("CYTHON_LINETRACE", "0")):
    linetrace = True
    define_macros.append(("CYTHON_TRACE_NOGIL", "1"))

extra_compile_args = []

if platform.system() != "Windows":
    extra_compile_args.append("-std=c++14")

# Check if the operating system is Windows and the Python version is less than 3.13.
# If both conditions are met, append the "-sdl" flag to the extra_compile_args list
# to enable additional security checks during compilation.
#
# The reason we have to exclude Python 3.13 is Py_UNICODE is deprecated warning and Py_UNICODE
# is used by Cython array.array that is used in this project.
# The Cython issue is tracked here:
# https://github.com/cython/cython/issues/6607
if platform.system() == "Windows" and sys.version_info < (3, 13):
    extra_compile_args.append("-sdl")

extension = Extension(
    "vttcompilepy.vttcompilepy",
    define_macros=define_macros,
    include_dirs=[".", "src"],
    sources=[
        "vttcompilepy/vttcompilepy.pyx",
        "src/application.cpp",
        "src/CvtManager.cpp",
        "src/File.cpp",
        "src/List.cpp",
        "src/MathUtils.cpp",
        "src/Memory.cpp",
        "src/Platform.cpp",
        "src/TextBuffer.cpp",
        "src/TMTParser.cpp",
        "src/TTAssembler.cpp",
        "src/TTEngine.cpp",
        "src/TTFont.cpp",
        "src/TTGenerator.cpp",
        "src/Variation.cpp",
        "src/VariationInstance.cpp",
        "src/VariationModels.cpp",
        "src/ttiua.cpp",
    ],
    language="c++",
    extra_compile_args=extra_compile_args,
)

setup(
    name="vttcompilepy",
    # version= vttpy_version,
    version="0.0.1.12",
    description="Python extension for Visual TrueType font compile. ",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="Paul Linnerud",
    author_email="paulli@microsoft.com",
    url="https://github.com/microsoft/VisualTrueType",
    license="MIT",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
    ],
    package_dir={"": "."},
    packages=["vttcompilepy"],
    zip_safe=False,
    install_requires=[
        "fonttools>=4.33.0",
    ],
    python_requires=">=3.7",
    ext_modules=cythonize(
        extension,
        annotate=bool(int(os.environ.get("CYTHON_ANNOTATE", "0"))),
        compiler_directives={"linetrace": linetrace},
    ),
    entry_points={
        "console_scripts": [
            "vttcompilepy = vttcompilepy.__main__:main",
            "quit_to_glyphs = vttcompilepy.quit_to_glyphs:main",
        ]
    },
)
