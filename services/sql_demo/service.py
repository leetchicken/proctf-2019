#!/usr/bin/env python2
  
import ctypes
import sys

SQLITE_OK = 0

@ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p))
def callback(not_used, argc, argv, col_names):
    values = []
    for i in range(0, argc):
        values.append(argv[i])
    print("|%s|" % '|'.join(values))
    return 0

libsqlite = ctypes.cdll.LoadLibrary("libsqlite3.so.0")

db = ctypes.c_void_p(0)
err_msg = ctypes.c_char_p(0)

rc = libsqlite.sqlite3_open(":memory:", ctypes.byref(db))
if rc != SQLITE_OK:
    print("Can't open database")
    libsqlite.sqlite3_close(db)
    sys.exit(-1)

while True:
    print('>'),
    sql = raw_input()

    if sql == '.quit':
        libsqlite.sqlite3_free(err_msg);
        libsqlite.sqlite3_close(db);
        sys.exit(0);

    rc = libsqlite.sqlite3_exec(db, sql, callback, 0, ctypes.byref(err_msg))
    if rc != SQLITE_OK:
        print("E! %s" % err_msg.value);
