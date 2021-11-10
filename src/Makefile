OBJS = \
Area.o CvtManager.o File.o List.o MathUtils.o Memory.o Platform.o TextBuffer.o \
TMTParser.o TTAssembler.o TTEngine.o TTFont.o TTGenerator.o Variation.o \
VariationInstance.o VariationModels.o vttcompile.o application.o 

.cpp.o :

.PHONY : clean

default: vttcompile

vttcompile: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	$(RM) $(OBJS) vttcompile vttcompile.exe
