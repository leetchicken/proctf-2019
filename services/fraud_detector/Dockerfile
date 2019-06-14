FROM python:3.7.3-alpine3.9

RUN adduser fraud_detector -u 10000 -D
RUN pip3.7 install gunicorn

WORKDIR /home/fraud_detector/
CMD ["./start.sh"]
