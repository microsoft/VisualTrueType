OBJS = \
Area.o CvtManager.o File.o List.o MathUtils.o Memory.o Platform.o TextBuffer.o \
TMTParser.o TTAssembler.o TTEngine.o TTFont.o TTGenerator.o Variation.o \
VariationInstance.o VariationModels.o vttcompile.o

# TTFont.cpp needs -fpermissive, TMTParser.cpp needs -finput-charset=cp1252
CXXFLAGS = -fpermissive -finput-charset=cp1252

.cpp.o :

.PHONY : clean

default: vttcompile

vttcompile: $(OBJS)
	$(CXX) -o $@ $^

clean :
	$(RM) $(OBJS) vttcompile vttcompile.exe
