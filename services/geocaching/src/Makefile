CC = gcc
CXXFLAGS = -DPORTABLE -Wno-deprecated-declarations
LDFLAGS = -l:libprotobuf-c.a -lssl -lcrypto -lsqlite3
SOURCES = main.c geocacher.pb-c.c protocol.c base64.c db.c mpilib.c custom_malloc.c
DEBUG_SOURCES = $(SOURCES) malloc_debug.c

all: release

debug: CXXFLAGS += -DDEBUG 
debug: geocacher-debug

release: CXXFLAGS += -O3
release: geocacher

geocacher: $(SOURCES)
	$(CC) -s $(SOURCES) --std=gnu99  $(CXXFLAGS) -I/usr/local/include -L/usr/local/lib $(LDFLAGS) -o geocacher

geocacher-debug: $(DEBUG_SOURCES)
	$(CC) $(DEBUG_SOURCES) --std=gnu99  $(CXXFLAGS) -I/usr/local/include -L/usr/local/lib $(LDFLAGS) -o geocacher-debug

geocacher.pb-c.c: geocacher.proto
	protoc --c_out=. geocacher.proto

clean:
	rm -f geocacher
