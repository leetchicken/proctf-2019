CXXFLAGS = -c -g -std=c++17 -mavx2
SOURCES = main.cpp dispatch.cpp kernels.cpp httpserver.cpp log.cpp png.cpp signature_verifier.cpp misc.cpp
LDFLAGS = -lmicrohttpd -lssl -lcrypto
OBJS = $(SOURCES:.cpp=.o)

all: release

debug: CXXFLAGS += -O0 -DDEBUG=1
debug: OUTPUT = convolution-dbg
debug: convolution

release: CXXFLAGS += -O2
release: OUTPUT = convolution
release: convolution

convolution: $(OBJS) convolution.asm
	compiler/compiler convolution.asm -elf convolution_kernel.o
	g++ $(OBJS) convolution_kernel.o $(LDFLAGS) -o $(OUTPUT)

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o convolution convolution-dbg
