FROM python:3.7.3-alpine3.9

RUN adduser gallery -u 10001 -D
RUN apk add --update alpine-sdk
RUN pip3.7 install gunicorn
RUN pip3.7 install numpy
RUN apk add perl
RUN wget https://github.com/xianyi/OpenBLAS/archive/v0.3.6.tar.gz \
	&& tar -xf v0.3.6.tar.gz \
	&& cd OpenBLAS-0.3.6/ \
	&& make BINARY=64 FC=$(which gfortran) USE_THREAD=1 \
	&& make PREFIX=/usr/lib/openblas install
RUN ATLAS=/usr/lib/openblas/lib/libopenblas.so LAPACK=/usr/lib/openblas/lib/libopenblas.so pip3.7 install scipy
RUN pip3.7 install Pillow
RUN pip3.7 install tensorflow
RUN pip3.7 install keras

WORKDIR /home/gallery/
CMD ["./start.sh"]
