#!/bin/bash

# Build the docker contrainer for building the application
docker build -f Dockerfile.build --tag drone_racing_build .
# Build our application!
docker run -v "$PWD":/home/build drone_racing_build 
