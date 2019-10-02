GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
BINARY_NAME=build/handy
DOCKER_TAG=handy

all: build

build: build-in-container build-files/Dockerfile
	docker build -f build-files/Dockerfile -t $(DOCKER_TAG) .

build-in-container:
	docker run --rm -it -v "$(PWD)":/home/build -w /home/build/server -e CGO_ENABLED=0 golang:latest go build -o "../$(BINARY_NAME)" -v

build-without-container:
	cd server; go build -o "../$(BINARY_NAME)" -v

clean:
	$(GOCLEAN)
	rm -f $(BINARY_NAME)