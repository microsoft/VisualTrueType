OBJS = \
Area.o CvtManager.o File.o List.o MathUtils.o Memory.o Platform.o TextBuffer.o \
TMTParser.o TTAssembler.o TTEngine.o TTFont.o TTGenerator.o Variation.o \
VariationInstance.o VariationModels.o vttcompile.o

# TTFont.cpp needs -fpermissive, TMTParser.cpp needs -finput-charset=cp1251
CXXFLAGS = -fpermissive -finput-charset=cp1251

.cpp.o :

default: vttcompile

vttcompile: $(OBJS)
	$(CXX) -o $@ $^

