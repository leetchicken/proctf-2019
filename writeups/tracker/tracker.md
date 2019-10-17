# Tracker

The "tracker" service allows users to register tracker devices. Each tracker device regularly sends its coordinates to the service. Service stores these tracks. Tracks are private by default, but can be shared among other users.

## Protocol

DCCP is used instead of TCP/UDP. It is an exotic message-oriented 
transport layer protocol. DCCP implements reliable connection setup, 
teardown and congestion control. It does not provide reliable in-order 
delivery.

Each message is a serialized by MessagePack Python structure.

Messages have two types:
- Requests, which are sent by users/trackers. They consist of 
**request type** and variable-sized arguments.
- Responses, which are sent by service. They consist of **status** 
and variable-sized arguments.

Requests are slightly encrypted (XOR-ed with a key). The key is sent 
by the service to the client as the first message after connection 
is established. Responses are not encrypted.

## API 

Request types:
```
USER_REGISTER = 0x00
USER_LOGIN = 0x01
USER_LOGOUT = 0x02
USER_DELETE = 0x03

TRACKER_LIST = 0x10
TRACKER_ADD = 0x11
TRACKER_DELETE = 0x12

POINT_ADD = 0x20
POINT_ADD_BATCH = 0x21

TRACK_LIST = 0x30
TRACK_GET = 0x31
TRACK_DELETE = 0x32
TRACK_REQUEST_SHARE = 0x33
TRACK_SHARE = 0x34
```

Response statuses:
```
OK = 0x00
BAD_REQUEST = 0x01
FORBIDDEN = 0x02
NOT_FOUND = 0x03
INTERNAL_ERROR = 0x04
```

The main object type - point - has four data fields:
- timestamp
- latitude
- longitude
- meta-information

Flags from the checksystem are embedded into points' meta-information. 
Each point stores one character of a flag. Points are grouped in tracks.
Each flag spans four tracks.

## Implementation

Service is written in Python language, but sources are not provided.
Only byte-code is provided (`tracker.bin`) with a small runner 
script (`execute.py`):
```
[root@tracker tracker]# docker ps
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS               NAMES
0f47b8c17ec8        proctf/tracker      "./start.sh"             11 hours ago        Up 10 hours                             tracker
84bd75c4356c        postgres:12.0       "docker-entrypoint.s…"   11 hours ago        Up 11 hours                             tracker_db_1
[root@tracker tracker]# docker exec -it 0f47b8c17ec8 sh
/home/tracker # ls -l
total 32
drwxr-sr-x    2 root     root            40 Oct 16 14:56 db
-rwxrwxr-x    1 root     root           271 Oct 16 18:14 execute.py
-rw-rw-r--    1 root     root           278 Oct 12 18:11 requirements.txt
-rwxrwxr-x    1 root     root            93 Oct 16 21:17 start.sh
-rw-rw-r--    1 root     root         18226 Oct 16 19:29 tracker.bin
```

Format of byte-code file is not ordinary `*.pyc`, but serialized
CodeType object. That's why just running, e.g. `uncompyle6` 
command-line tool doesn't help.

We should do something like that:
```
import marshal
import uncompyle6
import sys
with open(sys.argv[1], "rb") as f:
    raw = f.read()
    co = marshal.loads(raw)
    uncompyle6.code_deparse(co)
```

The service stores data in PostgreSQL database.
 
## Vulnerability

There is one known vulnerability: incorrect validation of access rights
in `TRACK_GET` method. By default, all tracks are private. Access to
a private track can be requested by any user using `TRACK_REQUEST_SHARE`
method. After that state of track in database changes from
`TrackAccess.PRIVATE` to `TrackAccess.PENDING`. Due to the bug, these
tracks are publicly readable:
```
async def track_get(db, secret, track_id):
...
    if (access is TrackAccess.PRIVATE or access is TrackAccess.PENDING) and user_id != row['user_id']:
        logging.warning('Get track is forbidden for user %d (owner: %d)', user_id, row['user_id'])
        return Response.FORBIDDEN
...
    return (Response.OK, points)
```

Here, `is` operator (compares the identity) is used instead of 
`==` (compares values) for numbers (access) comparison. 
`True` is returned by `is` only if both objects reference the 
same object. 

In CPython small integers are pre-allocated and exist in one  
instance. For them `is` works as `==`:
```
$ cat 1.py 
for x in range(-100, 300):
    b = x + 1
    b = b - 1
    print(x, x is b)

$ python3 1.py 
              # False for smaller numbers
-7 False
-6 False
-5 True       # <<<
-4 True
...           # True for these numbers
255 True
256 True      # <<<
257 False
258 False
              # False for bigger numbers
```

`TrackAccess.PENDING = 0x101 (257)`, that's why `access` value
obtained from DB will **never** be equal to program's constant
`TrackAccess.PENDING`. So, for `PENDING` tracks ownership is 
**not checked**.
