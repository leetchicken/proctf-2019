#.PHONY: debug release clean
CXXFLAGS = -c -g -std=c++17 -I../libpng -Wall
SOURCES = misc.cpp log.cpp notification.cpp network.cpp user.cpp team.cpp game.cpp main.cpp httpserver.cpp checksystem.cpp
LDFLAGS = -lmicrohttpd -lpugixml -L../libpng -lpng -lrt -pthread
OBJS = $(SOURCES:.cpp=.o)

all: release

debug: CXXFLAGS += -O0 -DDEBUG=1
debug: OUTPUT = server-dbg
debug: server

release: CXXFLAGS += -O2
release: OUTPUT = server
release: server

server: $(OBJS)
	g++ $(OBJS) $(LDFLAGS) -o $(OUTPUT)

.cpp.o:
	g++ $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o server server-dbg

