FROM python:3.7-slim-stretch

COPY . /app
WORKDIR /app
RUN apt-get update
RUN apt-get -y install gcc
COPY requirements.txt /tmp
RUN pip install -r /tmp/requirements.txt


CMD ["python", "main.py"]