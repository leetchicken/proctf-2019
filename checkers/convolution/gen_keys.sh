#!/bin/bash
openssl genpkey -out privkey.pem -algorithm rsa -pkeyopt rsa_keygen_bits:4096
openssl rsa -in privkey.pem -outform PEM -pubout -out pubkey.pem