# Handy

Handy is a peer-to-peer helping platform. It allows a person to register as a
master or as a regular user. A user can then list available masters and create a
task for a selected master.

## Typical Scenario

### Master

- Register on `/register`, specify it's a master
- Login on `/login`
- Look for new tasks on `/tasks`

### User

- Register on `/register`
- Login on `/login`
- Look at the list of masters on `/masters`
- Pick a master, open master's profile on `/profile`
- From the master's profile create a task

## Vuln

The way cookies are created and checked allows to exploit a Padding Oracle
attack. In Handy, cookie has the following structure:

```
cookie := <user-id> + encrypted(CRC32(<user-info>) + <user-info>)
```

where `<user-id>` is an UUID and `<user-info>` is a serialized proto with
additional information. Encryption algorithm is `AES-256` in `CRT` encryption
mode. `nonce` for `CRT` is a salted `SHA256` hash of the ID. The problem of this
solution is that while `CRT` doesn't require padding (it transforms a block
cipher into a stream cipher), the service actually pads the data.

Service parses the encrypted part lazily, i.e. until the data is actually
needed, proto in not deserialized and CRC32 is not checked. However, at the end
of every request, if a cookie is present, access is audit logged, using the data
from the cookie. The vulnerable target is `/profile/picture`, which generated a
GitHub-like picture of a given user of a given size. If the padding is invalid,
the request will fail before the picture is actually generated. If the padding
is valid, but the payload is corrupt, the handler will run, but the result will
still be `Bad Request`. However, if the image is large enough, one can notice a
difference in the wall time and use it as an oracle.

Using the oracle we can generate a cookie for any master, which, given that
the list of masters is openly available, allows us to read all of the created
tasks.

### Discovering

By tweaking the cookie value, we can quickly discover in the internal logs
`padding invalid` message, which is a start. Then thorough inspection of the
handlers allows to pick one that can leak some information about the padding
error.

### Fixing

Either padding should be removed or a handler should be modified to not leak the
information, e.g. by changing the max allowed size of the generated picture.
