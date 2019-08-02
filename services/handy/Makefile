GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
BINARY_NAME=build/handy
DOCKER_TAG=handy

all: run
build:
	docker run --rm -it -v "$(PWD)":/home/build -w /home/build/server -e CGO_ENABLED=0 golang:latest go build -o "../$(BINARY_NAME)" -v
	docker build -t $(DOCKER_TAG) .
build-no-docker:
	cd server; go build -o "../$(BINARY_NAME)" -v
clean: 
	$(GOCLEAN)
	rm -f $(BINARY_NAME)
	rm -f $(BINARY_UNIX)
run: build
	docker run --rm -it $(DOCKER_TAG)