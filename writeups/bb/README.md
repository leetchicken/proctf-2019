# BB bulletin board

## Desription

The service bulletin board (bb) is a simple and secure web site with the ability to create ads. Users can register, create own ads and leave private messages under any other ads. All messages are encrypted and stored in PostgreSQL database. Service has a special SQL injection vulnerability which allows reading files on the system, so hackers can get a decryption key and decrypt stolen messages.

## Features

1. register users by login/password
2. login/logout
3. create advertisements
4. list all/own advertisements
5. private communication with other users about advertisement
6. only encrypted messages in database

## Flags

Flags are private messages in bb threads. They are encrypted by `pgp_sym_encrypt` function from `pgcrypto` extension.

## Vuln

In `BB::Model::Boards->create_message()` input `$data` is not filtered, so we have an INSERT SQL injection.

Next, we need get a decryption key, it is stored in web app config (`b_b.conf`) and in ENV var `ENC_KEY` of `postgres` proccess. Web app config is in `bb_web` container, so we can get key only via ENV. This can be done via `pg_read_binary_file('/proc/1/environ', 0, 1024)` because `bb` connects to databse with postgres user as a result of misconfiguration.

Final message with sploit can look like this:

```
-' || (select string_agg(m, ',') from (select pgp_sym_decrypt(data, (select substring(encode(pg_read_binary_file('/proc/1/environ', 0, 1024), 'escape') from 'ENC_KEY=(\w+)'))) m from messages) x) || '-
```

## Soft

1. PostgreSQL
2. Perl + Mojolicious web framework

## Deploy

```bash
$ docker-compose build --pull
$ ./init.sh
$ docker-compose up -d
```
