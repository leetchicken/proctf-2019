#!/bin/sh

chown geocacher: /app/data
chmod -R 755 /app
chmod 700 /app/data

su geocacher -s /bin/sh -c './geocacher'
