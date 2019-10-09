#!/bin/sh

mkdir -p /var/www/html 2>/dev/null
chown www-data:www-data /var/www/html/
chmod 700 /var/www/html/

exec su -s ./updater www-data
