#!/bin/sh

export PYTHONUNBUFFERED=1

chown gallery:gallery /home/gallery/data
chmod 755 /home/gallery /home/gallery/static
chmod 700 /home/gallery/data/embeddings /home/gallery/data/paintings /home/gallery/data/previews /home/gallery/data/rewards

exec gunicorn -w 3 app:application -b 0.0.0.0:80 -u gallery -g gallery --capture-output --timeout=4 --reload
