#!/bin/bash

DST="single.py"

add() {
    name="$1"
    echo "  $name"
    echo -e "############################## $name ##############################\n" >> $DST
    cat "$name" | grep -v "^from app" | grep -v "^import app" | grep -v "^#!" >> $DST
}

cd "$(dirname $0)"

echo "#!/usr/bin/env python3" > $DST

echo "Merging files:"
add app/config.py
add app/common.py
add app/enums.py
add app/network.py
add app/protocol.py
add app/api/point.py
add app/api/track.py
add app/api/tracker.py
add app/api/user.py
add main.py

echo -en "Result:\n  "
wc -l $DST
