#!/bin/sh

chown rubik: /app/data /app/settings.ini
chmod 755 /app /app/wwwroot /app/wwwroot/fonts
chmod 700 /app/data /app/settings.ini

su rubik -s /bin/sh -c 'dotnet rubik.dll'

