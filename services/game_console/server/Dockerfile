FROM ubuntu:18.04

RUN apt-get update && \
	apt-get install -y libpugixml1v5 libmicrohttpd12 unzip

ADD . /root/server
WORKDIR /root/server

CMD ["./entrypoint.sh"]
