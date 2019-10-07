FROM python:3.7.3-alpine3.9

RUN adduser gallery -u 10001 -D
RUN pip3.7 install gunicorn
RUN pip3.7 install numpy
RUN pip3.7 install scipy
RUN pip3.7 install Pillow
RUN pip3.7 install tensorflow
RUN pip3.7 install keras

WORKDIR /home/gallery/
CMD ["./start.sh"]
