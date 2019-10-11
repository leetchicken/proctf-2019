FROM openjdk:14-jdk-alpine

MAINTAINER Andrew Gein <andgein@yandex.ru>

RUN apk update && apk upgrade && apk add unzip

RUN adduser drone_racing -u 10000 -D

WORKDIR /home/drone_racing

COPY build/distributions/drone_racing-1.0.0.zip /home/drone_racing/drone_racing-1.0.0.zip
RUN unzip /home/drone_racing/drone_racing-1.0.0.zip

CMD ["./drone_racing-1.0.0/bin/drone_racing"]
