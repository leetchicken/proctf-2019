FROM python:3.7.3-alpine3.9

RUN adduser gallery -u 10001 -D
RUN pip3.7 install gunicorn

WORKDIR /home/gallery/
CMD ["./start.sh"]
