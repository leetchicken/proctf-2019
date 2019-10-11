#!/bin/bash -e
# Generates a file list of the service

if [ -z "$1" ]; then
    echo "USAGE: get_service_files.sh <service>" >&2
    exit 1
fi

SERVICE="$1"

# The example command to generate file list:
# find services/fraud_detector
# Do not use spaces in files
# Put /./ to separate the path that will be copied to /service/$SERVICE/
# Put / in the end to copy the contents of the dir non-recursively

case "$SERVICE" in
    "fraud_detector")
    cat <<EOF
services/fraud_detector/./fraud_detector.py
services/fraud_detector/./docker-compose.yaml
services/fraud_detector/./start.sh
services/fraud_detector/./Dockerfile
services/fraud_detector/./app.py
services/fraud_detector/./static/
services/fraud_detector/./data/rules
services/fraud_detector/./data/users/
EOF
    ;;
    "polyfill")
    cat <<EOF
services/polyfill/./polyfill.wasm
services/polyfill/./docker-compose.yaml
services/polyfill/./polyfill_tcp/
services/polyfill/./polyfill_http/
EOF
    ;;
    "rubik")
    cat <<EOF
services/rubik/./docker-compose.yml
services/rubik/./Dockerfile
services/rubik/./settings.ini
services/rubik/./start.sh
services/rubik/./wwwroot/
services/rubik/./wwwroot/fonts/
services/rubik/./out/
EOF
    ;;
    "binder")
    cat <<EOF
services/Binder/./docker-compose.yml
services/Binder/./Dockerfile
services/Binder/./service/
services/Binder/./service/seccomp_test/
services/Binder/./service/static/
services/Binder/./service/templates/
EOF
    ;;
    "deer")
    cat <<EOF
services/deer/./docker-compose.yml
services/deer/./init.sh
EOF
    ;;
    "bb")
    cat <<EOF
services/bb/./docker-compose.yml
services/bb/./init.sh
services/bb/./b_b.conf
services/bb/./Dockerfile
services/bb/./lib/
services/bb/./lib/BB/
services/bb/./lib/BB/Controller/
services/bb/./lib/BB/Model/
services/bb/./public/
services/bb/./public/css/
services/bb/./public/js/
services/bb/./script/
services/bb/./sql/
services/bb/./ssl/
services/bb/./t/
services/bb/./templates/
services/bb/./templates/layouts/
services/bb/./templates/main/
EOF
    ;;
    "septon")
    cat <<EOF
services/SePtoN/./docker-compose.yml
services/SePtoN/./Dockerfile
services/SePtoN/./README.md
services/SePtoN/./start.sh
services/SePtoN/./Package.swift
services/SePtoN/./Sources/
services/SePtoN/./Sources/SePtoN/
EOF
    ;;
    "handy")
    cat <<EOF
services/handy/./docker-compose.yaml
EOF
    ;;
    "convolution")
    cat <<EOF
services/convolution/./
services/convolution/./compiler/
services/convolution/./compiler/antlr4-runtime/
services/convolution/./compiler/antlr4-runtime/include/
services/convolution/./compiler/antlr4-runtime/include/atn/
services/convolution/./compiler/antlr4-runtime/include/dfa/
services/convolution/./compiler/antlr4-runtime/include/misc/
services/convolution/./compiler/antlr4-runtime/include/support/
services/convolution/./compiler/antlr4-runtime/include/tree/
services/convolution/./compiler/antlr4-runtime/include/tree/pattern/
services/convolution/./compiler/antlr4-runtime/include/tree/xpath/
services/convolution/./compiler/antlr4-runtime/lib/
EOF
    ;;
    "gallery")
    cat <<EOF
services/gallery/./
services/gallery/./models/
services/gallery/./static/
EOF
    ;;
    "drone_racing")
    cat <<EOF
services/drone_racing/./docker-compose.yml
services/drone_racing/./backend/Dockerfile
services/drone_racing/./backend/build/distributions/drone_racing-1.0.0.zip
EOF
    ;;
    "ca")
    cat <<EOF
services/ca/./
services/ca/./data/
services/ca/./ssl/
EOF
    ;;
    "tracker")
    cat <<EOF
services/tracker/./Dockerfile
services/tracker/./main.py
services/tracker/./kernel/kernel-headers-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-cross-headers-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-tools-libs-devel-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-core-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-modules-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-tools-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-devel-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-modules-extra-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./kernel/kernel-tools-libs-4.18.0-80.7.1.el8.dccp.x86_64.rpm
services/tracker/./db
services/tracker/./db/drop.sql
services/tracker/./db/create.sql
services/tracker/./app
services/tracker/./app/enums.py
services/tracker/./app/__init__.py
services/tracker/./app/config.py
services/tracker/./app/protocol.py
services/tracker/./app/network.py
services/tracker/./app/common.py
services/tracker/./app/api
services/tracker/./app/api/track.py
services/tracker/./app/api/__init__.py
services/tracker/./app/api/tracker.py
services/tracker/./app/api/point.py
services/tracker/./app/api/user.py
services/tracker/./docker-compose.yml
services/tracker/./requirements.txt
EOF
    ;;
    "notepool")
    cat <<EOF
services/notepool/./docker-compose.yml
services/notepool/./Dockerfile
services/notepool/./start.sh
services/notepool/./out/
services/notepool/./out/wwwroot/
EOF
    ;;
    "startup")
    cat <<EOF
services/startup/./docker-compose.yaml
services/startup/./Dockerfile
services/startup/./start.sh
services/startup/./transport_http
services/startup/./transport_http2
services/startup/./updater
services/startup/./html/
services/startup/./src/
services/startup/./src/transport_http/
services/startup/./src/transport_http/src/
services/startup/./src/transport_http2/
services/startup/./src/transport_http2/src/
services/startup/./src/updater/
services/startup/./src/updater/src/
services/startup/./src/hashsum/readme.txt
EOF
    ;;
    "sql_demo")
    cat <<EOF
services/sql_demo/./docker-compose.yml
EOF
    ;;
    "spaceships")
    cat <<EOF
services/Spaceships/./docker-compose.yml
services/Spaceships/./Dockerfile
services/Spaceships/./service/
EOF
    ;;
    *) echo "No such service" >&2; exit 1;;
esac
