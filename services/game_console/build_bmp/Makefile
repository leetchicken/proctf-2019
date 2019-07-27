CXXFLAGS = -c -g -std=c++17 -O2 -I../libpng
SOURCES = main.cpp
LDFLAGS = -L../libpng -lpng
OBJS = $(SOURCES:.cpp=.o)

all: $(OBJS)
	g++ $(OBJS) $(LDFLAGS) -o build_bmp

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o build_bmp

