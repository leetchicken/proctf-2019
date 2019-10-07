FROM python:3.7.3-alpine3.9

WORKDIR /home/tracker

RUN adduser tracker -u 10000 -D

COPY requirements.txt /home/tracker/
RUN apk add --update alpine-sdk
RUN pip3 install -r requirements.txt

COPY .venv /home/tracker/.venv
COPY app /home/tracker/app
COPY db /home/tracker/db
COPY main.py admin.py config.yaml /home/tracker/

RUN ./admin.py create

CMD ["./main.py"]
