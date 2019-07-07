CXXFLAGS = -c -g -std=c++17 -O2
SOURCES = main.cpp png.cpp
OBJS = $(SOURCES:.cpp=.o)

all: $(OBJS)
	g++ $(OBJS) -o build_bmp

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o build_bmp

