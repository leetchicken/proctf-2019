#!/bin/bash
PYTHONPATH=. \
    LISTEN_HOST=0.0.0.0 \
    LISTEN_PORT=9090 \
    DB_HOST=localhost:5432 \
    DB_NAME=tracker \
    DB_USER=tracker \
    pytest -v tests/*.py $@
