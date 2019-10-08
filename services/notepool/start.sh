#!/bin/sh

chown notepool: /app/data
chmod -R 755 /app /app/wwwroot
chmod 700 /app/data

su notepool -s /bin/sh -c 'dotnet notepool.dll'
