FROM ubuntu
RUN apt update
RUN apt install -y python3 python3-flask
RUN apt install -y libseccomp-dev
WORKDIR /home/service
RUN apt install -y python3-pip
RUN pip3 install dataset
RUN pip3 install flask_session
CMD ./srv.py
