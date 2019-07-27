CXXFLAGS = -c -g -std=c++17 -O2
SOURCES = png.cpp
OBJS = $(SOURCES:.cpp=.o)

all: libpng

libpng: $(OBJS)
	ar rvs libpng.a $(OBJS)

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.a

