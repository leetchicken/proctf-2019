#!/bin/bash

openssl req -x509 -nodes -newkey rsa:4096 -keyout ssl/key.pem -out ssl/cert.pem -subj '/C=AE/CN=127.0.0.1' -days 365
