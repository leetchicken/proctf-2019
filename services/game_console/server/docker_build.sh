#!/bin/bash
docker build --tag=game_console_server .
docker run --restart=on-failure --detach -p 80:80 -p 8001:8001 -p 8002:8002 game_console_server 
