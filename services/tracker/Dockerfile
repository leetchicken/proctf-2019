FROM python:3.7.3-alpine3.9

WORKDIR /home/tracker

RUN adduser tracker -u 10000 -D

COPY requirements.txt /home/tracker/
RUN apk add --update alpine-sdk
RUN pip3 install -r requirements.txt

COPY db /home/tracker/db
COPY single.py /home/tracker/

CMD ["./single.py"]
