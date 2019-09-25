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
services/rubik/./rubik.sln
services/rubik/./wwwroot/
services/rubik/./out/
EOF
    ;;
    *) echo "No such service" >&2; exit 1;;
esac
