# Geocaching

This is a reverse engineering/crypto/pwn service.

The service represents a storage facility that occupies the entire planet, storing items in "cells" addressed by geographical coordinates. Same geographical location might contain multiple items, so the users also need to provide a valid passphrase to access their items.

## Protocol

The service is loosely based on a protocol seen in the wild. Messages are packed into protobuf, the protobuf data is then encrypted with AES and packed into binary envelope. The envelope can be optionally authenticated with HMAC.

The binary envelope structure is as follows:
```
typedef struct __attribute__((__packed__))
{
    unsigned int msg_id;    // protobuf message type identifier
    unsigned int seq_id;    // message sequence number, increments by 1 for client messages,
                            // always 0 for server messages.
    unsigned char flags;    // `1` means that the packet is encrypted,
                            // `2` means that the packet is authenticated
    unsigned int data_size; // length of protobuf payload in bytes

    // char signature[32] — optional, used if the message is authenticated
    // char iv[16] — optional, used if the message is encrypted

    unsigned char data[];   // protobuf data
} Packet;
```

Binary envelopes are sent base64-encoded.

The service demands that most of the messages are encrypted. The service doesn't require message authentication to be present, but it brings certain protection against replay attacks.

### Handshake
The only three unencrypted messages are `AuthRequest`, `AuthResponse`, `AuthResult`. They are used to set up a simple transport encryption.

The client first sends `AuthRequest` with 256 bits of random data encrypted with an RSA public key of the service.

The service decrypts the client data and interprets the first half of it (`client_A`) as the AES key, and the second half as data to encrypt with said key. The service performs this encryption with a random IV and sends back `AuthResponse` containing IV and the result of the encryption.

The client decrypts the response and verifies that the result indeed matches the data that was sent to the service thus verifying the indentity of the service. After that the client sends the `AuthResult` message containing either `OK` or `FAIL`. `OK` means that the client is satisfied that the service is in possession of the private key, and the handshake is considered successful. Upon receiving `FAIL` the service terminates the connection.

After both parties finish the handshake, they set two master keys: AES key is used to encrypt all the following messages and is computed as `hmacsha256(b"2" * 32, client_A + iv)`. HMAC key is used to authenticate messages and is computed as `hmacsha256(b"1" * 32, client_A + iv)`.

### Create-Retrieve-Delete

The basic encrypted protocol implements CRUD but without the U, and consists of the following messages:
```
# Storing items
StoreSecretRequest - sent by client, contains the secret data and the desired location to store it
StoreSecretResponse - sent by service, contains the passphrase for the stored data

# Retrieving items
GetSecretRequest - sent by client, contains the location and the passphrase
GetSecretResponse - sent by service, contains the secret data

# Discarding items
DiscardSecretRequest - sent by client, contains the location and the passphrase
DiscardSecretResponse - sent by service
```

### Listing all cells

There's a special request to list all items from the planet:
```
ListAllBusyCellsRequest - sent by client
ListAllBusyCellsResponse - sent by service, contains the list of (coordinates, secret data).
```

However, this request is restricted to the planet administration, so upon receiving `ListAllBusyCellsRequest` the service sends back `AdminChallenge`. The client then must send back `AdminResponse`, and if the service is satisfied, it will respond with `ListAllBusyCellsResponse`.

### Other messages

The only other message is `UnknownMessage`, sent by the service if the incoming message cannot be parsed.

## Intended vulnerabilities

### RCE

The service is backed by an sqlite database, but it implements an in-memory storage to avoid accessing the db within a session. This fast storage holds 10 items. When the client attempts to store an item, the service first checks if there's an available slot in the fast storage, and falls back to sqlite if there isn't. When the client requests to retrieve or delete an item, it is first looked for in the fast storage, and then in the database if it wasn't found in fast storage.

This allows the client to only interact with the fast storage within the session. Upon session termination, the service dumps the fast storage into the database.

The fast storage is implemented as a static array of pointers. When the item is stored, fast storage allocates some memory with `malloc` as records the pointer. When the item is removed, `free` is performed and the pointer is nulled-out.

There's a trivial one-byte heap overflow in the storing function, the amount of memory allocated is always one byte short of the requested size. This can be exploited as any other one-byte heap overflow. The provided exploit converts one-byte overflow into multi-byte overflow by overwriting the size of the next chunk and then freeing and reallocating it. Once the multi-byte overflow is achieved, the exploit leaks the libc address by allocating and reading a chunk next to the arena border, and then uses the same overflowing technique to overwrite `__free_hook` in libc with a pointer to `execve('/bin/sh')` gadget.

To facilitate the exploitation there are two optional fields in `StoreSecretRequest`: `size_hint` and `key`.

Normally the allocated memory is one byte short of the size of the secret. But if a `size_hint` is provided, the service will allocate that much memory as long as its bigger than the size of the secret.

The `key` field allows to override the default random key that the service generates when the item is stored. One gadgets found in libc all have specific constraints. One of the gadgets needs both `rcx` and `rdx` to point to a zero, and luckily right before calling `free()` `rcx` and `rdx` are used to check that the provided key matches the one associated with the item. So if the item was stored with a zero key, both `rcx` and `rdx` will conveniently point at zeros.

100% of the flags can be retrieved via this bug.

**The fix**: Increase the malloc size parameter by one.

### RSA signature

When listing all items, the service sends `AdminChallenge` to verify that the client holds the private RSA key of the planet administrator (the checker). `AdminChallenge` consists on N bytes. The client receives the challenge, prepends it with the first 4 bytes of the session AES key, signs it with their RSA private key, and sends it back in `AdminResponse`. The service then verifies that the signature starts with the first 4 bytes of the session AES key followed by the challenge that was initially sent.

This challenge can be passed without knowing the private key due to a combination of four weaknesses:
* the signature doesn't implement any padding;
* the public exponent is small (17);
* the challenge is placed in the most significant bits of the message;
* the default challenge size is 6 bytes: 4 bytes of AES session key and 2 random bytes sent by the service.

Because of the first weakness the signature is essentially `s(m) = m ^ d mod n` and the verification is `m(s) = s ^ e mod n`. Because of this, there are messages for which the signature can be trivially recovered: for any message that is an `e`-th power of an integer, the signature is the `e`-th root of the message.

Because the public exponent is small, there's a lot of such messages. Precisely, for `n = 1024`, `e = 17` there are `2 ^ (1024 / 17) ~ 2 ^ 60` messages for which the signature is known.

The fact that the challenge is placed in the most significant bits of the message increases the likelihood of finding a message that is a `e`-th power of some integer _and_ contains the challenge in its upper bits.

Finally, the default size of the challenge makes it _very_ likely. Any challenge of 5 bytes or less can be trivially signed. Of 6-bytes challenges about 36% can be trivially signed. The probability decreases exponentially for bigger challenges.

Since the default challenge length is 6 bytes, the signature for such challenge can be forged in 3 attempts on average, giving access to `ListAllBusyCellsResponse`.

20% of the flags can be retrieved via this bug.

**The fix**: Increase the service challenge size.

## Technology

* `base64` by Jouni Malinen `<j@w1.fi>` is used to send binary messages in ASCII;
* `openssl` is used for the protocol cryptography (RSA, AES, HMAC256);
* `pgp` is used for big number modular arithmetics in `AdminChallenge`;
* `protobuf-c` implements C functions for `protobuf` message (de)-serialization;
* `sqlite3` is used for flags storage.
