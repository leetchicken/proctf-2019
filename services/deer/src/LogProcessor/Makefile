image = proctf/deer-log-processor
  
all: build-container

build-container:
	docker build -t $(image) -f Dockerfile .

clean:
	docker rmi $(image)
