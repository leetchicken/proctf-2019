FROM tensorflow/tensorflow:1.14.0-py3

RUN pip3 install --upgrade pip
RUN pip3 install keras
RUN pip3 install Pillow
RUN pip3 install gunicorn

RUN adduser --home /home/gallery --shell /usr/sbin/nologin --no-create-home --disabled-password gallery

WORKDIR /home/gallery/
CMD ["./start.sh"]
