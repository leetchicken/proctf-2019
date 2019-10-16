# Handy

Handy is a peer-to-peer helping platform, inspired by services like [Helpling](https://en.wikipedia.org/wiki/Helpling) and named after a [song by Al Yankovic](https://www.youtube.com/watch?v=eXiwYUCe_bY). The service allows a person to register as a master or as a regular user. A user can then list available masters and create a task for a selected master.

## Usage

### Master

- Register and specify it's a master
- Login
- Look for new tasks and potentially even perform some of them

Registering new master is restricted to the checksystem, teams cannot create new masters.

### User

- Register
- Login
- Look at the list of available masters
- Pick a master, open master's profile
- From the master's profile create a new task

## Vuln

### Open Mongo

Unintentionally, mongo container is available to anyone without authentication. [Well](https://nakedsecurity.sophos.com/2019/08/06/attackers-ransom-booksellers-exposed-mongodb-database/), [shit](https://nakedsecurity.sophos.com/2019/05/10/275m-indian-citizens-records-exposed-by-insecure-mongodb-database/) [happens](https://www.bankinfosecurity.com/mongodb-database-exposed-188-million-records-researchers-a-12769).

### Padding Oracle

The way cookies are created and checked allows to exploit a Padding Oracle attack. In Handy, cookie has the following structure:

```
cookie := <user-id> + encrypted(CRC32(<user-info>) + <user-info>)
```

where `<user-id>` is an UUID and `<user-info>` is a serialized proto with additional information, like username and whether the user is a master. Encryption algorithm is `AES-256` in `CTR` encryption mode. `nonce` for `CTR` is a salted `SHA256` hash of the ID. The problem of this solution is that while `CTR` doesn't require padding (it transforms a block cipher into a stream cipher), the service actually pads the data.

Service parses the encrypted part lazily, i.e. until the data is actually needed, proto is not deserialized and CRC32 is not checked. However, at the end of every request, if a cookie is present, access is audit logged, using the data from the cookie.

The vulnerable target is the `/profile/picture` handler, which generated a GitHub-like picture of a given user of a given size. If the padding is invalid, the request will fail with `Bad Request` response code before the picture is actually generated. If the padding is valid, but the cookie payload is corrupt, the handler code will run, but the response will still be `Bad Request` due to audit logging. However, if the image is large enough, one can notice a difference in the wall time and use it as an oracle.

Using the oracle we can generate a cookie for any master, which, given that the list of masters is openly available, allows us to read all of the created tasks. Forging a string for the `CTR` mode using Padding Oracle is not something usually implemented in the hacking tools, but it's not complex and one can find the algorithm [on the internet](https://crypto.stackexchange.com/questions/18185/modes-of-operation-that-allow-padding-oracle-attacks).

### Discovering

Binary wasn't stripped, so by looking at the names of the functions we can spot `handy/server/util.Pkcs7Pad`, because Go doesn't have a padding algorithm implemented in the standard library, which should give an idea about a Padding Oracle. Tweaking the cookie value yields `invalid cookie` and looking at the usages, we can find the `handy/server/backends.(*CookieStorage).UnpackCookie` function. Disassembling it shows calls to `crypto/cipher.NewCTR` and `handy/server/util.Pkcs7Unpad`.

### Fixing

Either the padding should be removed or a handler should be modified to not leak the information, e.g. by changing the max allowed size of the generated picture.
