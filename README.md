# Project

Microsoft Visual TrueType(VTT) is a professional-level tool for graphically instructing TrueType and OpenType fonts. 
For details on the tool visit https://docs.microsoft.com/en-us/typography/tools/vtt/. 

Visual TrueType compiles binary from source formats included in the fonts that are instructed with Visual TrueType. 

This repo contains source for VTTCompile which is a standalone tool can compile the high level VTT Talk source
to a lower level assembler source and then can assemble that source to cooresponding binaries. Since VTT can also 
generate variation 'cvar' for variable fonts, VTTCompile can also compile the 'cvar' from source format. VTTCompile
also includes file management functions to prepare a font for production and to strip source formats for a final 
'ship' font binary. 

The interface for VTTCompile is similar to VTTShell that is included if Visual TrueType download package. 

The source code in the repo is mostly a subset of the source code of Visual TrueType as needed to produce VTTCompile. 
However not all of the source code included in the repo is necessary to produce VTTCompile but we optomized for including 
as complete source files as possible to make future maintenance easier. 

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
