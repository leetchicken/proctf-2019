FROM python:3.7-slim-stretch

RUN apt-get update
RUN apt-get -y install gcc
COPY requirements.txt /tmp
RUN pip install -r /tmp/requirements.txt

COPY . /app
WORKDIR /app

RUN adduser --system --home /app --shell /usr/sbin/nologin --no-create-home --disabled-password dotfm

CMD ["./start.sh"]