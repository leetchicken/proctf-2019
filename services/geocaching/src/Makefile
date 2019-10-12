CC = gcc
CXXFLAGS = -DPORTABLE -Wno-deprecated-declarations
LDFLAGS = -l:libprotobuf-c.a -lssl -lcrypto -lsqlite3
SOURCES = main.c malloc_debug.c geocacher.pb-c.c protocol.c base64.c db.c mpilib.c

all: release

debug: CXXFLAGS += -DDEBUG 
debug: geocacher

release: CXXFLAGS += -O3
release: geocacher

geocacher: $(SOURCES)
	$(CC) $(SOURCES) --std=gnu99  $(CXXFLAGS) -I/usr/local/include -L/usr/local/lib  $(LDFLAGS) -o geocacher

geocacher.pb-c.c:
	protoc --c_out=. geocacher.proto

clean:
	rm -f geocacher
