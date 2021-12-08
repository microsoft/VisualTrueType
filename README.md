# Project

Microsoft Visual TrueType(VTT) is a professional-level tool for graphically instructing TrueType and OpenType fonts. 
For details on the tool visit https://docs.microsoft.com/en-us/typography/tools/vtt/. 

Visual TrueType compiles binary from source formats included in the fonts that are instructed with Visual TrueType. 

This repo contains the source for the compilers of Visual TrueType. The compilers can compile the high level VTT Talk source
to a lower level assembler source and then can assemble that source to corresponding binaries. Since VTT can also generate
variation 'cvar' data for variable fonts, the compilers can also produce 'cvar' data from a source format. File management
functions are included to prepare a font for production and to strip source formats for a final 'ship' font binary.

The source in this repo can be compiled and used in two different ways.

First is VTTCompile which is a standalone tool can VTTCompile that runs on the command line. The interface for VTTCompile is
similar to VTTShell which is included in Visual TrueType download package.

Second is VTTCompilePy which is Cython based Python extension that exports interfaces enabling a Python program to compile
and assemble TrueType data. This extension also provides a command line interface to the Python interface. VTTCompilePy is
available on PyPi, please visit https://pypi.org/project/vttcompilepy. 

The source code in the repo is mostly a subset of the source code of Visual TrueType as needed to produce VTTCompile. 
However not all of the source code included in the repo is necessary to produce VTTCompile but we optimized for including 
as complete source files as possible to make future maintenance easier. 

For documentation of the Visual TrueType source formats visit https://docs.microsoft.com/en-us/typography/tools/vtt/tsi-tables. 

## Building

In the "vttcompile" folder, there is a Visual Studio Solution for Microsoft Visual Studio users,
and an Xcode Project For Apple Xcode users.

For unix users (including Linux and Apple command-line), `cd src && make` should work. You
can also cross-compile for 32-bit windows with `cd src && make CXX=i686-w64-mingw32-c++`,
for 64-bit windows with `cd src && make CXX=x86_64-w64-mingw32-c++`; and use clang,
enabling all the recommended warnings with `cd src && make CXX=clang++ CXXFLAGS=-Wall`.
Build as 32-bit on 64-bit systems with `cd src && make CXXFLAGS=-m32`.
You may need to do `cd src && make CXXFLAGS="-std=c++14"` to explicitly request
support for the 2014 ISO C++ standard.

In the "vttcompilepy" folder is the Cython source for the Python extension but the build is 
done through the setup.py file in the main folder. 

Setup a Python environment including dependencies in requirements-dev.txt. 

To build the extension on local machine use "Python setup.py build".
To install the built extension into current Python environment use "Python setup.py install".
To create a distribution package for current system use "Python setup.py bdist_wheel".

The workflow Python Extension uses ciBuildWheel to build the extension across multiple platforms
and optionally upload result to PyPi. 

The minimum compiler requirement is support for the 2014 ISO C++ standard plus amendments.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
